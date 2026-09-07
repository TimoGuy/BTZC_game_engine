#pragma once

#define BTZC_GAME_ENGINE_VERSION        "v0.1.0-develop.2 WIP"
#define BTZC_GAME_ENGINE_DEV_STAGE      "PRE-ALPHA"

#if _WIN64
    #define BTZC_GAME_ENGINE_OS_NAME        "Windows x64"
#elif _WIN32
    #error "Windows x86 (32-bit) is unsupported."
#elif __APPLE__
    #if __LP64__
        #define BTZC_GAME_ENGINE_OS_NAME    "MacOS x64"
    #else
        #error "MacOS 32-bit is unsupported."
    #endif
#else
    #error "Unknown, currently unsupported OS です"
#endif

/// Paths.
#define BTZC_GAME_ENGINE_ASSET_ANIM_FRAME_ACTIONS_PATH  "assets/anim_frame_actions/"
#define BTZC_GAME_ENGINE_ASSET_ANIMATOR_TEMPLATES_PATH  "assets/animator_templates/"
#define BTZC_GAME_ENGINE_ASSET_AUDIO_PATH               "assets/audio/"
#define BTZC_GAME_ENGINE_ASSET_MODEL_PATH               "assets/models/"
#define BTZC_GAME_ENGINE_ASSET_UI_PATH                  "assets/ui_canvases/"
#define BTZC_GAME_ENGINE_ASSET_SHADER_PATH              "assets/shaders/"
#define BTZC_GAME_ENGINE_ASSET_TEXTURE_PATH             "assets/textures/"
#define BTZC_GAME_ENGINE_ASSET_SCENE_PATH               "assets/scenes/"
#define BTZC_GAME_ENGINE_ASSET_SETTINGS_PATH            "assets/settings/"

/// Settings.
#define BTZC_GAME_ENGINE_SETTING_ENTITY_POOL_POOL_SIZE  65536

#ifdef JPH_DOUBLE_PRECISION
#define BTZC_GAME_ENGINE_SETTING_REAL_TYPE_USES_DBL_PRECISION 1
#else
#define BTZC_GAME_ENGINE_SETTING_REAL_TYPE_USES_DBL_PRECISION 0
#endif 
