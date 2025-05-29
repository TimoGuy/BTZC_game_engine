cmake_minimum_required(VERSION 3.28)

# Project.
project(btzc_game_engine
    VERSION 0.1.0
    LANGUAGES CXX C)

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
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fastgltf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/imgui  # Header only.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/JoltPhysics
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader)

add_subdirectory(third_party/fastgltf)
add_subdirectory(third_party/fmt)
add_subdirectory(third_party/glfw)
add_subdirectory(third_party/JoltPhysics/Build)

# Executable build.
add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/scripts/pps_player_character_movement.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/scripts/pre_physics_scripts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/scripts/pre_physics_scripts.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/scripts/pre_render_scripts.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/scripts/pre_render_scripts.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/scripts/prs_apply_phys_xform_to_rend_obj.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/scripts/serialization.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/game_object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/game_object/game_object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input_handler/input_codes.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input_handler/input_handler.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/input_handler/input_handler.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logger/logger.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/logger/logger.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/camera_read_ifc.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/camera.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/camera.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_opaque_shaded.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_opaque_shaded.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_post_process.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material_impl_post_process.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/material.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/mesh.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/mesh.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/render_object.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/render_object.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/renderer_impl_win64.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/renderer_impl_win64.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/renderer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/shader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderer/shader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/timer/timer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/timer/timer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/btzc_game_engine.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    
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
    
    # Tinyobjloader source files.
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader/tiny_obj_loader.cc
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyobjloader/tiny_obj_loader.h)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/logger
        ${DEPENDENCY_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    fastgltf
    fmt::fmt
    glfw
    ${GLFW_LIBRARIES}
    Jolt)

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
