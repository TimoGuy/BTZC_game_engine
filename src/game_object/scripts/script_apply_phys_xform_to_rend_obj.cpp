#include "scripts.h"

#include "../game_object.h"
#include "../physics_engine/physics_engine.h"
#include "../physics_engine/physics_object.h"
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
                                        Game_object_pool& game_obj_pool,
                                        UUID game_obj_key)
        : m_phys_engine{ phys_engine }
        , m_game_obj_pool{ game_obj_pool }
        , m_game_obj_key{ game_obj_key }
    {
    }

    Physics_engine& m_phys_engine;
    Game_object_pool& m_game_obj_pool;
    UUID m_game_obj_key;

    // Script_ifc.
    Script_type get_type() override
    {
        return SCRIPT_TYPE_apply_physics_transform_to_render_object;
    }

    void serialize_datas(json& node_ref) override
    {
        node_ref["game_obj_key"] = UUID_helper::to_pretty_repr(m_game_obj_key);
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
        Game_object_pool* game_obj_pool,
        json const& node_ref)
{
    return unique_ptr<Script_ifc>(
        new Script_apply_phys_xform_to_rend_obj{ *phys_engine,
                                                 *game_obj_pool,
                                                 UUID_helper::to_UUID(node_ref["game_obj_key"]) });
}


// Script_apply_phys_xform_to_rend_obj.
void BT::Scripts::Script_apply_phys_xform_to_rend_obj::on_pre_render(float_t delta_time)
{
    // @NOTE: Inside scripts, all game objects are checked out, so no lock is required.
    auto game_obj{ m_game_obj_pool.get_one_no_lock(m_game_obj_key) };

    // Get transform from phys obj.
    rvec3 position;
    versor rotation;
    auto phys_obj{ m_phys_engine.checkout_physics_object(game_obj->get_phys_obj_key()) };
    phys_obj->get_transform_for_game_obj(position, rotation);
    m_phys_engine.return_physics_object(phys_obj);

    // Apply to game object.
    game_obj->get_transform_handle().set_global_pos_rot(position, rotation);
    game_obj->propagate_transform_changes();
}
