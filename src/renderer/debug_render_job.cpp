#include "debug_render_job.h"

#include "../uuid/uuid.h"
#include "../renderer/mesh.h"
#include "glad/glad.h"
#include "logger.h"
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
            mesh_job.second.model.render_model(mesh_job.second.transform,
                                               mesh_job.second.foreground_material);
        if (mesh_job.second.background_material != nullptr)
            mesh_job.second.model.render_model(mesh_job.second.transform,
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
    m_lines[write_idx] = { timeout, std::move(dbg_line) };

    // Add to active indices.
    std::lock_guard<std::mutex> lock{ m_active_indices_mutex };
    m_active_indices.emplace_back(write_idx);
    m_is_dirty = true;
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
    {
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
