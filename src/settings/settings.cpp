////////////////////////////////////////////////////////////////////////////////////////////////////
/// @copyright (c) 2025 Thea Bennett
/// @brief Struct of settings for the app. Can be mutated and then saved to disk.
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "settings.h"

#include "btlogger.h"
#include "toml++/toml.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>


namespace
{

using namespace BT;

constexpr char const* const k_settings_fname{ "settings.toml" };

static App_settings* s_app_settings{ nullptr };

/// Converts TOML file contents to app settings, only changing values that exist in the file.
void toml_to_app_settings(toml::table const& toml_tbl, App_settings& app_settings)
{
    app_settings.window_settings.monitor_idx     = toml_tbl["window_settings"]["monitor_idx"].value_or(app_settings.window_settings.monitor_idx);
    app_settings.window_settings.windowed_width  = toml_tbl["window_settings"]["windowed_width"].value_or(app_settings.window_settings.windowed_width);
    app_settings.window_settings.windowed_height = toml_tbl["window_settings"]["windowed_height"].value_or(app_settings.window_settings.windowed_height);
    app_settings.window_settings.is_resizable    = toml_tbl["window_settings"]["is_resizable"].value_or(app_settings.window_settings.is_resizable);
    app_settings.window_settings.has_border      = toml_tbl["window_settings"]["has_border"].value_or(app_settings.window_settings.has_border);
    app_settings.window_settings.is_maximized    = toml_tbl["window_settings"]["is_maximized"].value_or(app_settings.window_settings.is_maximized);
    app_settings.window_settings.is_fullscreen   = toml_tbl["window_settings"]["is_fullscreen"].value_or(app_settings.window_settings.is_fullscreen);
}

/// Converts app settings into TOML file, including all fields. Returns the TOML file.
toml::table app_settings_to_toml(App_settings& app_settings)
{
    return toml::table{
        { "window_settings", toml::table{
                { "monitor_idx",     app_settings.window_settings.monitor_idx  },
                { "windowed_width",  app_settings.window_settings.windowed_width  },
                { "windowed_height", app_settings.window_settings.windowed_height },
                { "is_resizable",    app_settings.window_settings.is_resizable    },
                { "has_border",      app_settings.window_settings.has_border      },
                { "is_maximized",    app_settings.window_settings.is_maximized    },
                { "is_fullscreen",   app_settings.window_settings.is_fullscreen   },
            }
        },
    };
}

}  // namespace


void BT::initialize_app_settings_from_file_or_fallback_to_defaults()
{   // Ensure doesn't run multiple times.
    assert(s_app_settings == nullptr);

    // Create default app settings.
    static App_settings s_main_app_settings;
    s_app_settings = &s_main_app_settings;

    // Try to load app settings from a file.
    if (std::filesystem::exists(k_settings_fname) &&
        std::filesystem::is_regular_file(k_settings_fname))
    {
        try
        {   // Load settings from disk and deserialize.
            toml::table tbl{ toml::parse_file(k_settings_fname) };
            toml_to_app_settings(tbl, s_main_app_settings);

            BT_TRACEF("Successfully loaded app settings from \"%s\"", k_settings_fname);
        }
        catch (toml::parse_error const& err)
        {
            BT_ERRORF("Parsing failed of file\"%s\": %s", k_settings_fname, err.what());
        }
    }
    else
    {   // Write default settings to disk.
        save_app_settings_to_disk();
    }
}

BT::App_settings const& BT::get_app_settings_read_handle()
{
    assert(s_app_settings != nullptr);
    return *s_app_settings;
}

BT::App_settings& BT::get_app_settings_write_handle()
{
    assert(s_app_settings != nullptr);
    return *s_app_settings;
}

void BT::save_app_settings_to_disk()
{
    auto toml_tbl{ app_settings_to_toml(*s_app_settings) };

    // Write to file.
    std::ofstream f{ k_settings_fname };
    f << toml_tbl << "\n";

    BT_TRACEF("Successfully wrote app settings to \"%s\"", k_settings_fname);
}
