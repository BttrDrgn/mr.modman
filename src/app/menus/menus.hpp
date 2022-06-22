#pragma once

struct game_t
{
	std::string name, path, cwd, pack;
	std::vector<std::string> packs;
};

struct color_t
{
	int r, g, b, a;
};

class menus
{
public:
	static void update();

	static void init();
	static void prepare();
	static void present();
	static void cleanup();

	static char game_path_buffer[MAX_PATH];
	static char game_name_buffer[32];
	static char pack_name_buffer[32];
	static bool use_custom_dir;
	static char custom_dir_buffer[MAX_PATH];

	static std::initializer_list<std::string> settings_exts;
	static std::vector<std::string> console_output;
	static std::vector<std::string> games;
	static std::vector<std::string> global_mods;
	static std::vector<std::string> pack_mods;
	static game_t current_game;

	static color_t background_col;

private:
	static void build_font(ImGuiIO& io);
	static void clear_buffer(char* bufffer, size_t size);

	static ImVec4 rgba_to_col(float r, float g, float b, float a)
	{
		return ImVec4{ r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f };
	}

	static void menu_bar();
	static void file();
	static void packs();

	static void load_game();
	static void delete_game();
	static void load_pack();
	static void delete_pack();

	static void new_game();
	static void new_pack();

	static void spacer();

	static void mods();

	static void console();

	static bool show_new_game;
	static bool show_new_packs;
	static bool show_load_packs;
	static bool show_mods;
};
