#include "global.hpp"

#include "logger/logger.hpp"
#include "fs/fs.hpp"

#include "settings.hpp"
#include "menus/menus.hpp"

void settings::init()
{
	settings::update();
}

void settings::update()
{
	ini_t* config;

	if (fs::exists(settings::config_file))
	{
		config = ini_load(settings::config_file.c_str());

		if (strcmp(ini_get(config, "core", "version"), VERSION))
		{
			fs::del(settings::config_file);
			settings::update();
			return;
		}
	}
	else if (!fs::exists(settings::config_file))
	{
		std::string ini_default = logger::va(
			"[core]\n"
			"version = \"%s\"\n\n"

			"[colors]\n"
			"background_r = \"30\"\n"
			"background_g = \"30\"\n"
			"background_b = \"30\"\n"
			"background_a = \"255\"\n\n", VERSION);

		config = ini_create(ini_default.c_str(), strlen(ini_default.c_str()));

		ini_save(config, settings::config_file.c_str());
	}

	menus::background_col =
	{
		std::stoi(ini_get(config, "colors", "background_r")),
		std::stoi(ini_get(config, "colors", "background_g")),
		std::stoi(ini_get(config, "colors", "background_b")),
		std::stoi(ini_get(config, "colors", "background_a")),
	};

	ini_free(config);

	menus::games = fs::get_all_dirs(fs::get_pref_dir().append("mods\\"));
	std::sort(menus::games.begin(), menus::games.end());
}

bool settings::get_boolean(const char* bool_text)
{
	if (!std::strcmp(bool_text, "true")) return true;
	else return false;
}

std::string settings::config_file = logger::va("%s%s", fs::get_pref_dir().c_str(), "config.ini");
