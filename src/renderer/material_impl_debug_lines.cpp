#include "material_impl_debug_lines.h"

#include "glad/glad.h"
#include "shader.h"


BT::Material_debug_lines::Material_debug_lines(bool foreground)
    : m_foreground{ foreground }
    , m_ssbo{ (uint32_t)-1 }
{
}

void BT::Material_debug_lines::set_lines_ssbo(uint32_t ssbo)
{
    m_ssbo = ssbo;
}

void BT::Material_debug_lines::bind_material(mat4 transform)
{
    mat4 projection;
    mat4 view;
    mat4 projection_view;
    Material_bank::get_camera_read_ifc()->fetch_camera_matrices(projection, view, projection_view);

    // Setup depth test function.
    glDepthFunc(m_foreground ? GL_LEQUAL : GL_GREATER);

    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    static auto& s_shader{ *Shader_bank::get_shader("color_unlit_lines") };
    s_shader.bind();
    s_shader.set_mat4("camera_projection_view", projection_view);
}

void BT::Material_debug_lines::unbind_material()
{
    Shader::unbind();

    // Reset depth test params.
    glDepthFunc(GL_LEQUAL);

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
}
