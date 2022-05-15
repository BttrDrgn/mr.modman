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

#ifndef OVERLAY
	ImGui_ImplSDL2_InitForSDLRenderer(global::window, global::renderer);
	ImGui_ImplSDLRenderer_Init(global::renderer);
#endif
}

void menus::prepare()
{
#ifndef OVERLAY
	ImGui_ImplSDLRenderer_NewFrame();
	ImGui_ImplSDL2_NewFrame();
	ImGui::NewFrame();
	SDL_SetRenderDrawColor(global::renderer, 30, 30, 30, 255);
	SDL_RenderClear(global::renderer);
#else
	switch (global::renderer)
	{
	case kiero::RenderType::D3D9:
		ImGui_ImplDX9_NewFrame();
		break;

	case kiero::RenderType::D3D10:
		ImGui_ImplDX10_NewFrame();
		break;

	case kiero::RenderType::D3D11:
		ImGui_ImplDX11_NewFrame();
		break;

	case kiero::RenderType::OpenGL:
		ImGui_ImplOpenGL3_NewFrame();
		break;
	}

	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
#endif
}

void menus::present()
{
#ifndef OVERLAY
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
#else
	ImGui::EndFrame();
	ImGui::Render();

	switch (global::renderer)
	{
	case kiero::RenderType::D3D9:
		ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
		break;

	case kiero::RenderType::D3D10:
		ImGui_ImplDX10_RenderDrawData(ImGui::GetDrawData());
		break;

	case kiero::RenderType::D3D11:
		ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
		break;

	case kiero::RenderType::OpenGL:
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
		break;
	}
#endif
}

void menus::cleanup()
{
#ifndef OVERLAY
	ImGui_ImplSDLRenderer_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyRenderer(global::renderer);
	SDL_DestroyWindow(global::window);
	SDL_Quit();
#endif
}

void menus::update()
{
#ifndef OVERLAY
	ImGui::SetNextWindowPos({ 0, 0 });
	ImGui::SetNextWindowSize(global::resolution);
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
		ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_MenuBar;
	if (ImGui::Begin("Mr. Modman", nullptr, flags))
	{
		menus::menu_bar();
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
				fs::browse(menus::game_path_buffer, "Executable (.exe)\0*.exe\0", "Select your game executable");
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
				ImGui::Text("By default, the mods will load from:\n%s", &path[0]);
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

				if (name != "")
				{
					ImGui::End();
					return;
				}

				if (!fs::exists(path))
				{
					logger::log_debug(logger::va("%s not found", &path[0]));
					ImGui::End();
					return;
				}

				menus::show_new_game = false;

				if (path == "")
				{
					logger::log_debug(path);

				}
				
				//fs::write();

				menus::clear_buffer(menus::game_name_buffer, sizeof(menus::game_name_buffer));
				menus::clear_buffer(menus::game_path_buffer, sizeof(menus::game_path_buffer));
				menus::clear_buffer(menus::custom_dir_buffer, sizeof(menus::custom_dir_buffer));
				menus::use_custom_dir = false;
			}
		}
		ImGui::End();
	}

#endif
}

void menus::menu_bar()
{
	if (ImGui::BeginMenuBar())
	{
		menus::file();
		ImGui::EndMenuBar();
	}
}

void menus::file()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::Button("New Game")) menus::show_new_game = true;
		ImGui::Button("Load Game");
		ImGui::Text("__________");
		if(ImGui::Button("Exit")) global::shutdown = true;
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
		io.Fonts->AddFontFromFileTTF(&font[0], 18.0f);

		static ImFontConfig cfg;
		static ImWchar emoji_ranges[] = { 0x1, 0x1FFFF, 0 };

		if (fs::exists(emoji))
		{
			cfg.MergeMode = true;
			cfg.OversampleH = cfg.OversampleV = 1;
			io.Fonts->AddFontFromFileTTF(&emoji[0], 12.0f, &cfg, emoji_ranges);
		}

		if (fs::exists(font_jp))
		{
			ImFontConfig cfg;
			cfg.OversampleH = cfg.OversampleV = 1;
			cfg.MergeMode = true;
			io.Fonts->AddFontFromFileTTF(&font_jp[0], 18.0f, &cfg, io.Fonts->GetGlyphRangesJapanese());
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