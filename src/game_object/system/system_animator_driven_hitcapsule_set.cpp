// clang-format off
#include "concrete_systems.h"
// clang-format on

#include "../../service_finder/service_finder.h"
#include "../component_registry.h"
#include "../components.h"
#include "../game_object.h"
#include "../renderer/model_animator.h"
#include "system_ifc.h"


namespace
{
enum : int32_t
{
    Q_IDX_COMP_LISTS_WITH_ANIMATOR_HITCAPSULE,
};
}  // namespace


BT::component_system::system::System_animator_driven_hitcapsule_set::
    System_animator_driven_hitcapsule_set()
    : System_ifc({
          Component_list_query::compile_query_string(
              "(Component_model_animator && "
              "Component_hitcapsule_group_set)"),
      })
{   // @NOTE: Do not remove adding concrete class to service finder!!
    BT_SERVICE_FINDER_ADD_SERVICE(System_animator_driven_hitcapsule_set, this);
}

void BT::component_system::system::System_animator_driven_hitcapsule_set::invoke_system_inner(
    Component_lists_per_query&& comp_lists_per_query) const /*override*/
{
    for (auto comp_list : comp_lists_per_query[Q_IDX_COMP_LISTS_WITH_ANIMATOR_HITCAPSULE])
    {   // Get component handles.
        auto& comp_animator{ comp_list->get_component_handle<Component_model_animator>() };
        auto& comp_hitcapsule_grp_set{
            comp_list->get_component_handle<Component_hitcapsule_group_set>()
        };

        // Update whether capsules are enabled and keep capsules attached to connecting bone in
        // animator.
        comp_animator.animator->get_anim_frame_action_data_handle()
            .assign_hitcapsule_enabled_flags();

        mat4 game_obj_trans;
        comp_list->get_attached_game_obj().get_transform_handle().get_transform_as_mat4(
            game_obj_trans);

        // @TODO: @FIXME: If this could be physics thread dependent instead of having to depend on
        // the timing of the render thread, then that would be wonderful. It would be sooo much more
        // consistent  -Thea 2025/10/07
        std::vector<mat4s> joint_matrices;
        comp_animator.animator->get_anim_floored_frame_pose(joint_matrices);

        comp_animator.animator->get_anim_frame_action_data_handle().update_hitcapsule_transforms(
            game_obj_trans,
            joint_matrices);
    }
}





#if 0
#include "logger.h"
#include "scripts.h"

#include "../game_object/game_object.h"
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

        mat4 game_obj_trans;
        rend_obj->get_owning_game_obj().get_transform_handle().get_transform_as_mat4(game_obj_trans);

        // @TODO: @FIXME: If this could be physics thread dependent instead of having to depend on
        // the timing of the render thread, then that would be wonderful. It would be sooo much more
        // consistent  -Thea 2025/10/07
        std::vector<mat4s> joint_matrices;
        model_animator.get_anim_floored_frame_pose(joint_matrices);

        model_animator.get_anim_frame_action_data_handle().update_hitcapsule_transforms(
            game_obj_trans,
            joint_matrices);

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
#endif  // 0
