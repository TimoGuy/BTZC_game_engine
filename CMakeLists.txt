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

# Dependencies.
set(DEPENDENCY_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fastgltf/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/fmt/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glfw/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/JoltPhysics)

add_subdirectory(third_party/fastgltf)
add_subdirectory(third_party/fmt)
add_subdirectory(third_party/glfw)
add_subdirectory(third_party/JoltPhysics/Build)

# Executable build.
add_executable(${PROJECT_NAME}
    ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/glad/src/glad.c)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${DEPENDENCY_INCLUDE_DIRS})

target_link_libraries(${PROJECT_NAME}
    fastgltf
    fmt::fmt
    glfw
    ${GLFW_LIBRARIES}
    Jolt)

# Set this project as Visual Studio startup project.
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
