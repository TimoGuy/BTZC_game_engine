#include "material_impl_opaque_texture_shaded.h"

#include "material.h"
#include "shader.h"


BT::Material_opaque_texture_shaded::Material_opaque_texture_shaded(uint32_t color_image, vec3 tint)
    : m_color_image{ color_image }
{
    glm_vec3_copy(tint, m_tint);
}

void BT::Material_opaque_texture_shaded::bind_material(mat4 transform)
{
    mat4 projection;
    mat4 view;
    mat4 projection_view;
    Material_bank::get_camera_read_ifc()->fetch_camera_matrices(projection, view, projection_view);

    static auto& s_shader{ *Shader_bank::get_shader("textured_shaded") };
    s_shader.bind();
    s_shader.set_mat4("camera_projection_view", projection_view);
    s_shader.set_mat4("model_transform", transform);
    s_shader.bind_texture("color_image", 0, m_color_image);
    s_shader.set_vec3("tint", m_tint);
}

void BT::Material_opaque_texture_shaded::unbind_material()
{
    Shader::unbind();
}
