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

	//Style init
	ImGuiStyle& s = ImGui::GetStyle();
	s.PopupBorderSize = 0.0f;
	s.WindowBorderSize = 0.0f;
	s.ScrollbarSize = 5.0f;
	s.ScrollbarRounding = 0.0f;
}

void menus::prepare()
{
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	SDL_SetRenderDrawColor(global::renderer, menus::background_col.r, menus::background_col.g, menus::background_col.b, menus::background_col.a);
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

	std::string title;
	if (menus::current_game.name == "" && menus::current_game.pack == "")
	{
		title = "Mr. Modman###main";

	}
	else if (menus::current_game.name != "" && menus::current_game.pack == "")
	{
		title = logger::va("Mr. Modman | %s###main", menus::current_game.name.c_str());
	}
	else if (menus::current_game.name != "" && menus::current_game.pack != "")
	{
		title = logger::va("Mr. Modman | %s, %s###main", menus::current_game.name.c_str(), menus::current_game.pack.c_str());
	}

	if (ImGui::Begin(title.c_str(), nullptr, flags))
	{
		menus::menu_bar();
		menus::console();
		ImGui::End();
	}

	menus::new_game();
	menus::new_pack();
	menus::mods();
}

void menus::menu_bar()
{
	if (ImGui::BeginMenuBar())
	{
		menus::file();

		if (menus::current_game.name != "")
		{
			menus::packs();
		}

		if (menus::current_game.name != "" && menus::current_game.pack != "")
		{
			if (ImGui::Button("Mods"))
			{
				//Setup temp and reset pack_mods
				auto temp = fs::get_all_dirs(fs::get_pref_dir().append("mods\\" + menus::current_game.name + "\\" + menus::current_game.pack), "|f");
				menus::pack_mods = temp;

				if (menus::pack_mods.size() >= 1)
				{
					menus::pack_mods.emplace_back("||");
				}

				temp = fs::get_all_files(fs::get_pref_dir().append("mods\\" + menus::current_game.name + "\\" + menus::current_game.pack));
				menus::pack_mods.insert(menus::pack_mods.end(), temp.begin(), temp.end());
				//End

				//Global Mods
				temp = fs::get_all_dirs(fs::get_pref_dir().append("mods\\" + menus::current_game.name + "\\_global"), "|f");
				menus::global_mods = temp;

				if (menus::global_mods.size() >= 1)
				{
					menus::global_mods.emplace_back("||");
				}

				temp = fs::get_all_files(fs::get_pref_dir().append("mods\\" + menus::current_game.name + "\\_global"));
				menus::global_mods.insert(menus::global_mods.end(), temp.begin(), temp.end());
				//End

				temp.clear();

				menus::show_mods = !menus::show_mods;
			}

			if (ImGui::Button("Play"))
			{
				STARTUPINFOA startup_info;
				PROCESS_INFORMATION process_info;

				memset(&startup_info, 0, sizeof(startup_info));
				memset(&process_info, 0, sizeof(process_info));
				startup_info.cb = sizeof(startup_info);

				//For compatibility with old INIs
				if (menus::current_game.cwd.back() == '\\')
				{
					menus::current_game.cwd = menus::current_game.cwd.erase(menus::current_game.cwd.size() - 1, 1);
				}

				CreateProcessA
				(
					fs::get_cur_dir().append("loader.exe").c_str(),
					logger::va
					(
						"--exe \"%s\" --cwd \"%s\" --game \"%s\" --modpack \"%s\"",
						menus::current_game.path.c_str(),
						menus::current_game.cwd.c_str(),
						menus::current_game.name.c_str(),
						menus::current_game.pack.c_str()
					).data(),
					nullptr,
					nullptr,
					false,
					0,
					nullptr,
					fs::get_cur_dir().c_str(),
					&startup_info,
					&process_info
				);	
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

		ImGui::NewLine();

		if (ImGui::Button("Open Directory"))
		{
			fs::open_folder(fs::get_pref_dir());
		}

		if (menus::current_game.name != "")
		{
			menus::set_default();
			menus::spacer();
			menus::delete_game();
		}

		menus::spacer();

		if (ImGui::Button("Minimize"))
		{
			SDL_MinimizeWindow(global::window);
		}

		if(ImGui::Button("Exit")) global::shutdown = true;
		ImGui::EndMenu();
	}
}

void menus::packs()
{
	if (ImGui::BeginMenu("Packs"))
	{
		if (ImGui::Button("New Pack")) menus::show_new_packs = true;
		menus::load_pack();

		if (menus::current_game.name != "" && menus::current_game.pack != "")
		{
			menus::spacer();
			menus::delete_pack();
		}

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
	}
	ImGui::EndChild();
}

void menus::new_game()
{
	if (menus::show_new_game)
	{
		ImGuiWindowFlags ng_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

		static ImVec2 size = { 400, 300 };
		ImGui::SetNextWindowPos({ (global::resolution.x / 2) - (size.x / 2), (global::resolution.y / 2) - (size.y / 2) }, ImGuiCond_FirstUseEver);
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

			//ImGui::Text("Custom Mod Directory:");
			//ImGui::SameLine();
			//ImGui::Checkbox("##custom_dir", &menus::use_custom_dir);

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
			else if (!menus::custom_dir_buffer)
			{
				menus::clear_buffer(menus::custom_dir_buffer, sizeof(menus::custom_dir_buffer));
			}

			ImGui::SetCursorPos({ size.x - 100, size.y - 35 });
			if (ImGui::Button("Close##game"))
			{
				menus::show_new_game = false;
				menus::clear_buffer(menus::game_name_buffer, sizeof(menus::game_name_buffer));
				menus::clear_buffer(menus::game_path_buffer, sizeof(menus::game_path_buffer));
				menus::clear_buffer(menus::custom_dir_buffer, sizeof(menus::custom_dir_buffer));
				menus::use_custom_dir = false;
			}
			ImGui::SameLine();
			if (ImGui::Button("Finish##game"))
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

				menus::current_game = { name, menus::game_path_buffer, game_cwd };

				logger::log_info(logger::va("Finished setup for %s!", menus::game_name_buffer));
				settings::update();

				menus::show_new_game = false;
				menus::clear_buffer(menus::game_name_buffer, sizeof(menus::game_name_buffer));
				menus::clear_buffer(menus::game_path_buffer, sizeof(menus::game_path_buffer));
				menus::clear_buffer(menus::custom_dir_buffer, sizeof(menus::custom_dir_buffer));
				menus::use_custom_dir = false;

			}
			ImGui::End();
		}
	}
}

void menus::load_game(const std::string& game_name)
{
	if (game_name == "")
	{
		if (ImGui::BeginMenu("Load Game"))
		{
			for (auto game : menus::games)
			{
				if (ImGui::Button(game.c_str()))
				{
					ini_t* ini = ini_load(fs::get_pref_dir().append(logger::va("mods\\%s\\config.ini", game.c_str())).c_str());

					std::string path = ini_get(ini, "game", "path");

					menus::current_game =
					{
						game,
						path,
						ini_get(ini, "game", "cwd"),
						"",
						fs::get_all_dirs(fs::get_pref_dir().append(logger::va("mods\\%s", game.c_str())))
					};

					logger::log_info(logger::va("%s (%i packs) loaded!", menus::current_game.name.c_str(), menus::current_game.packs.size() - 1));
				}
			}

			ImGui::EndMenu();
		}
	}
	else
	{
		std::string ini_file = fs::get_pref_dir().append("mods\\" + game_name + "\\config.ini");

		if (fs::exists(ini_file))
		{
			ini_t* ini = ini_load(ini_file.c_str());

			std::string path = ini_get(ini, "game", "path");

			menus::current_game =
			{
				game_name,
				path,
				ini_get(ini, "game", "cwd"),
				"",
				fs::get_all_dirs(fs::get_pref_dir().append(logger::va("mods\\%s", game_name.c_str())))
			};

			logger::log_info(logger::va("Default game %s (%i packs) loaded!", menus::current_game.name.c_str(), menus::current_game.packs.size() - 1));
		}
		else
		{
			logger::log_error(logger::va("Default game %s not loaded! Unable to find path. Resetting default game.", menus::current_game.name.c_str(), menus::current_game.packs.size() - 1));
		}
	}
}

void menus::set_default()
{
	if (ImGui::Button(logger::va("Open %s On Startup", menus::current_game.name.c_str()).c_str()))
	{
		ini_t* config = ini_load(settings::config_file.c_str());
		ini_set(config, "core", "default_game", menus::current_game.name.c_str());
		ini_save(config, settings::config_file.c_str());
		ini_free(config);
	}
}

void menus::delete_game()
{
	if (ImGui::Button(logger::va("Delete %s", menus::current_game.name.c_str()).c_str()))
	{
		fs::del(fs::get_pref_dir().append("mods/" + menus::current_game.name), true);
		menus::show_mods = false;
		menus::games.erase(std::find(menus::games.begin(), menus::games.end(), menus::current_game.name));
		menus::current_game = {};
	}
}

void menus::delete_pack()
{
	if (ImGui::Button(logger::va("Delete %s", menus::current_game.pack.c_str()).c_str()))
	{
		fs::del(fs::get_pref_dir().append("mods/" + menus::current_game.name + "/" + menus::current_game.pack), true);
		menus::show_mods = false;
		menus::current_game.packs.erase(std::find(menus::current_game.packs.begin(), menus::current_game.packs.end(), menus::current_game.pack));
		menus::current_game.pack = "";
	}
}

void menus::new_pack()
{
	if (menus::show_new_packs)
	{
		ImGuiWindowFlags np_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse;

		static ImVec2 size = { 400, 300 };
		ImGui::SetNextWindowPos({ (global::resolution.x / 2) - (size.x / 2), (global::resolution.y / 2) - (size.y / 2) }, ImGuiCond_FirstUseEver);
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin(logger::va("New Pack For %s", menus::current_game.name.c_str()).c_str(), nullptr, np_flags))
		{
			ImGui::Text("Pack Name:");
			ImGui::SameLine();
			ImGui::InputText("##game_name", menus::pack_name_buffer, sizeof(menus::pack_name_buffer));

			ImGui::SetCursorPos({ size.x - 100, size.y - 35 });
			if (ImGui::Button("Close##pack"))
			{
				menus::show_new_packs = false;
				menus::clear_buffer(menus::pack_name_buffer, sizeof(menus::pack_name_buffer));
			}
			ImGui::SameLine();
			if (ImGui::Button("Finish##pack"))
			{
				std::string name = menus::pack_name_buffer;
				std::string path = fs::get_pref_dir().append(logger::va("mods\\%s\\%s", menus::current_game.name.c_str(), menus::pack_name_buffer));
				logger::log_warning(path);

				if (std::strlen(menus::pack_name_buffer) <= 0)
				{
					logger::log_error(logger::va("Pack Name invalid. Did you insert a pack name?"));
					ImGui::End();
					return;
				}

				if (fs::exists(path))
				{
					logger::log_error(logger::va("Pack \"%s\" alrady exists! Please change the name of your pack.", path.c_str()));
					ImGui::End();
					return;
				}

				fs::mkdir(path);
				menus::current_game.pack = name;
				menus::current_game.packs.emplace_back(menus::current_game.pack);

				logger::log_info(logger::va("Finished setup for pack %s!", menus::pack_name_buffer));

				menus::show_new_packs = false;
				menus::clear_buffer(menus::pack_name_buffer, sizeof(menus::pack_name_buffer));

			}
			ImGui::End();
		}
	}
}

void menus::load_pack()
{
	if (ImGui::BeginMenu("Load Pack"))
	{
		for (auto pack : menus::current_game.packs)
		{
			if (pack.compare("_global") && ImGui::Button(pack.c_str()))
			{
				menus::current_game.pack = pack;
				menus::show_mods = false;
				logger::log_info(logger::va("Pack %s loaded!", pack.c_str()));
			}
		}

		ImGui::EndMenu();
	}
}

void menus::mods()
{
	if (menus::show_mods)
	{
		ImGuiWindowFlags mods_flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_MenuBar;
		ImVec2 size = {350, 400};
		ImVec2 prev_pos;

		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin("Global Mods", nullptr, mods_flags))
		{
			prev_pos = ImGui::GetWindowPos();

			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File##global"))
				{
					if(ImGui::Button("Open Directory##global"))
					{
						fs::open_folder(fs::get_pref_dir().append("mods\\" + menus::current_game.name + "\\_global\\"));
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			int count = 0;
			for (auto mod : menus::global_mods)
			{
				if (mod == "||")
				{
					menus::spacer();
					continue;
				}

				bool folder = false;
				if (mod.find("|f") != std::string::npos)
				{
					mod = mod.erase(mod.size() - 2, 2);
					folder = true;
				}

				ImGui::Text(mod.c_str());
				ImGui::SameLine();
				ImGui::SetCursorPosX(size.x - 25);

				if (folder)
				{
					ImGui::SetCursorPosX(size.x - 53);

					if (ImGui::Button(logger::va("...##%s_pack", mod.c_str()).c_str()))
					{
						std::string file = fs::get_pref_dir().append("mods/" + menus::current_game.name + "/" + menus::current_game.pack + "/" + mod);
						fs::open_folder(file);
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Click to open...");
						ImGui::EndTooltip();
					}

					ImGui::SameLine();
				}
				else
				{
					for (auto ext : menus::settings_exts)
					{
						if (logger::ends_with(mod, ext))
						{
							ImGui::SetCursorPosX(size.x - 47);

							if (ImGui::Button(logger::va("S##%s_global", mod.c_str()).c_str()))
							{
								std::string file = fs::get_pref_dir().append("mods/" + menus::current_game.name + "/_global/" + mod);
								fs::open_editor(file);
							}

							if (ImGui::IsItemHovered())
							{
								ImGui::BeginTooltip();
								ImGui::Text("Click to editt...");
								ImGui::EndTooltip();
							}

							ImGui::SameLine();
						}
					}
				}

				if (ImGui::Button(logger::va("X##%s_global", mod.c_str()).c_str()))
				{
					fs::del(fs::get_pref_dir().append("mods/" + menus::current_game.name + "/_global/" + mod));
					menus::global_mods.erase(menus::global_mods.begin() + count);
				}
				count++;
			}
			ImGui::End();
		}

		ImGui::SetNextWindowPos({ prev_pos.x + size.x, prev_pos.y }, ImGuiCond_Appearing);
		ImGui::SetNextWindowSize(size);
		if (ImGui::Begin("Pack Mods", nullptr, mods_flags))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File##pack"))
				{
					if (ImGui::Button("Open Directory##pack"))
					{
						fs::open_folder(fs::get_pref_dir().append("mods\\" + menus::current_game.name + "\\" + menus::current_game.pack + "\\"));
					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			int count = 0;
			for (auto mod : menus::pack_mods)
			{
				if (mod == "||")
				{
					menus::spacer();
					continue;
				}

				bool folder = false;
				if (mod.find("|f") != std::string::npos)
				{
					mod = mod.erase(mod.size() - 2, 2);
					folder = true;
				}

				ImGui::Text(mod.c_str());
				ImGui::SameLine();
				ImGui::SetCursorPosX(size.x - 25);

				if (folder)
				{
					ImGui::SetCursorPosX(size.x - 53);

					if (ImGui::Button(logger::va("...##%s_pack", mod.c_str()).c_str()))
					{
						std::string file = fs::get_pref_dir().append("mods/" + menus::current_game.name + "/" + menus::current_game.pack + "/" + mod);
						fs::open_folder(file);
					}

					if (ImGui::IsItemHovered())
					{
						ImGui::BeginTooltip();
						ImGui::Text("Click to open...");
						ImGui::EndTooltip();
					}

					ImGui::SameLine();
				}
				else
				{
					for (auto ext : menus::settings_exts)
					{
						if (logger::ends_with(mod, ext))
						{
							ImGui::SetCursorPosX(size.x - 47);

							if (ImGui::Button(logger::va("S##%s_pack", mod.c_str()).c_str()))
							{
								std::string file = fs::get_pref_dir().append("mods/" + menus::current_game.name + "/" + menus::current_game.pack + "/" + mod);
								fs::open_editor(file);
							}

							if (ImGui::IsItemHovered())
							{
								ImGui::BeginTooltip();
								ImGui::Text("Click to edit...");
								ImGui::EndTooltip();
							}

							ImGui::SameLine();
						}
					}
				}

				if (ImGui::Button(logger::va("X##%s_pack", mod.c_str()).c_str()))
				{
					std::string file = fs::get_pref_dir().append("mods/" + menus::current_game.name + "/" + menus::current_game.pack + "/" + mod);
					fs::del(file);
					menus::pack_mods.erase(menus::pack_mods.begin() + count);
				}
				count++;
			}
			ImGui::End();
		}
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

void menus::spacer()
{
	ImGui::Separator();
	ImGui::NewLine();
}

char menus::game_path_buffer[MAX_PATH];
char menus::game_name_buffer[32];
char menus::pack_name_buffer[32];
bool menus::use_custom_dir = false;
char menus::custom_dir_buffer[MAX_PATH];

bool menus::show_new_game = false;
bool menus::show_new_packs = false;
bool menus::show_load_packs = false;
bool menus::show_mods = false;

std::vector<std::string> menus::global_mods;
std::vector<std::string> menus::pack_mods;

std::initializer_list<std::string> menus::settings_exts = {".ini", ".cfg"};

std::vector<std::string> menus::console_output;
std::vector<std::string> menus::games;
game_t menus::current_game;

std::string menus::default_game = "";

color_t menus::background_col = { 30, 30, 30, 255 };
