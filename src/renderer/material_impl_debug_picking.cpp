#include "material_impl_debug_picking.h"

#include "shader.h"


BT::Material_debug_picking::Material_debug_picking()
    : m_color{ 0.0f, 0.0f, 0.0f }
{
}

void BT::Material_debug_picking::set_color(vec3 color)
{
    glm_vec3_copy(color, m_color);
}

void BT::Material_debug_picking::bind_material(mat4 transform)
{
    mat4 projection;
    mat4 view;
    mat4 projection_view;
    Material_bank::get_camera_read_ifc()->fetch_camera_matrices(projection, view, projection_view);

    static auto& s_shader{ *Shader_bank::get_shader("color_unlit") };
    s_shader.bind();
    s_shader.set_mat4("camera_projection_view", projection_view);
    s_shader.set_mat4("model_transform", transform);
    s_shader.set_vec3("color", m_color);
}

void BT::Material_debug_picking::unbind_material()
{
    Shader::unbind();
}
