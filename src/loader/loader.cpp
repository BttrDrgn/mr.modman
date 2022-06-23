#include "loader.hpp"

#include "logger/logger.hpp"
#include "fs/fs.hpp"
#include "hook/hook.hpp"

bool has_tls = false;
unsigned long entry_point = 0;

#undef min
#undef max

#pragma comment(linker, "/base:0x400000")
#pragma comment(linker, "/merge:.data=.cld")
#pragma comment(linker, "/merge:.rdata=.clr")
#pragma comment(linker, "/merge:.cl=.main")
#pragma comment(linker, "/merge:.text=.main")
#pragma comment(linker, "/section:.main,re")

#pragma bss_seg(".payload")
//Max we can load without crashing, maybe look into automatic data size
char payload_data[0x70FFFFFF];

#pragma data_seg(".main")
char main_data[0x1000] = { 1 };

#pragma optimize( "", off )
__declspec(thread) char tls_data[0x10000];
#pragma optimize( "", on )

void load_section(const HMODULE target, const HMODULE source, IMAGE_SECTION_HEADER* section)
{
    void* target_ptr = reinterpret_cast<void*>(reinterpret_cast<std::uint32_t>(target) + section->VirtualAddress);
    const void* source_ptr = reinterpret_cast<void*>(reinterpret_cast<std::uint32_t>(source) + section->PointerToRawData);

    if (section->SizeOfRawData > 0)
    {
        const auto size_of_data = std::min(section->SizeOfRawData, section->Misc.VirtualSize);

        DWORD old_protect;
        VirtualProtect(target_ptr, size_of_data, PAGE_EXECUTE_READWRITE, &old_protect);

        std::memmove(target_ptr, source_ptr, size_of_data);
    }

    if (!strcmp((char*)(section->Name), ".tls"))
    {
        has_tls = true;
    }
}

void load_sections(const HMODULE target, const HMODULE source)
{
    const auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(source);
    const auto nt_headers = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<std::uint32_t>(source) + dos_header->e_lfanew);

    auto section = IMAGE_FIRST_SECTION(nt_headers);

    for (auto i = 0u; i < nt_headers->FileHeader.NumberOfSections; ++i, ++section)
    {
        if (section)
        {
            load_section(target, source, section);
        }
    }
}

HMODULE find_library(LPCSTR library)
{
    auto handle = GetModuleHandleA(library);

    if (!handle)
    {
        handle = LoadLibraryA(library);
    }

    return handle;
}

void load_imports(const HMODULE target, const HMODULE source)
{
    const auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(source);
    const auto nt_headers = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<std::uint32_t>(source) + dos_header->e_lfanew);

    const auto import_directory = &nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    auto descriptor = PIMAGE_IMPORT_DESCRIPTOR(reinterpret_cast<std::uint32_t>(target) + import_directory->VirtualAddress);

    while (descriptor->Name)
    {
        std::string library_name = LPSTR(reinterpret_cast<std::uint32_t>(target) + descriptor->Name);

        auto name_table_entry = reinterpret_cast<uintptr_t*>(reinterpret_cast<std::uint32_t>(target) + descriptor->OriginalFirstThunk);
        auto address_table_entry = reinterpret_cast<uintptr_t*>(reinterpret_cast<std::uint32_t>(target) + descriptor->FirstThunk);

        if (!descriptor->OriginalFirstThunk)
        {
            name_table_entry = reinterpret_cast<uintptr_t*>(reinterpret_cast<std::uint32_t>(target) + descriptor->FirstThunk);
        }

        while (*name_table_entry)
        {
            FARPROC function = nullptr;

            if (IMAGE_SNAP_BY_ORDINAL(*name_table_entry))
            {
                auto module = find_library(library_name.data());
                if (module)
                {
                    function = GetProcAddress(module, MAKEINTRESOURCEA(IMAGE_ORDINAL(*name_table_entry)));
                }
            }
            else
            {
                auto import = PIMAGE_IMPORT_BY_NAME(reinterpret_cast<std::uint32_t>(target) + *name_table_entry);

                auto module = find_library(library_name.data());
                if (module)
                {
                    function = GetProcAddress(module, import->Name);
                }
            }

            if (!function)
            {
                throw std::runtime_error("unresolved import!");
            }

            *address_table_entry = reinterpret_cast<uintptr_t>(function);

            name_table_entry++;
            address_table_entry++;
        }

        descriptor++;
    }
}

void verify_tls()
{
    const auto self = GetModuleHandleA(nullptr);
    const auto dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(self);
    const auto nt_headers = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<std::uint32_t>(self) + dos_header->e_lfanew);

    const auto self_tls = reinterpret_cast<PIMAGE_TLS_DIRECTORY>(reinterpret_cast<std::uint32_t>(self) + nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);

    const auto ref = std::uintptr_t(&tls_data);
    const auto tls_index = *reinterpret_cast<std::uintptr_t*>(self_tls->AddressOfIndex);

    const auto tls_vector = *reinterpret_cast<std::uintptr_t*>(__readfsdword(0x2C) + 4 * tls_index);

    const auto offset = ref - tls_vector;

    if (offset != 0 && offset != (sizeof(std::uintptr_t) * 2))
    {
        throw std::runtime_error("TLS mapping is wrong!");
    }
}

void loader::load(const char* bin_name)
{
    memset(tls_data, 0, sizeof tls_data);

    std::ifstream bin(bin_name, std::ifstream::binary);

    if (!bin.is_open())
    {
        return;
    }

    bin.seekg(0, bin.end);
    auto binary_size = bin.tellg();
    bin.seekg(0, bin.beg);

    std::vector<std::uint8_t> executable_buffer;
    executable_buffer.resize(binary_size);

    bin.read(reinterpret_cast<char*>(&executable_buffer[0]), binary_size);

    const auto module = GetModuleHandleA(nullptr);
    const auto module_dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
    const auto module_nt_headers = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uint32_t>(module) + module_dos_header->e_lfanew);

    const auto source = reinterpret_cast<HMODULE>(&executable_buffer[0]);
    const auto source_dos_header = reinterpret_cast<IMAGE_DOS_HEADER*>(source);
    const auto source_nt_headers = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<uint32_t>(source) + source_dos_header->e_lfanew);

    if (source_nt_headers->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64)
    {
        MessageBoxA(nullptr, "This binary is x64, this loader only supports x86 binaires!", "Loader", 0);
        exit(0);
    }

    load_sections(module, source);
    load_imports(module, source);

    if (source_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].Size)
    {
        if (!module_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress)
        {
            __debugbreak();
        }


        const auto target_tls = reinterpret_cast<PIMAGE_TLS_DIRECTORY>(reinterpret_cast<uint32_t>(module) + module_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);
        const auto source_tls = reinterpret_cast<PIMAGE_TLS_DIRECTORY>(reinterpret_cast<uint32_t>(module) + source_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS].VirtualAddress);

        const auto source_tls_size = source_tls->EndAddressOfRawData - source_tls->StartAddressOfRawData;
        const auto target_tls_size = target_tls->EndAddressOfRawData - target_tls->StartAddressOfRawData;

        const auto target_tls_index = *reinterpret_cast<DWORD*>(target_tls->AddressOfIndex);
        const auto source_tls_index = *reinterpret_cast<DWORD*>(source_tls->AddressOfIndex);
        *reinterpret_cast<DWORD*>(target_tls->AddressOfIndex) += source_tls_index;

        DWORD old_protect;
        VirtualProtect(PVOID(target_tls->StartAddressOfRawData), source_tls_size, PAGE_READWRITE, &old_protect);

        const auto tls_base = *reinterpret_cast<LPVOID*>(__readfsdword(0x2C) + (sizeof(std::uintptr_t) * source_tls_index) + (sizeof(std::uintptr_t) * target_tls_index));
        std::memmove(tls_base, PVOID(source_tls->StartAddressOfRawData), source_tls_size);
        std::memmove(PVOID(target_tls->StartAddressOfRawData), PVOID(source_tls->StartAddressOfRawData), source_tls_size);
    }

    entry_point = (source_nt_headers->OptionalHeader.ImageBase + source_nt_headers->OptionalHeader.AddressOfEntryPoint);

    DWORD old_protect;
    VirtualProtect(module_nt_headers, 0x1000, PAGE_EXECUTE_READWRITE, &old_protect);

    module_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT] = source_nt_headers->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    std::memmove(module_nt_headers, source_nt_headers, sizeof(IMAGE_NT_HEADERS) + (module_nt_headers->FileHeader.NumberOfSections * (sizeof(IMAGE_SECTION_HEADER))));

    if (has_tls)
    {
        verify_tls();
    }
}

std::string game_name;
std::string pack_name;
std::string cwd;

static HANDLE(__stdcall* oCreateFile)(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile);

HANDLE __stdcall create_file(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile)
{
	//Convert to string for easier string ops
	//Would def be fast to use C
	std::string file_name = lpFileName;

	//If we found a file that is read from the game dir
	if (file_name.find(cwd) != std::string::npos)
	{
		//Shorten the filename to the game dir
		file_name = logger::replace(file_name, "/", "\\").erase(0, cwd.length());

		//Global comes first
		std::string global = fs::get_pref_dir().append("mods\\" + game_name + "\\_global" + file_name);
		if (fs::exists(file_name))
		{
			logger::log("GLOBAL", global);
			return oCreateFile(global.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		}

		//Then pack
		std::string pack = fs::get_pref_dir().append("mods\\" + game_name + "\\" + pack_name + file_name);
		if (fs::exists(pack))
		{
			logger::log("PACK", pack);
			return oCreateFile(pack.c_str(), dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
		}
	}

	//Then original if nothing found
	return oCreateFile(lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

std::initializer_list<std::string> ext_whitelist
{
    ".dll",
    ".asi",
};

int init()
{

#ifdef DEBUG
    AllocConsole();
    SetConsoleTitleA("Loader Debug Console");
    std::freopen("CONOUT$", "w", stdout);
    std::freopen("CONIN$", "r", stdin);
#endif

    for (auto i = 0; i < __argc; i++)
    {
        if (!strcmp("--exe", __argv[i]))
        {
            loader::load(__argv[i + 1]);
        }
        else if (!strcmp("--cwd", __argv[i]))
        {
			cwd = __argv[i + 1];
			cwd.erase(cwd.length() - 1, 1);
            SetCurrentDirectoryA(cwd.c_str());
        }
        else if (!strcmp("--game", __argv[i]))
        {
            game_name = __argv[i + 1];
        }
        else if (!strcmp("--modpack", __argv[i]))
        {
            pack_name = __argv[i + 1];
        }
    }

    //Load _global
    std::string global = fs::get_pref_dir().append(logger::va("mods\\%s\\_global\\", game_name.c_str()));
    for (auto bin : fs::get_all_files(global))
    {
        for (auto ext : ext_whitelist)
        {
            if (logger::ends_with(bin, ext))
            {
                std::string to_load = global + bin;
                LoadLibraryA(to_load.c_str());
            }
        }
    }

    //Load pack
    std::string pack = fs::get_pref_dir().append(logger::va("mods\\%s\\%s\\", game_name.c_str(), pack_name.c_str()));
    for (auto bin : fs::get_all_files(fs::get_pref_dir().append(logger::va("mods\\%s\\%s\\", game_name.c_str(), pack_name.c_str()))))
    {
        for (auto ext : ext_whitelist)
        {
            if (logger::ends_with(bin, ext))
            {
                std::string to_load = pack + bin;
                LoadLibraryA(to_load.c_str());
            }
        }
    }

	MH_Initialize();

	//MH_CreateHookApi(L"kernel32.dll", "ReadFile", (void**)&read_file, (void**)&oReadFile);
	MH_CreateHookApi(L"kernel32.dll", "CreateFileA", (void**)&create_file, (void**)&oCreateFile);

	MH_EnableHook(MH_ALL_HOOKS);

	MessageBoxA(0, 0, 0, 0);

    return loader::run(entry_point);
}

#ifdef _CONSOLE
int __cdecl main(int argc, char argv[])
{
    return init();
}
#else
int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    return init();
}
#endif
