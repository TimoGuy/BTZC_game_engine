#include "debug_render_job.h"

#include "../uuid/uuid.h"
#include "../renderer/mesh.h"
#include "btglm.h"
#include "glad/glad.h"
#include "btlogger.h"
#include <array>
#include <cassert>
#include <memory>
#include <mutex>


// Debug mesh.
BT::UUID BT::Debug_mesh_pool::emplace_debug_mesh(Debug_mesh&& dbg_mesh)
{
    std::lock_guard<std::mutex> lock{ m_meshes_mutex };

    UUID uuid{ UUID_helper::generate_uuid() };
    if (uuid.is_nil())
    {
        logger::printe(logger::ERROR, "Invalid UUID generated... somehow.");
        assert(false);
    }

    m_meshes.emplace(uuid, std::move(dbg_mesh));

    return uuid;
}

BT::Debug_mesh& BT::Debug_mesh_pool::get_debug_mesh_volatile_handle(UUID key)
{
    std::lock_guard<std::mutex> lock{ m_meshes_mutex };

    if (m_meshes.find(key) == m_meshes.end())
    {
        // Fail bc key was invalid.
        logger::printef(logger::ERROR, "Mesh key %s does not exist", UUID_helper::to_pretty_repr(key).c_str());
        assert(false);
        abort();

        // In case abort and assert don't crash program. Just force program to spin forever.
        while (false) {}
    }

    return m_meshes.at(key);
}

void BT::Debug_mesh_pool::remove_debug_mesh(UUID key)
{
    std::lock_guard<std::mutex> lock{ m_meshes_mutex };

    if (m_meshes.find(key) == m_meshes.end())
    {
        // Fail bc key was invalid.
        logger::printef(logger::ERROR, "Mesh key %s does not exist", UUID_helper::to_pretty_repr(key).c_str());
        assert(false);
        return;
    }

    m_meshes.erase(key);
}

void BT::Debug_mesh_pool::render_all_meshes()
{
    if (!get_visible())
        return;

    std::lock_guard<std::mutex> lock{ m_meshes_mutex };

    for (auto& mesh_job : m_meshes)
    {
        // Render foreground first so that foreground doesn't overwrite background material.
        if (mesh_job.second.foreground_material != nullptr)
            mesh_job.second.model.render(mesh_job.second.transform,
                                         mesh_job.second.foreground_material);
        if (mesh_job.second.background_material != nullptr)
            mesh_job.second.model.render(mesh_job.second.transform,
                                         mesh_job.second.background_material);
    }
}

namespace
{

std::unique_ptr<BT::Debug_mesh_pool> s_debug_mesh_pool{ nullptr };

}  // namespace

BT::Debug_mesh_pool& BT::set_main_debug_mesh_pool(std::unique_ptr<Debug_mesh_pool>&& dbg_mesh_pool)
{
    s_debug_mesh_pool = std::move(dbg_mesh_pool);
    return *s_debug_mesh_pool;
}

BT::Debug_mesh_pool& BT::get_main_debug_mesh_pool()
{
    return *s_debug_mesh_pool;
}


// Debug line.
BT::Debug_line_pool::Debug_line_pool()
{
    glGenBuffers(1, &m_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 sizeof(Debug_line) * k_num_lines,
                 nullptr,  //  m_huhidkwtd.data(),
                 GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

BT::Debug_line_pool::~Debug_line_pool()
{
    glDeleteBuffers(1, &m_ssbo);
}

void BT::Debug_line_pool::emplace_debug_line(Debug_line&& dbg_line, float_t timeout /*= 1.0f*/)
{
    assert(timeout > 0.0f);

    // Add to pool.
    uint32_t write_idx{ m_next_write_idx++ };
    write_idx = (write_idx % k_num_lines);
    m_lines[write_idx] = { timeout, std::move(dbg_line) };

    // Add to active indices.
    std::lock_guard<std::mutex> lock{ m_active_indices_mutex };
    m_active_indices.emplace_back(write_idx);
    m_is_dirty = true;
}

void BT::Debug_line_pool::emplace_debug_line_based_capsule(
    vec3 origin_a, vec3 origin_b, float_t radius, vec4 color, float_t timeout /*= 1.0f*/)
{   // Calculate basis vectors.
    vec3s basis_x{ 1.0f, 0.0f, 0.0f };
    vec3s basis_y{ 0.0f, 1.0f, 0.0f };
    vec3s basis_z{ 0.0f, 0.0f, 1.0f };

    if (glm_vec3_distance2(origin_a, origin_b) > 1e-6f)
    {   // Calc basis-y.
        glm_vec3_sub(origin_b, origin_a, basis_y.raw);
        glm_vec3_normalize(basis_y.raw);

        // Calc next basis axis.
        bool using_z_axis{ false };
        vec3s some_axis{ 0.0f, 1.0f, 0.0f };
        if (std::abs(basis_y.y) > 1.0f - 1e-6f)
        {
            using_z_axis = true;
            some_axis = { 0.0f, 0.0f, 1.0f };
        }

        glm_vec3_crossn(basis_y.raw, some_axis.raw, basis_x.raw);

        // Calc final basis axis.
        glm_vec3_crossn(basis_x.raw, basis_y.raw, basis_z.raw);
    }

    // Transform points.
    static std::vector<vec3s> const k_end_cap_a_x{
        { 0,  0,          -1          },
        { 0, -0.38268343, -0.92387953 },
        { 0, -0.70710678, -0.70710678 },
        { 0, -0.92387953, -0.38268343 },
        { 0, -1,           0          },
        { 0, -0.92387953,  0.38268343 },
        { 0, -0.70710678,  0.70710678 },
        { 0, -0.38268343,  0.92387953 },
        { 0,  0,           1          },
    };
    static std::vector<vec3s> const k_end_cap_a_z{
        { -1,           0,          0 },
        { -0.92387953, -0.38268343, 0 },
        { -0.70710678, -0.70710678, 0 },
        { -0.38268343, -0.92387953, 0 },
        {  0,          -1,          0 },
        {  0.38268343, -0.92387953, 0 },
        {  0.70710678, -0.70710678, 0 },
        {  0.92387953, -0.38268343, 0 },
        {  1,           0,          0 },
    };
    static std::vector<vec3s> const k_end_cap_b_x{
        { 0, 0,           1          },
        { 0, 0.38268343,  0.92387953 },
        { 0, 0.70710678,  0.70710678 },
        { 0, 0.92387953,  0.38268343 },
        { 0, 1,           0          },
        { 0, 0.92387953, -0.38268343 },
        { 0, 0.70710678, -0.70710678 },
        { 0, 0.38268343, -0.92387953 },
        { 0, 0,          -1          },
    };
    static std::vector<vec3s> const k_end_cap_b_z{
        {  1,           0,          0 },
        {  0.92387953,  0.38268343, 0 },
        {  0.70710678,  0.70710678, 0 },
        {  0.38268343,  0.92387953, 0 },
        {  0,           1,          0 },
        { -0.38268343,  0.92387953, 0 },
        { -0.70710678,  0.70710678, 0 },
        { -0.92387953,  0.38268343, 0 },
        { -1,           0,          0 },
    };
    static std::vector<vec3s> const k_end_cap_ab_y{
        {  1,          0,  0          },
        {  0.92387953, 0,  0.38268343 },
        {  0.70710678, 0,  0.70710678 },
        {  0.38268343, 0,  0.92387953 },
        {  0,          0,  1          },
        { -0.38268343, 0,  0.92387953 },
        { -0.70710678, 0,  0.70710678 },
        { -0.92387953, 0,  0.38268343 },
        { -1,          0,  0          },
        { -0.92387953, 0, -0.38268343 },
        { -0.70710678, 0, -0.70710678 },
        { -0.38268343, 0, -0.92387953 },
        {  0,          0, -1          },
        {  0.38268343, 0, -0.92387953 },
        {  0.70710678, 0, -0.70710678 },
        {  0.92387953, 0, -0.38268343 },
        {  1,          0,  0          },
    };

    struct Transformed_end_cap_point_set
    {
        std::vector<vec3s> const& base_end_cap_ref;
        bool use_origin_a;
        std::vector<vec3s> trans_end_cap;
    };
    std::vector<Transformed_end_cap_point_set> trans_ecp_sets{
        { k_end_cap_a_x, true },    // End cap A-X.
        { k_end_cap_a_z, true },    // End cap A-Z.
        { k_end_cap_ab_y, true },   // End cap A-Y.
        { k_end_cap_b_x, false },   // End cap B-X.
        { k_end_cap_b_z, false },   // End cap B-Z.
        { k_end_cap_ab_y, false },  // End cap B-Y.
    };

    for (auto& ecp_set : trans_ecp_sets)
    {
        ecp_set.trans_end_cap.reserve(ecp_set.base_end_cap_ref.size());

        for (auto& pt : ecp_set.base_end_cap_ref)
        {   // Transform point into capsule transform.
            vec3s trans_pt;
            glm_vec3_scale(basis_x.raw, pt.x * radius, trans_pt.raw);
            glm_vec3_muladds(basis_y.raw, pt.y * radius, trans_pt.raw);
            glm_vec3_muladds(basis_z.raw, pt.z * radius, trans_pt.raw);

            glm_vec3_add(trans_pt.raw,
                         ecp_set.use_origin_a ? origin_a : origin_b,
                         trans_pt.raw);

            ecp_set.trans_end_cap.emplace_back(trans_pt);
        }
    }

    // Emplace points as lines.
    for (auto& ecp_set : trans_ecp_sets)
        for (size_t idx = 1; idx < ecp_set.trans_end_cap.size(); idx++)
        {   // Draw line from translated end cap points.
            Debug_line new_line;
            glm_vec3_copy(ecp_set.trans_end_cap[idx - 1].raw, new_line.pos1);
            glm_vec3_copy(ecp_set.trans_end_cap[idx + 0].raw, new_line.pos2);
            glm_vec4_copy(color, new_line.color1);
            glm_vec4_copy(color, new_line.color2);

            emplace_debug_line(std::move(new_line), timeout);
        }
    
    auto& end_cap_a_y{ trans_ecp_sets[2] };
    auto& end_cap_b_y{ trans_ecp_sets[5] };
    for (size_t idx = 0; idx < 16; idx += 2)
    {   // Draw "ribs": connecting lines between end caps.
        Debug_line new_line;
        glm_vec3_copy(end_cap_a_y.trans_end_cap[idx].raw, new_line.pos1);
        glm_vec3_copy(end_cap_b_y.trans_end_cap[idx].raw, new_line.pos2);
        glm_vec4_copy(color, new_line.color1);
        glm_vec4_copy(color, new_line.color2);

        emplace_debug_line(std::move(new_line), timeout);
    }
}

BT::Debug_line_pool::Render_data BT::Debug_line_pool::calc_render_data(float_t delta_time)
{
    std::lock_guard<std::mutex> lock{ m_active_indices_mutex };

    // Process job timeouts.
    for (int64_t i = m_active_indices.size() - 1; i >= 0; i--)
    {
        auto& line{ m_lines[m_active_indices[i]] };
        line.remaining_time -= delta_time;
        if (line.remaining_time < 0.0f)
        {
            // Remove expired job.
            m_active_indices.erase(m_active_indices.begin() + i);
            m_is_dirty = true;
        }
    }

    if (m_is_dirty)
    {   // @NOTE: This seems to be the only reason why the GPU upload `memcpy()` step crashes.
        assert(m_active_indices.size() <= k_num_lines);

        // Insert jobs into SSBO.
        std::vector<Debug_line> jobs;
        jobs.reserve(m_active_indices.size());
        for (uint32_t idx : m_active_indices)
        {
            jobs.emplace_back(m_lines[idx].dbg_line);
        }

        glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
        GLvoid* data = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
        memcpy(data, jobs.data(), sizeof(Debug_line) * jobs.size());
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        m_is_dirty = false;
    }

    Render_data data;
    data.num_lines_to_render = (get_visible() ? m_active_indices.size() : 0);
    data.ssbo = m_ssbo;
    return data;
}


namespace
{

std::unique_ptr<BT::Debug_line_pool> s_debug_line_pool{ nullptr };

}  // namespace

BT::Debug_line_pool& BT::set_main_debug_line_pool(std::unique_ptr<Debug_line_pool>&& dbg_line_pool)
{
    s_debug_line_pool = std::move(dbg_line_pool);
    return *s_debug_line_pool;
}

BT::Debug_line_pool& BT::get_main_debug_line_pool()
{
    return *s_debug_line_pool;
}
