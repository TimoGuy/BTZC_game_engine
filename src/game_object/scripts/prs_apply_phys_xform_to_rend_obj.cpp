#include "pre_render_scripts.h"

#include "../physics_engine/physics_engine.h"
#include "../physics_engine/physics_object.h"
#include "../renderer/renderer.h"
#include "cglm/affine.h"
#include "cglm/quat.h"
#include "serialization.h"


void BT::Pre_render_script::script_apply_physics_transform_to_render_object(
    Renderer* renderer,
    vector<uint64_t> const& datas,
    size_t& in_out_read_data_idx)
{
    auto rend_obj_key{ Serial::pop_u64(datas, in_out_read_data_idx) };
    auto phys_engine{
        reinterpret_cast<Physics_engine*>(Serial::pop_void_ptr(datas, in_out_read_data_idx)) };

    Render_object* rend_obj{ renderer->get_render_object(rend_obj_key) };
    auto tethered_phys_key{ rend_obj->get_tethered_phys_obj_key() };

    // Get transform for rendering.
    rvec3 position;
    versor rotation;
    auto phys_obj{ phys_engine->checkout_physics_object(tethered_phys_key) };
    phys_obj->get_transform_for_rendering(position, rotation);
    phys_engine->return_physics_object(phys_obj);

    // Apply to render object.
    mat4 transform;
    glm_translate_make(transform, vec3{ position[0], position[1], position[2] });
    glm_quat_rotate(transform, rotation, transform);
    rend_obj->set_transform(transform);
}
