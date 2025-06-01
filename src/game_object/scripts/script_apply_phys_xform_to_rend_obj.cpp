#include "scripts.h"

#include "../physics_engine/physics_engine.h"
#include "../physics_engine/physics_object.h"
#include "../renderer/renderer.h"
#include "cglm/affine.h"
#include "cglm/quat.h"
#include <memory>

using std::unique_ptr;


namespace BT::Scripts
{

class Script_apply_phys_xform_to_rend_obj : public Script_ifc
{
public:
    Script_apply_phys_xform_to_rend_obj(Physics_engine& phys_engine,
                                        Renderer& renderer,
                                        UUID rend_obj_key)
        : m_phys_engine{ phys_engine }
        , m_renderer{ renderer }
        , m_rend_obj_key{ rend_obj_key }
    {
    }

    Physics_engine& m_phys_engine;
    Renderer& m_renderer;
    UUID m_rend_obj_key;
    bool m_prev_jump_pressed{ false };
    bool m_prev_crouch_pressed{ false };

    // Script_ifc.
    Script_type get_type() override
    {
        return SCRIPT_TYPE_apply_physics_transform_to_render_object;
    }

    void serialize_datas(json& node_ref) override
    {
        node_ref["rend_obj_key"] = UUID_helper::to_pretty_repr(m_rend_obj_key);
    }

    void on_pre_render(float_t delta_time) override;
};

}  // namespace BT


// Create script.
unique_ptr<BT::Scripts::Script_ifc>
BT::Scripts::Factory_impl_funcs
    ::create_script_apply_physics_transform_to_render_object_from_serialized_datas(
        Input_handler* input_handler,
        Physics_engine* phys_engine,
        Renderer* renderer,
        json const& node_ref)
{
    return unique_ptr<Script_ifc>(
        new Script_apply_phys_xform_to_rend_obj{ *phys_engine,
                                                 *renderer,
                                                 UUID_helper::to_UUID(node_ref["rend_obj_key"]) });
}


// Script_apply_phys_xform_to_rend_obj.
void BT::Scripts::Script_apply_phys_xform_to_rend_obj::on_pre_render(float_t delta_time)
{
    auto& rend_obj_pool{ m_renderer.get_render_object_pool() };
    vector<UUID> rend_obj_keys{ m_rend_obj_key };
    auto rend_objs{ rend_obj_pool.checkout_render_obj_by_key(std::move(rend_obj_keys)) };
    auto tethered_phys_key{ rend_objs[0]->get_tethered_phys_obj_key() };

    // Get transform for rendering.
    rvec3 position;
    versor rotation;
    auto phys_obj{ m_phys_engine.checkout_physics_object(tethered_phys_key) };
    phys_obj->get_transform_for_rendering(position, rotation);
    m_phys_engine.return_physics_object(phys_obj);

    // Apply to render object.
    mat4 transform;
    glm_translate_make(transform, vec3{ position[0], position[1], position[2] });
    glm_quat_rotate(transform, rotation, transform);
    rend_objs[0]->set_transform(transform);
    rend_obj_pool.return_render_objs(std::move(rend_objs));
}
