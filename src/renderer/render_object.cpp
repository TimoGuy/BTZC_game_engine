#include "render_object.h"

#include "cglm/affine.h"
#include "cglm/cglm.h"
#include "logger.h"
#include "mesh.h"


BT::Render_object::Render_object(Model const& model,
                                 Render_layer layer,
                                 mat4 init_transform,
                                 physics_object_key_t tethered_phys_obj /*= (physics_object_key_t)-1*/)
    : m_model(model)
    , m_layer(layer)
    , m_tethered_phys_obj(tethered_phys_obj)
{
    glm_mat4_copy(init_transform, m_transform);

    // Check that the layer is a single layer, not an aggregate layer.
    constexpr uint32_t k_num_shifts{ sizeof(Render_layer) * 8 };
    uint32_t num_matches{ 0 };
    for (uint32_t i = 0; i < k_num_shifts; i++)
    {
        static_assert(sizeof(Render_layer) <= sizeof(uint64_t));
        uint64_t check_val{ 1ui64 << i };
        if (static_cast<uint64_t>(layer) & check_val)
        {
            num_matches++;
        }
    }

    if (num_matches == 0)
    {
        logger::printe(logger::ERROR, "No render layer assigned to render object.");
        assert(false);
    }
    else if (num_matches > 1)
    {
        logger::printe(logger::ERROR, "Multiple render layers assigned to render object.");
        assert(false);
    }
}

void BT::Render_object::render(Render_layer active_layers)
{
    if (m_layer & active_layers)
    {
        m_model.render_model(m_transform);
    }
}

void BT::Render_object::set_transform(mat4 transform)
{
    glm_mat4_copy(transform, m_transform);
}

void BT::Render_object::get_position(vec3& position)
{
    vec4 pos4;
    glm_vec4_copy(m_transform[3], pos4);  // Copied from `glm_decompose()`.
    glm_vec3(pos4, position);
}
