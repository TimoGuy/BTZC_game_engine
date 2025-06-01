#include "pre_render_scripts.h"

#include "../physics_engine/physics_engine.h"
#include "../physics_engine/physics_object.h"
#include "../renderer/renderer.h"
#include "cglm/affine.h"
#include "cglm/quat.h"
#include "serialization.h"


void BT::Pre_render_script::script_apply_physics_transform_to_render_object_serialize(
    Input_handler* input_handler,
    Physics_engine* phys_engine,
    Renderer* renderer,
    Scene_serialization_mode mode,
    json& node_ref,
    vector<uint64_t> const& datas,
    size_t& in_out_read_data_idx)
{
    if (mode == Scene_serialization_mode::SCENE_SERIAL_MODE_SERIALIZE)
    {
        node_ref["rend_obj_key"] = UUID_helper::pretty_repr(Serial::pop_uuid(datas, in_out_read_data_idx));
        // Physics engine.
    }
    else if (mode == Scene_serialization_mode::SCENE_SERIAL_MODE_DESERIALIZE)
    {
        // @TODO
        assert(false);
    }
}

void BT::Pre_render_script::script_apply_physics_transform_to_render_object(
    Renderer* renderer,
    vector<uint64_t> const& datas,
    size_t& in_out_read_data_idx)
{
    auto rend_obj_key{ Serial::pop_uuid(datas, in_out_read_data_idx) };
    auto phys_engine{
        reinterpret_cast<Physics_engine*>(Serial::pop_void_ptr(datas, in_out_read_data_idx)) };

    auto& rend_obj_pool{ renderer->get_render_object_pool() };
    vector<UUID> rend_obj_keys{ rend_obj_key };
    auto rend_objs{ rend_obj_pool.checkout_render_obj_by_key(std::move(rend_obj_keys)) };
    auto tethered_phys_key{ rend_objs[0]->get_tethered_phys_obj_key() };

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
    rend_objs[0]->set_transform(transform);
    rend_obj_pool.return_render_objs(std::move(rend_objs));
}
