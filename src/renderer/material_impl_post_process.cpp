#include "material_impl_post_process.h"

#include "material.h"
#include "shader.h"
#include "texture.h"


BT::Material_impl_post_process::Material_impl_post_process(float_t exposure)
    : m_exposure{ exposure }
{
}

void BT::Material_impl_post_process::bind_material(mat4 transform)
{
    static auto& s_shader{ *Shader_bank::get_shader("post_process") };
    s_shader.bind();
    s_shader.bind_texture("hdr_buffer", 0, Texture_bank::get_texture_2d("hdr_color_texture"));
    s_shader.set_float("exposure", m_exposure);
}

void BT::Material_impl_post_process::unbind_material()
{
    Shader::unbind();
}
