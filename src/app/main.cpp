#include "global.hpp"
#include "logger/logger.hpp"
#include "input/input.hpp"
#include "menus/menus.hpp"
#include "settings/settings.hpp"
#include "fs/fs.hpp"

#ifndef OVERLAY
#include "window/window.hpp"
#else
#include "hook/impl/d3d9_impl.h"
#include "hook/impl/d3d10_impl.h"
#include "hook/impl/d3d11_impl.h"
#include "hook/impl/opengl3_impl.h"
#endif

//Main app init
#ifndef OVERLAY
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
	SetConsoleTitleA("Radio.Garten Debug Console");

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
							logger::log("WINVER", logger::va("%u", major_ver));

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

#else
HMODULE self;

//Overlay init
void init_overlay()
{
	settings::init();
	if (kiero::init(kiero::RenderType::Auto) == kiero::Status::Success)
	{
		switch (kiero::getRenderType())
		{
#if KIERO_INCLUDE_D3D9
		case kiero::RenderType::D3D9:
			impl::d3d9::init();
			break;
#endif

#if KIERO_INCLUDE_D3D10
		case kiero::RenderType::D3D10:
			impl::d3d10::init();
			break;
#endif

#if KIERO_INCLUDE_D3D11
		case kiero::RenderType::D3D11:
			impl::d3d11::init();
			break;
#endif

#if KIERO_INCLUDE_OPENGL
		case kiero::RenderType::OpenGL:
			impl::opengl3::init();
			break;
#endif

		case kiero::RenderType::None:
			FreeLibraryAndExitThread(self, 0);
			break;
		}
	}
}

bool __stdcall DllMain(::HMODULE hmod, ::DWORD reason, ::LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
#ifdef DEBUG
		AllocConsole();
		SetConsoleTitleA("Radio.Garten Debug Console");

		std::freopen("CONOUT$", "w", stdout);
		std::freopen("CONIN$", "r", stdin);
		logger::log_info("Attached!");
#endif
		DisableThreadLibraryCalls(hmod);
		self = hmod;
		CreateThread(0, 0, (LPTHREAD_START_ROUTINE)init_overlay, 0, 0, 0);
	}
	return true;
}
#endif