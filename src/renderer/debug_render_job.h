#pragma once

#include "../uuid/uuid.h"
#include "cglm/mat4.h"
#include "cglm/types.h"
#include <array>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>


namespace BT
{

class Model;
class Material_ifc;

// Debug mesh.
struct Debug_mesh
{
    Model const& model;
    Material_ifc* foreground_material{ nullptr };
    Material_ifc* background_material{ nullptr };
    mat4 transform = GLM_MAT4_IDENTITY_INIT;
};

class Debug_mesh_pool
{
public:
    UUID emplace_debug_mesh(Debug_mesh&& dbg_mesh);
    Debug_mesh& get_debug_mesh_volatile_handle(UUID key);
    void remove_debug_mesh(UUID key);

    void render_all_meshes();

private:
    std::mutex m_meshes_mutex;
    std::unordered_map<UUID, Debug_mesh> m_meshes;
};

Debug_mesh_pool& set_main_debug_mesh_pool(std::unique_ptr<Debug_mesh_pool>&& dbg_mesh_pool);
Debug_mesh_pool& get_main_debug_mesh_pool();


// Debug line.
struct Debug_line
{
    vec4 pos1;
    vec4 pos2;
    vec4 color1;
    vec4 color2;
};

class Debug_line_pool
{
public:
    Debug_line_pool();
    ~Debug_line_pool();

    void emplace_debug_line(Debug_line&& dbg_line, float_t timeout = 1.0f);

    struct Render_data
    {
        size_t num_lines_to_render;
        uint32_t ssbo;
    };
    Render_data calc_render_data(float_t delta_time);

private:
    struct Debug_line_with_timeout
    {
        float_t remaining_time{ -1.0f };
        Debug_line dbg_line;
    };


    static constexpr uint32_t k_num_lines{ 1024 };  // @NOTE: Must be power of 2.
    std::array<Debug_line_with_timeout, k_num_lines> m_lines;
    std::atomic_uint32_t m_next_write_idx{ 0 };

    // std::array<Debug_line, k_num_lines> m_huhidkwtd;

    std::mutex m_active_indices_mutex;
    std::vector<uint32_t> m_active_indices;
    bool m_is_dirty{ false };

    uint32_t m_ssbo{ (uint32_t)-1 };
};

Debug_line_pool& set_main_debug_line_pool(std::unique_ptr<Debug_line_pool>&& dbg_line_pool);
Debug_line_pool& get_main_debug_line_pool();

}  // namespace BT
