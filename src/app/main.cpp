#include "global.hpp"
#include "logger/logger.hpp"
#include "input/input.hpp"
#include "menus/menus.hpp"
#include "settings/settings.hpp"
#include "fs/fs.hpp"

#include "window/window.hpp"

//Main app init
void init_app()
{
#ifdef WIN32
#ifdef _M_AMD64
	SetDllDirectoryA("x86_64");
#else
	SetDllDirectoryA("x86");
#endif
#endif

	fs::init();
	settings::init();

	global::desired_framerate = 60;
	global::framelimit = 1000 / global::desired_framerate;

	global::window = SDL_CreateWindow("Mr. Modman", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		global::resolution.x, global::resolution.y, SDL_WINDOW_BORDERLESS);

	SDL_SysWMinfo wmi;
	SDL_VERSION(&wmi.version);
	SDL_GetWindowWMInfo(global::window, &wmi);
	global::hwnd = wmi.info.win.window;

	//Use hardware
	if (!global::use_hardware)
	{
		global::surface = SDL_GetWindowSurface(global::window);
		global::renderer = SDL_CreateSoftwareRenderer(global::surface);
	}
	else if (global::use_hardware)
	{
		global::renderer = SDL_CreateRenderer(global::window, 0, SDL_RENDERER_ACCELERATED);
	}

	if (SDL_SetWindowHitTest(global::window, input::hit_test_callback, 0) != 0)
	{
		logger::log_error(logger::va("Failed to init hit test! %s", SDL_GetError()));
		global::shutdown = true;
	}

	menus::init();

	while (!global::shutdown)
	{
		global::tick_start();

		window::update();
		input::update();

		menus::prepare();
		menus::update();
		menus::present();

		global::tick_end();
	}

	menus::cleanup();
}

int main(int argc, char* argv[])
{

#ifdef _WIN32
#ifdef DEBUG
	AllocConsole();
	SetConsoleTitleA("Mr. Modman Debug Console");

	std::freopen("CONOUT$", "w", stdout);
	std::freopen("CONIN$", "r", stdin);
#endif

	//https://stackoverflow.com/a/940743
	//Some crap microsoft code i don't want to use to detect windows version
	if (global::winver == -1)
	{
		UINT   buffer_len = 0;
		LPBYTE buffer = 0;
		DWORD  info_size = GetFileVersionInfoSizeA("C:\\WINDOWS\\system32\\kernel32.dll", 0);
		LPSTR info_data = new char[info_size];

		if (info_size != 0)
		{
			if (GetFileVersionInfoA("C:\\WINDOWS\\system32\\kernel32.dll", 0, info_size, info_data))
			{
				if (VerQueryValueA(info_data, "\\", (LPVOID*)&buffer, &buffer_len))
				{
					if (buffer_len)
					{
						VS_FIXEDFILEINFO* winver_info = (VS_FIXEDFILEINFO*)buffer;
						if (winver_info->dwSignature == 0xfeef04bd)
						{
							DWORD major_ver = (winver_info->dwFileVersionLS >> 16) & 0xffff;

							if (major_ver < 22000)
							{
								global::winver = 10;
							}
							else if (major_ver >= 22000)
							{
								global::winver = 11;
							}
						}
					}
				}
			}
		}

		delete[] info_data;
	}

#endif

	if (SDL_Init(SDL_INIT_EVENTS | SDL_INIT_TIMER | SDL_INIT_VIDEO) != 0)
	{
		logger::log_error(logger::va("%s", SDL_GetError()));
		global::shutdown = true;
	}

	init_app();

	return 0;
}