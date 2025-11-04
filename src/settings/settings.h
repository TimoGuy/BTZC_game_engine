////////////////////////////////////////////////////////////////////////////////////////////////////
/// @copyright (c) 2025 Thea Bennett
/// @brief Struct of settings for the app. Can be mutated and then saved to disk.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once


namespace BT
{

/// For initializing settings.
void initialize_app_settings_from_file_or_fallback_to_defaults();


/// Forward decl.
struct App_settings;

/// Getting app settings data read handle.
App_settings const& get_app_settings_read_handle();
/// Getting app settings data write handle.
App_settings& get_app_settings_write_handle();

/// Serialize/saving data from struct to the settings file.
void save_app_settings_to_disk();


/// Global state of app settings.
struct App_settings
{


    // The vv below vv is for preventing others from instantiating the struct.
    friend void ::BT::initialize_app_settings_from_file_or_fallback_to_defaults();
private: App_settings() = default;
};

}  // namespace BT
