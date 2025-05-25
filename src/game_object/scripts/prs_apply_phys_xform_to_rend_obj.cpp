#include "pre_render_scripts.h"

#include "../physics_engine/physics_engine.h"
#include "../physics_engine/physics_object.h"
#include "../renderer/render_object.h"
#include "cglm/affine.h"
#include "cglm/quat.h"


void BT::Pre_render_script::script_apply_physics_transform_to_render_object(
    Render_object* rend_obj,
    vector<uint64_t> const& datas,
    size_t& in_out_read_data_idx)
{
    auto key{ rend_obj->get_tethered_phys_obj_key() };
    auto phys_engine{ reinterpret_cast<Physics_engine*>(datas[in_out_read_data_idx++]) };

    // Get transform for rendering.
    rvec3 position;
    versor rotation;
    auto phys_obj{ phys_engine->checkout_physics_object(key) };
    phys_obj->get_transform_for_rendering(position, rotation);
    phys_engine->return_physics_object(phys_obj);

    // Apply to render object.
    mat4 transform;
    glm_translate_make(transform, vec3{ position[0], position[1], position[2] });
    glm_quat_rotate(transform, rotation, transform);
    rend_obj->set_transform(transform);
}
