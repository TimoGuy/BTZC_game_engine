cmake_minimum_required(VERSION 3.29 FATAL_ERROR)

# Project.
project(btzc_game_engine
    VERSION 0.1.0
    LANGUAGES CXX C)

# When turning this option on, the library will be compiled using assertions. By default asserts are enabled in Debug build.
option(ENABLE_PROFILING "Enable profiling" OFF)

# When turning this option on, the logging version of the FMOD library will be linked.
option(ENABLE_FMOD_LOGGING "Enable FMOD logging" OFF)

# Print out all options.
message(
    "\n"
    "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n"
    "|               ::OPTION VALUES::                \n"
    "| ENABLE_PROFILING    : ${ENABLE_PROFILING}\n"
    "| ENABLE_FMOD_LOGGING : ${ENABLE_FMOD_LOGGING}\n"
    "-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-\n")

# App icon.
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    enable_language("RC")
    set(WIN64_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/win64/app_icon.rc)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    message(WARNING "MACOS APP ICON STUFF: https://stackoverflow.com/questions/22745728/cmake-set-icon-for-a-mac-os-x-app")
else()
    message(FATAL_ERROR "Asset folder location must be defined in `ASSET_DIR`")
endif()

# C/C++ standard.
# set(CMAKE_UNITY_BUILD ON)  # @TODO: get this working at some point!!
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # MacOS requires enabling experimental library for clang-17 (default).
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexperimental-library")
endif()

# Ensure 64-bit build.
if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Only 64-bit build allowed.")
    return()
endif()

# Multithreaded build.
set(CMAKE_BUILD_PARALLEL_LEVEL 16)
# if(MSVC)  # @NOTE: this can cause errors so don't do this lol.  -Thea 2026/02/14
#     set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
# endif()

# Profiling.
if(ENABLE_PROFILING)
    message(FATAL_ERROR "@TODO: Implement profiling libraries here!")
endif()

# Assets directory.
set(ASSET_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/assets)

# Check certain env vars exist before dependency list.
if(NOT DEFINED ENV{FMOD_STUDIO_API_DIR})
    message(FATAL_ERROR "Env var \"FMOD_STUDIO_API_DIR\" is not defined. Set it to the base dir of the FMOD API and retry.")
endif()

# Dependencies.
set(DEPENDENCY_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cpp_glm/include  # Header only.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/entt/src  # Header only.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt/include
    $ENV{FMOD_STUDIO_API_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/thea_imgui  # Header only.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/JoltPhysics
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb  # Header only (essentially).
    # ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stduuid/include  # @NOTE: Removing this so that it's not easy to mix up the uuid.h header files.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tomlplusplus/include  # Header only.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TXP_renderer/include
)

add_subdirectory(third_party/fmt)
set(DOUBLE_PRECISION ON CACHE BOOL "Use double precision math" FORCE)  # Use double precision for large, open world physics.
add_subdirectory(third_party/JoltPhysics/Build)
add_subdirectory(third_party/json)
add_subdirectory(third_party/stduuid)
add_subdirectory(third_party/TXP_renderer)

# Create symlink to assets folder.
if(DEFINED ASSET_DIR)
    set(dir_name ${ASSET_DIR})
    set(target_dir_name ${CMAKE_BINARY_DIR}/assets)
    if(WIN32)
        string(REPLACE "/" "\\" win32_dir_name ${dir_name})
        string(REPLACE "/" "\\" win32_target_dir_name ${target_dir_name})
        if(EXISTS ${dir_name})
            execute_process(
                COMMAND cmd /C rmdir ${win32_target_dir_name}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        endif()
        execute_process(
            COMMAND cmd /C mklink /J ${win32_target_dir_name} ${win32_dir_name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    else()
        # The way you create a symlink in every single OS except Windows.s
        file(CREATE_LINK ${dir_name} ${target_dir_name} SYMBOLIC)
    endif()
endif()


# Resource files inside ASSET_DIR.
set(ASSET_DIR_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/anim_frame_actions/simple_combat_char.btafa
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/animator_templates/simple_combat_char.btanitor
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/Leever.glb
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/SlimeGirl.glb
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/box_0.5_2.mtl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/cylinder_0.5_2.mtl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/player_model_0.5_2.mtl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/probuilder_example.mtl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/probuilder_example.wobj
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/rail_line_editor_gizmo.mtl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/rail_line_editor_gizmo.wobj
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/rails.glb
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/rails.mtl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/rails.wobj
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/simple_combat_char.glb
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/stupid_polygon_grid.mtl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/unit_box.mtl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/models/unit_box.wobj
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/scenes/_dev_animation_editor_view OLD COPY.btscene
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/scenes/_dev_animation_editor_view.btscene
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/scenes/_dev_sample_scene OLD COPY.btscene
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/scenes/_dev_sample_scene.btscene
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/__debug_color_grad_line.shader
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/__debug_color_grad_line.shadrefl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/__debug_color_wireframe.shader
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/__debug_color_wireframe.shadrefl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/basic_diffuse.shader
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/basic_diffuse.shadrefl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/default_shader.shader
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/default_shader.shadrefl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/gradient.shader
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/gradient.shadrefl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/skinned_model.shader
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/shaders/skinned_model.shadrefl
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/textures/test_ktx_tex.ktx2
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/ui_canvases/jojo.btui
    ${CMAKE_CURRENT_SOURCE_DIR}/assets/unnamed.mtl
)

# macOS: add all files in assets dir into app bundle package.
if (TXP_RENDERER_STANDALONE)
    foreach (FILE ${ASSET_DIR_FILES})
        file(RELATIVE_PATH NEW_FILE "${ASSET_DIR}" ${FILE})
        get_filename_component(NEW_FILE_PATH ${NEW_FILE} DIRECTORY)
        set_source_files_properties(${FILE}
            PROPERTIES
                MACOSX_PACKAGE_LOCATION "Resources/assets/${NEW_FILE_PATH}"
        )
    endforeach()
endif()


# Main source files.
set(MAIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio_engine/audio_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio_engine/audio_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio_engine/audio_impl_fmod.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio_engine/audio_impl_fmod.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/audio_engine/util.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btdatecheck.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btdatecheck.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btglm.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btjson.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btjson.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btlogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btlogger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btuuid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btuuid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/btzc_game_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/anim_frame_action_controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/character_movement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/character_movement.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/combat_stats.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/combat_stats.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/component_imgui_edit_functions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/component_imgui_edit_functions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/component_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/component_registry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/cpu_enemy_awareness.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/follow_camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/health_stats.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/physics_object_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/physics_object_settings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/rail_line.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/rail_line.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/transform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/transform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/entity_container.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/entity_container.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/animator_driven_hitcapsule_sets_update.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/animator_driven_hitcapsule_sets_update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/character_broadcast_attack_msg_to_enemies.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/character_broadcast_attack_msg_to_enemies.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/cpu_character_enemy_detection.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/cpu_character_enemy_detection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/cpu_character_world_space_input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/cpu_character_world_space_input.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/follow_camera_position_update.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/follow_camera_position_update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/helper_funcs.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/helper_funcs.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/hitcapsule_attack_processing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/hitcapsule_attack_processing.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/imgui_render_transform_hierarchy_window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/imgui_render_transform_hierarchy_window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/input_controlled_character_movement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/input_controlled_character_movement.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/player_character_lock_onto_target.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/player_character_lock_onto_target.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/player_character_world_space_input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/player_character_world_space_input.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/process_physics_object_lifetime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/process_physics_object_lifetime.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/propagate_changed_transforms.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/propagate_changed_transforms.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/rail_line_editor_update.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/rail_line_editor_update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/rail_line_rider_update.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/rail_line_rider_update.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/tick_sim_char_mvt_animator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/tick_sim_char_mvt_animator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/write_entity_transforms_from_physics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/write_entity_transforms_from_physics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/write_render_transforms.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/write_render_transforms.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/world/scene_loader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/world/scene_loader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/world/scene_serialization.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/world/scene_serialization.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/world/world_properties.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/world/world_properties.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/brush.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/entity.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/entity_geometry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/face.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/geo_generator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/geo_generator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/libmap.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/libmap_math.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/map_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/map_data.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/map_parser.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/map_parser.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/matrix.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/matrix.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/platform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/surface_gatherer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/surface_gatherer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/vector.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/libmap_cpp/src/vector.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/map_importer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/map_importer/map_importer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl_custom_listeners.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl_error_callbacks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl_layers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl_obj_vs_broad_phase_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object_impl_char_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object_impl_char_controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object_impl_tri_mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object_impl_tri_mesh.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/raycast_helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/raycast_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service_finder/service_finder.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/service_finder/service_finder.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/settings/settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/settings/settings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/timer/timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/timer/timer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/timer/watchdog_timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/timer/watchdog_timer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uuid/uuid_ifc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uuid/uuid_ifc.h
)

# Executable build.
add_executable(${PROJECT_NAME}
    # Main source files.
    ${MAIN_SOURCES}

    # stb header files.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/stb_image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/stb_image_resize2.h

    # Native extras.
    ${WIN64_RESOURCES}

    # Resource files.
    ${ASSET_DIR_FILES}
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib
        ${DEPENDENCY_INCLUDE_DIRS})

# Build FMOD link library path.
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    if(ENABLE_FMOD_LOGGING)
        set(FMOD_LIB_FILE_PREFIX fmodL)
    else()
        set(FMOD_LIB_FILE_PREFIX fmod)
    endif()
    string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} SYS_PROCESSOR_LOWER)
    if(${SYS_PROCESSOR_LOWER} MATCHES "arm64|aarch64")
        set(FMOD_LIBRARY_PATH
            $ENV{FMOD_STUDIO_API_DIR}/api/core/lib/arm64/)
    else()
        set(FMOD_LIBRARY_PATH
            $ENV{FMOD_STUDIO_API_DIR}/api/core/lib/x64/)
    endif()
    set(FMOD_LIBRARIES
        ${FMOD_LIBRARY_PATH}${FMOD_LIB_FILE_PREFIX}_vc.lib)
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(ENABLE_FMOD_LOGGING)
        set(FMOD_LIB_FILE_PREFIX libfmodL)
    else()
        set(FMOD_LIB_FILE_PREFIX libfmod)
    endif()

    file(REAL_PATH $ENV{FMOD_STUDIO_API_DIR} FMOD_STUDIO_API_DIR_ABS_PATH EXPAND_TILDE)

    set(FMOD_LIBRARY_PATH
        ${FMOD_STUDIO_API_DIR_ABS_PATH}/api/core/lib/)
    set(FMOD_DYLIB_PATH
        ${FMOD_LIBRARY_PATH}${FMOD_LIB_FILE_PREFIX}.dylib)

    # Create an imported library target for FMOD.
    set(FMOD_LIBRARIES btzc_fmod_dylib)
    add_library(${FMOD_LIBRARIES} SHARED IMPORTED)
    set_target_properties(${FMOD_LIBRARIES} PROPERTIES
        IMPORTED_LOCATION ${FMOD_DYLIB_PATH}
    )
else()
    message(FATAL_ERROR "Building FMOD link lib path missing!")
endif()

message("FMOD lib path: ${FMOD_LIBRARIES}")

target_link_libraries(${PROJECT_NAME}
    txp_renderer
    ${FMOD_LIBRARIES}
    fmt::fmt
    Jolt
    nlohmann_json::nlohmann_json
    stduuid
)

# Copy dynamic libs.
# @TODO: check to see if instead of copying the dlls, maybe you can use the same shared-imported trick as above???
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${FMOD_LIBRARY_PATH}${FMOD_LIB_FILE_PREFIX}.dll
            ${CMAKE_CURRENT_BINARY_DIR})
elseif(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    # message(WARNING "MACOS APP DYNA-lib stuff (TODO: RESEARCH!!!)")
else()
    message(FATAL_ERROR "Copying dynamic libs missing!!")
endif()

# Create symlink to assets folder.
if(DEFINED ASSET_DIR)
    set(dir_name ${ASSET_DIR})
    set(target_dir_name ${CMAKE_BINARY_DIR}/assets)
    if(WIN32)
        string(REPLACE "/" "\\" win32_dir_name ${dir_name})
        string(REPLACE "/" "\\" win32_target_dir_name ${target_dir_name})
        if(EXISTS ${dir_name})
            execute_process(
                COMMAND cmd /C rmdir ${win32_target_dir_name}
                WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
        endif()
        execute_process(
            COMMAND cmd /C mklink /J ${win32_target_dir_name} ${win32_dir_name}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    else()
        # The way you create a symlink in every single OS except Windows.s
        file(CREATE_LINK ${dir_name} ${target_dir_name} SYMBOLIC)
    endif()
else()
    message(FATAL_ERROR "Asset folder location must be defined in `ASSET_DIR`. Unable to create symlink.")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER "com.hawsoo.btzc_game_engine"
    MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
    MACOSX_BUNDLE_BUNDLE_VERSION "0.1.0"
    MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
    # RESOURCE "${ASSET_DIR_FILES}"  @CHECK: commented these out since it caused resource files to get placed at the root of the bundle. So it should be good enough if changes?  -Thea 2026/02/08
)

# Set this project as Visual Studio startup project.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
