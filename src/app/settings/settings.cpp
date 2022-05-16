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
	if(fs::exists(settings::config_file)) settings::config = ini_load(settings::config_file.c_str());
	else if (!fs::exists(settings::config_file))
	{
		const char* ini_default =
			"";

		settings::config = ini_create(ini_default, strlen(ini_default));

		ini_save(settings::config, settings::config_file.c_str());
	}

	ini_free(settings::config);

	menus::games = fs::get_all_dirs(fs::get_pref_dir().append("mods\\"));
}

bool settings::get_boolean(const char* bool_text)
{
	if (!std::strcmp(bool_text, "true")) return true;
	else return false;
}

std::string settings::config_file = logger::va("%s%s", fs::get_pref_dir().c_str(), "config.ini");
ini_t* settings::config;
