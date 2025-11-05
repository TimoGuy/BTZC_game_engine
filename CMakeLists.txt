cmake_minimum_required(VERSION 3.28)

# Project.
project(btzc_game_engine
    VERSION 0.1.0
    LANGUAGES CXX C)

# App icon.
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    enable_language("RC")
    set(WIN64_RESOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/win64/app_icon.rc)
else()
    message(FATAL_ERROR "Asset folder location must be defined in `ASSET_DIR`")
endif()

# C/C++ standard.
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if(MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# Multithreaded build.
set(CMAKE_BUILD_PARALLEL_LEVEL 16)
if(MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /MP")
endif()

# Assets directory.
set(ASSET_DIR
    ${CMAKE_CURRENT_SOURCE_DIR}/assets)

# Dependencies.
set(DEPENDENCY_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/cglm/include  # Header only.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/entt/src  # Header only.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fastgltf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui  # Header only.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ImGuizmo
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/JoltPhysics
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/json/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb  # Header only (essentially).
    # ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stduuid/include  # @NOTE: Removing this so that it's not easy to mix up the uuid.h header files.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tomlplusplus/include  # Header only.
)

add_subdirectory(third_party/fastgltf)
add_subdirectory(third_party/fmt)
add_subdirectory(third_party/glfw)
set(DOUBLE_PRECISION ON CACHE BOOL "Use double precision math" FORCE)  # Use double precision for large, open world physics.
add_subdirectory(third_party/JoltPhysics/Build)
add_subdirectory(third_party/json)
add_subdirectory(third_party/stduuid)

# Main source files.
set(MAIN_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animation_frame_action_tool/editor_state.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animation_frame_action_tool/editor_state.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animation_frame_action_tool/runtime_data.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animation_frame_action_tool/runtime_data.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btglm.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btjson.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btjson.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btlogger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib/btlogger.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/btzc_game_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/component_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/component_registry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/components.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/components.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/game_object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/game_object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/system/concrete_systems.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/system/system__dev__anim_editor_tool_state_agent.cpp  @TEMP: To reduce errors for refactor.
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/system/system_animator_driven_hitcapsule_set.cpp  @TEMP: To reduce errors for refactor.
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/system/system_apply_phys_xform_to_rend_obj.cpp  @TEMP: To reduce errors for refactor.
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/system/system_ifc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/system/system_ifc.h
    # ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/system/system_player_character_movement.cpp  @TEMP: To reduce errors for refactor.
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/UNUSED_serialization.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/component_imgui_edit_functions.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/component_imgui_edit_functions.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/component_registry.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/component_registry.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/entity_metadata.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/physics_object_settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/physics_object_settings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/render_object_settings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/transform.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/component/transform.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/entity_container.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/entity_container.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/imgui_render_transform_hierarchy_window.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/imgui_render_transform_hierarchy_window.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/process_physics_object_lifetime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/process_physics_object_lifetime.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/process_render_object_lifetime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/process_render_object_lifetime.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/propagate_changed_transforms.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_system_logic/system/propagate_changed_transforms.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hitbox_interactor/hitcapsule.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/hitbox_interactor/hitcapsule.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input_handler/input_codes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input_handler/input_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input_handler/input_handler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl_custom_listeners.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl_error_callbacks.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl_layers.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl_obj_vs_broad_phase_filter.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine_impl.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object_impl_char_controller.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object_impl_char_controller.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object_impl_tri_mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object_impl_tri_mesh.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/physics_object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/raycast_helper.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/physics_engine/raycast_helper.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/refactor_to_entt.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/animator_template.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/animator_template.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/camera_read_ifc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/debug_render_job.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/debug_render_job.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/imgui_renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/imgui_renderer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_debug_lines.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_debug_lines.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_debug_picking.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_debug_picking.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_opaque_color_unlit.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_opaque_color_unlit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_opaque_shaded.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_opaque_shaded.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_opaque_texture_shaded.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_opaque_texture_shaded.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_post_process.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_post_process.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/mesh.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/model_animator.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/model_animator.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/render_layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/render_object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/render_object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/renderer_impl_win64.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/renderer_impl_win64.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/renderer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/texture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene_serialization_ifc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/scene/scene_serialization.cpp
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uuid/uuid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uuid/uuid.h
)

# Executable build.
add_executable(${PROJECT_NAME}
    # Main source files.
    ${MAIN_SOURCES}

    # OpenGL source files.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/src/glad.c

    # Dear ImGui implementation source files.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_glfw.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_opengl3_loader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_opengl3.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/backends/imgui_impl_opengl3.h

    # Dear ImGui base source files.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imconfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_demo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_draw.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_internal.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_tables.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui_widgets.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imgui.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imstb_rectpack.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imstb_textedit.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/imstb_truetype.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/misc/cpp/imgui_stdlib.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui/misc/cpp/imgui_stdlib.h

    # ImGuizmo source files.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ImGuizmo/ImGuizmo.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ImGuizmo/ImGuizmo.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ImGuizmo/ImSequencer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/ImGuizmo/ImSequencer.h

    # stb header files.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/stb_image.h
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/stb/stb_image_resize2.h

    # Tinyobjloader source files.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader/tiny_obj_loader.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader/tiny_obj_loader.h

    # Native extras.
    ${WIN64_RESOURCES})

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/bt_lib
        ${DEPENDENCY_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    fastgltf
    fmt::fmt
    glfw
    ${GLFW_LIBRARIES}
    Jolt
    nlohmann_json::nlohmann_json
    stduuid)

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
        # @TODO: Probably add the delete symlink code right here?
        file(CREATE_LINK ${dir_name} ${target_dir_name} SYMBOLIC)
    endif()
else()
    message(FATAL_ERROR "Asset folder location must be defined in `ASSET_DIR`")
endif()

# Set this project as Visual Studio startup project.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
