#include "material_impl_opaque_texture_shaded.h"

#include "cglm/util.h"
#include "material.h"
#include "shader.h"
#include <cmath>


BT::Material_opaque_texture_shaded::Material_opaque_texture_shaded(uint32_t color_image,
                                                                   vec3 tint_standable,
                                                                   vec3 tint_non_standable,
                                                                   float_t standable_angle_deg)
    : m_color_image{ color_image }
    , m_sin_standable_angle{ sinf(glm_rad(standable_angle_deg)) }
{
    glm_vec3_copy(tint_standable, m_tint_standable);
    glm_vec3_copy(tint_non_standable, m_tint_non_standable);
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
    s_shader.set_vec3("tint_standable", m_tint_standable);
    s_shader.set_vec3("tint_non_standable", m_tint_non_standable);
    s_shader.set_float("sin_standable_angle", m_sin_standable_angle);
}

void BT::Material_opaque_texture_shaded::unbind_material()
{
    Shader::unbind();
}
