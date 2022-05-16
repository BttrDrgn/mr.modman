#pragma once

//Could maybe do some fancy stuff with this later
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
	static bool use_custom_dir;
	static char custom_dir_buffer[MAX_PATH];

	static std::vector<std::string> console_output;


private:
	static void build_font(ImGuiIO& io);
	static void clear_buffer(char* bufffer, size_t size);

	static ImVec4 rgba_to_col(float r, float g, float b, float a)
	{
		return ImVec4{r/255.0f, g/255.0f, b/255.0f, a/255.0f};
	}

	static void menu_bar();
	static void file();

	static void console();

	static bool show_new_game;
};