#include "logger.h"
#include "scripts.h"

#include "../renderer/renderer.h"
// #include "../renderer/mesh.h"
// #include "../renderer/model_animator.h"
// #include "../animation_frame_action_tool/editor_state.h"
#include "../hitbox_interactor/hitcapsule.h"
#include "../service_finder/service_finder.h"
#include "../uuid/uuid.h"
#include "cglm/types-struct.h"
// #include <memory>
#include <vector>

namespace BT
{
class Input_handler;
class Physics_engine;
class Game_object_pool;
}


namespace BT::Scripts
{

class Script_animator_driven_hitcapsule_set : public Script_ifc
{
public:
    Script_animator_driven_hitcapsule_set(json const& node_ref)
    {
        m_rend_obj_key = UUID_helper::to_UUID(node_ref["render_obj"]);
    }

    // Script_ifc.
    Script_type get_type() override { return SCRIPT_TYPE_animator_driven_hitcapsule_set; }
    void serialize_datas(json& node_ref) override;

    // Script data.
    UUID m_rend_obj_key;


    void on_pre_physics(float_t physics_delta_time) override
    {   // Update whether capsules are enabled and keep capsules attached
        // to connecting bone in animator.
        auto& rend_obj_pool{ service_finder::find_service<Renderer>()
                             .get_render_object_pool() };
        auto rend_obj{ rend_obj_pool
                           .checkout_render_obj_by_key({ m_rend_obj_key })
                           .front() };

        auto& model_animator{ *rend_obj->get_model_animator() };

        model_animator.get_anim_frame_action_data_handle().assign_hitcapsule_enabled_flags();

        std::vector<mat4s> joint_matrices;
        model_animator.get_anim_floored_frame_pose(joint_matrices);
        model_animator.get_anim_frame_action_data_handle()
            .update_hitcapsule_transform_to_joint_mats(joint_matrices);

        rend_obj_pool.return_render_objs({ rend_obj });
    }
};

}  // namespace BT

void BT::Scripts::Script_animator_driven_hitcapsule_set::serialize_datas(json& node_ref)
{
    node_ref["render_obj"] = UUID_helper::to_pretty_repr(m_rend_obj_key);
}

// Create script.
std::unique_ptr<BT::Scripts::Script_ifc>
BT::Scripts::Factory_impl_funcs
    ::create_script_animator_driven_hitcapsule_set_from_serialized_datas(
        Input_handler*    input_handler,
        Physics_engine*   phys_engine,
        Renderer*         renderer,
        Game_object_pool* game_obj_pool,
        json const&       node_ref)
{
    return std::unique_ptr<Script_ifc>(new Script_animator_driven_hitcapsule_set{ node_ref });
}
