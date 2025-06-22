#include "material_impl_opaque_color_unlit.h"

#include "material.h"
#include "shader.h"
#include "glad/glad.h"
#include <cstdint>
#include <unordered_map>

using std::unordered_map;


namespace
{

static unordered_map<uint8_t, int32_t> s_depth_test_mode_codes{
    { BT::k_depth_test_mode_any, GL_ALWAYS },
    { BT::k_depth_test_mode_front, GL_LEQUAL },
    { BT::k_depth_test_mode_back, GL_GREATER },
};

}  // namespace


BT::Material_opaque_color_unlit::Material_opaque_color_unlit(vec3 color, uint8_t depth_test_mode)
    : m_depth_test_mode{ depth_test_mode }
{
    glm_vec3_copy(color, m_color);
}

void BT::Material_opaque_color_unlit::bind_material(mat4 transform)
{
    mat4 projection;
    mat4 view;
    mat4 projection_view;
    Material_bank::get_camera_read_ifc()->fetch_camera_matrices(projection, view, projection_view);

    // Setup depth test function.
    glDepthFunc(s_depth_test_mode_codes.at(m_depth_test_mode));

    // Wireframe mode.
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    static auto& s_shader{ *Shader_bank::get_shader("color_unlit") };
    s_shader.bind();
    s_shader.set_mat4("camera_projection_view", projection_view);
    s_shader.set_mat4("model_transform", transform);
    s_shader.set_vec3("color", m_color);
}

void BT::Material_opaque_color_unlit::unbind_material()
{
    Shader::unbind();

    // Fill mode.
    // @NOTE: If there's ever a wireframe rendering mode, there needs to be a better
    //   way to push/pop polygon modes bc the assumption of `GL_FILL` being normal
    //   isn't good.
    // @NOTE: The problem^^ is not an issue w/ Vulkan.
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

    // Reset depth test params.
    // @NOTE: Same problem as above. Idk what the defaults are @TECHDEBT
    glDepthFunc(GL_LEQUAL);
}
