#pragma once

#define BTZC_GAME_ENGINE_VERSION        "v0.1.0-develop.2 WIP"
#define BTZC_GAME_ENGINE_DEV_STAGE      "PRE-ALPHA"

#if _WIN64
#define BTZC_GAME_ENGINE_OS_NAME        "Windows x64"
#elif _WIN32
#error "Windows x86 (32-bit) is unsupported."
#else
#error "Unknown, currently unsupported OS です"
#endif

// Paths.
#define BTZC_GAME_ENGINE_ASSET_MODEL_PATH     "assets/models/"
#define BTZC_GAME_ENGINE_ASSET_SHADER_PATH    "assets/shaders/"
#define BTZC_GAME_ENGINE_ASSET_TEXTURE_PATH   "assets/textures/"
#define BTZC_GAME_ENGINE_ASSET_SCENE_PATH     "assets/scenes/"
#define BTZC_GAME_ENGINE_ASSET_SETTINGS_PATH  "assets/settings/"
