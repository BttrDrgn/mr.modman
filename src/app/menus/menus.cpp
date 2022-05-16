#include "global.hpp"
#include "logger/logger.hpp"
#include "fs/fs.hpp"
#include "menus.hpp"
#include "settings/settings.hpp"

#ifdef _WIN32
#include <shellapi.h>
#endif

void menus::init()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGui::GetIO().IniFilename = nullptr;

	menus::build_font(ImGui::GetIO());

	ImGui_ImplSDL2_InitForSDLRenderer(global::window, global::renderer);
	ImGui_ImplSDLRenderer_Init(global::renderer);
}

void menus::prepare()
{
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	SDL_SetRenderDrawColor(global::renderer, 30, 30, 30, 255);
	SDL_RenderClear(global::renderer);
}

void menus::present()
{
	ImGui::Render();
	ImGui_ImplSDLRenderer_RenderDrawData(ImGui::GetDrawData());

	if (!global::use_hardware)
	{
		SDL_UpdateWindowSurface(global::window);

	}
	else if (global::use_hardware)
	{
		SDL_RenderPresent(global::renderer);
	}
}

void menus::cleanup()
{
	ImGui_ImplSDLRenderer_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(global::renderer);
	SDL_DestroyWindow(global::window);
	SDL_Quit();
}

void menus::update()
{
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize(global::resolution);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;
	if (ImGui::Begin("Mr. Modman", nullptr, flags))
	{
		menus::menu_bar();
		menus::console();
		ImGui::End();
	}


	if (menus::show_new_game)
	{
		ImGuiWindowFlags ng_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

		static ImVec2 size = { 400, 300 };
		ImGui::SetNextWindowPos({(global::resolution.x / 2) - (size.x / 2), (global::resolution.y / 2) - (size.y / 2)}, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin("New Game", nullptr, ng_flags))
		{
			ImGui::Text("Game Name:");
			ImGui::SameLine();
			ImGui::InputText("##game_name", menus::game_name_buffer, sizeof(menus::game_name_buffer));
			ImGui::Text("Game Path:");
			ImGui::SameLine();
			ImGui::InputText("##game_path", menus::game_path_buffer, sizeof(menus::game_path_buffer));

			if (ImGui::Button("Browse##game"))
			{
				fs::browse(menus::game_path_buffer, "Executable (.exe)\0*.exe\0\0", "Select your game executable");
			}

			ImGui::NewLine();

			ImGui::Text("Custom Mod Directory:");
			ImGui::SameLine();
			ImGui::Checkbox("##custom_dir", &menus::use_custom_dir);

			if (ImGui::IsItemHovered())
			{
				std::string path = fs::get_pref_dir().append("mods\\");

				if (std::strlen(menus::game_name_buffer) <= 0)
				{
					path.append("{GAME_NAME}\\");
				}
				else if (std::strlen(menus::game_name_buffer) > 0)
				{
					path.append(logger::va("%s\\", menus::game_name_buffer));
				}

				ImGui::BeginTooltip();
				ImGui::Text("Set a custom path for your mods to load from.");
				ImGui::Text("By default, the mods will load from:\n%s", path.c_str());
				ImGui::EndTooltip();
			}

			if (menus::use_custom_dir)
			{
				ImGui::Text("Mod Directory:");
				ImGui::SameLine();
				ImGui::InputText("##custom_mod_path", menus::custom_dir_buffer, sizeof(menus::custom_dir_buffer));
				ImGui::Button("Browse##path");
			}
			else if(!menus::custom_dir_buffer)
			{
				menus::clear_buffer(menus::custom_dir_buffer, sizeof(menus::custom_dir_buffer));
			}

			ImGui::SetCursorPos({size.x - 100, size.y - 35});
			if (ImGui::Button("Close"))
			{
				menus::show_new_game = false;
				menus::clear_buffer(menus::game_name_buffer, sizeof(menus::game_name_buffer));
				menus::clear_buffer(menus::game_path_buffer, sizeof(menus::game_path_buffer));
				menus::clear_buffer(menus::custom_dir_buffer, sizeof(menus::custom_dir_buffer));
				menus::use_custom_dir = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Finish"))
			{
				std::string name = menus::game_name_buffer;
				std::string path = menus::game_path_buffer;
				std::string custom_path = menus::custom_dir_buffer;
				std::string mod_path;
				std::string config_path;
				std::string game_cwd;

				if (std::strlen(menus::game_name_buffer) <= 0)
				{
					logger::log_error(logger::va("Game Name invalid. Did you insert a game name?"));
					ImGui::End();
					return;
				}

				if (std::strlen(menus::game_path_buffer) <= 0)
				{
					logger::log_error(logger::va("Game Path invalid. Did you insert a game path?"));
					ImGui::End();
					return;
				}

				if (!fs::exists(path))
				{
					logger::log_error(logger::va("File \"%s\" not found. Did you select a game path?", path.c_str()));
					ImGui::End();
					return;
				}

				if (menus::use_custom_dir)
				{
					if (std::strlen(menus::custom_dir_buffer) <= 0)
					{
						logger::log_error(logger::va("Custom Mod Directory invalid. Did you insert a directory?"));
						ImGui::End();
						return;
					}
					else
					{
						mod_path = custom_path.append(logger::va("mods\\%s", menus::game_name_buffer));
						config_path = mod_path;
					}
				}
				else
				{
					mod_path = fs::get_pref_dir().append(logger::va("mods\\%s", menus::game_name_buffer));
					config_path = mod_path;
				}

				if (!fs::exists(mod_path))
				{
					logger::log_info(logger::va("\"%s\" created at \"%s\"", menus::game_name_buffer, mod_path.c_str()));
					fs::mkdir(mod_path.append("\\_global"));
				}
				else
				{
					logger::log_error(logger::va("\"%s\" already exists at \"%s\"!", menus::game_name_buffer, fs::get_pref_dir().append("mods\\").c_str()));
					ImGui::End();
					return;
				}

				std::vector<std::string> temp = logger::split(path, "\\");
				for (auto i = 0; i < temp.size() - 1; i++)
				{
					game_cwd.append(temp[i] + "\\");
				}

				std::string ini = logger::va("[game]\nPath=%s\nCWD=%s", menus::game_path_buffer, game_cwd.c_str());
				ini_t* ini_t = ini_create(ini.c_str(), std::strlen(ini.c_str()));
				ini_save(ini_t, config_path.append("\\config.ini").c_str());

				menus::current_game = {name, menus::game_path_buffer, game_cwd};

				logger::log_info(logger::va("Finished setup for %s!", menus::game_name_buffer));
				settings::update();

				menus::show_new_game = false;
				menus::clear_buffer(menus::game_name_buffer, sizeof(menus::game_name_buffer));
				menus::clear_buffer(menus::game_path_buffer, sizeof(menus::game_path_buffer));
				menus::clear_buffer(menus::custom_dir_buffer, sizeof(menus::custom_dir_buffer));
				menus::use_custom_dir = false;

			}
		}
		ImGui::End();
	}
}

void menus::menu_bar()
{
	if (ImGui::BeginMenuBar())
	{
		menus::file();

		if (menus::current_game.name != "")
		{
			if (ImGui::Button("Play"))
			{
				logger::log_debug(logger::va("loader.exe --exe %s --cwd %s", menus::current_game.path.c_str(), menus::current_game.cwd.c_str()));

				STARTUPINFOA startup_info;
				PROCESS_INFORMATION process_info;

				memset(&startup_info, 0, sizeof(startup_info));
				memset(&process_info, 0, sizeof(process_info));
				startup_info.cb = sizeof(startup_info);

				logger::log_debug(fs::get_cur_dir());

				CreateProcessA
				(
					fs::get_cur_dir().append("loader.exe").c_str(),
					logger::va("--exe %s --cwd %s", menus::current_game.path, menus::current_game.cwd).data(),
					nullptr,
					nullptr,
					false,
					0,
					nullptr,
					fs::get_cur_dir().c_str(),
					&startup_info,
					&process_info
				);
				
				logger::log_debug(logger::va("%i", GetLastError()));
			}
		}

		ImGui::EndMenuBar();
	}
}

void menus::file()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::Button("New Game")) menus::show_new_game = true;
		menus::load_game();
		ImGui::Text("__________");
		if(ImGui::Button("Exit")) global::shutdown = true;
		ImGui::EndMenu();
	}
}

void menus::console()
{
	static ImVec2 size = { global::resolution.x - 10, 200 };
	ImGui::SetNextWindowPos({ 5, global::resolution.y - 210 });
	if (ImGui::BeginChild("Console", size, 1, 0))
	{
		for (auto entry : menus::console_output)
		{
			ImGui::Text(entry.c_str());
		}

		ImGui::EndChild();
	}
}

void menus::load_game()
{
	if (ImGui::BeginMenu("Load Game"))
	{
		for (auto game : menus::games)
		{
			if (ImGui::Button(game.c_str()))
			{
				ini_t* ini = ini_load(fs::get_pref_dir().append(logger::va("mods\\%s\\config.ini", game.c_str())).c_str());
				menus::current_game = {game, ini_get(ini, "game", "path"), ini_get(ini, "game", "cwd")};
				logger::log_info(logger::va("%s loaded!", menus::current_game.name));
			}
		}

		ImGui::EndMenu();
	}
}

void menus::build_font(ImGuiIO& io)
{
	std::string font = fs::get_pref_dir().append("fonts/NotoSans-Regular.ttf");
	std::string font_jp = fs::get_pref_dir().append("fonts/NotoSansJP-Regular.ttf");
	std::string emoji = fs::get_pref_dir().append("fonts/NotoEmoji-Regular.ttf");

	if (fs::exists(font))
	{
		io.Fonts->AddFontFromFileTTF(font.c_str(), 18.0f);

		static ImFontConfig cfg;
		static ImWchar emoji_ranges[] = { 0x1, 0x1FFFF, 0 };

		if (fs::exists(emoji))
		{
			cfg.MergeMode = true;
			cfg.OversampleH = cfg.OversampleV = 1;
			io.Fonts->AddFontFromFileTTF(emoji.c_str(), 12.0f, &cfg, emoji_ranges);
		}

		if (fs::exists(font_jp))
		{
			ImFontConfig cfg;
			cfg.OversampleH = cfg.OversampleV = 1;
			cfg.MergeMode = true;
			io.Fonts->AddFontFromFileTTF(font_jp.c_str(), 18.0f, &cfg, io.Fonts->GetGlyphRangesJapanese());
		}
	}
}

void menus::clear_buffer(char* buffer, size_t size)
{
	memset(buffer, 0, size);
}

char menus::game_path_buffer[MAX_PATH];
char menus::game_name_buffer[32];
bool menus::use_custom_dir = false;
char menus::custom_dir_buffer[MAX_PATH];

bool menus::show_new_game = false;

std::vector<std::string> menus::console_output;
std::vector<std::string> menus::games;
game_t menus::current_game;