#include "debug_render_job.h"

#include "glad/glad.h"
#include <cassert>
#include <memory>
#include <mutex>


BT::Debug_line_pool::Debug_line_pool()
{
    glGenBuffers(1, &m_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER,
                 sizeof(Debug_line) * k_num_lines,
                 nullptr,
                 GL_DYNAMIC_STORAGE_BIT);
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
        glBufferSubData(GL_SHADER_STORAGE_BUFFER,
                        0,
                        sizeof(Debug_line) * m_active_indices.size(),
                        jobs.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    Render_data data;
    data.num_lines_to_render = m_active_indices.size();
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
