#pragma once

#include <SDL.h>
#include <fstream>
#include <sstream>
#include <filesystem>

#include <commdlg.h>

class fs
{
public:
	//Copies important files to the pref directory
	static void init()
	{
		if (!fs::exists(fs::get_pref_dir().append("mods\\")))
		{
			fs::mkdir(fs::get_pref_dir().append("mods\\"));
		}

		if (fs::exists("fonts"))
		{
			fs::move(fs::get_cur_dir().append("fonts"), fs::get_pref_dir().append("fonts"));
			fs::del("fonts");
		}
	}

	static bool exists(const std::string& path)
	{
		return std::filesystem::exists(path);
	}

	static std::string get_cur_dir()
	{
		return std::filesystem::current_path().string() + "\\";
	}

	static std::string get_pref_dir()
	{
#ifndef HELPER
		return std::string(SDL_GetPrefPath("BttrDrgn", "mr.modman"));
#endif
		return "";
	}

	static void write(const std::string& path, const std::string& contents, const bool append)
	{
		std::ofstream stream(path, std::ios::binary | std::ofstream::out | (append ? std::ofstream::app : 0));

		if (stream.is_open())
		{
			stream.write(contents.data(), static_cast<std::streamsize>(contents.size()));
			stream.close();
		}
	}

	static void mkdir(const std::string& path)
	{
		std::filesystem::create_directories(path);
	}

	static std::vector<std::string> get_all_files(const std::string& path)
	{
		std::vector<std::string> retn;
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (!entry.is_directory())
			{
				std::vector<std::string> temp = logger::split(entry.path().string(), "\\");
				retn.emplace_back(temp[temp.size() - 1]);
			}
		}

		return retn;
	}

	static std::vector<std::string> get_all_dirs(const std::string& path)
	{
		std::vector<std::string> retn;
		for (const auto& entry : std::filesystem::directory_iterator(path))
		{
			if (entry.is_directory())
			{
				std::vector<std::string> temp = logger::split(entry.path().string(), "\\");
				retn.emplace_back(temp[temp.size() - 1]);
			}
		}

		return retn;
	}

	static std::string read(const std::string& path)
	{
		std::ifstream in(path);
		std::ostringstream out;
		out << in.rdbuf();
		return out.str();
	}

	static void del(const std::string& path, bool folder = false)
	{
		if (!fs::exists(path)) return;

		switch (folder)
		{
		case false:
			std::filesystem::remove(path);
			break;
		case true:
			std::filesystem::remove_all(path);
			break;
		}
	}

	static void move(const std::string& path, const std::string& new_path, bool create_root = true)
	{
		if (create_root)
		{
			std::filesystem::create_directory(new_path);
		}

		for (const std::filesystem::path& p : std::filesystem::directory_iterator(path))
		{
			std::filesystem::path dest = new_path / p.filename();

			if (fs::exists(path))
			{
				if (std::filesystem::is_directory(p))
				{
					std::filesystem::create_directory(dest);
					fs::move(p.string().c_str(), dest.string().c_str(), false);
				}
				else
				{
					std::filesystem::rename(p, dest);
				}
			}
		}
	}

#ifndef LOADER
	static void browse(char* buffer, char* filter, char* message)
	{
		std::string cwd = fs::get_cur_dir();

		OPENFILENAMEA browse;
		memset(&browse, 0, sizeof(browse));
		browse.lpstrInitialDir = cwd.c_str();
		browse.lStructSize = sizeof(browse);
		browse.hwndOwner = global::hwnd;
		browse.lpstrFilter = filter;
		browse.lpstrFile = buffer;
		browse.nMaxFile = MAX_PATH;
		browse.lpstrTitle = message;
		browse.Flags = 0x00001000;

		GetOpenFileNameA(&browse);

		SetCurrentDirectoryA(cwd.c_str());
	}

	static void open_editor(const char* file)
	{
		ShellExecuteA(0, "open", file, 0, 0, 0);

		if (GetLastError() == ERROR_NO_ASSOCIATION)
		{
			ShellExecuteA(0, "edit", file, 0, 0, 0);
		}
	}
#endif
};