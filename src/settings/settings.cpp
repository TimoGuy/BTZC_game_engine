////////////////////////////////////////////////////////////////////////////////////////////////////
/// @copyright (c) 2025 Thea Bennett
/// @brief Struct of settings for the app. Can be mutated and then saved to disk.
////////////////////////////////////////////////////////////////////////////////////////////////////
#include "settings.h"
#include <cassert>
#include <filesystem>
#include <string>


namespace
{

using namespace BT;

constexpr char const* const k_settings_fname{ "settings.toml" };

static App_settings* s_app_settings{ nullptr };

}  // namespace


void BT::initialize_app_settings_from_file_or_fallback_to_defaults()
{   // Ensure doesn't run multiple times.
    assert(s_app_settings == nullptr);

    // Create default app settings.
    static App_settings s_main_app_settings;

    // Try to load app settings from a file.
    if (std::filesystem::exists(k_settings_fname) &&
        std::filesystem::is_regular_file(k_settings_fname))
    {   // Load settings from disk.
        assert(false);; // @TODO
    }
    else
    {   // Write default settings to disk.
        save_app_settings_to_disk();
    }
}

BT::App_settings const& BT::get_app_settings_read_handle()
{}

BT::App_settings& BT::get_app_settings_write_handle()
{}

void BT::save_app_settings_to_disk()
{

}
