#include "animator_driven_hitcapsule_sets_update.h"

#include "animation_frame_action_tool/runtime_data.h"
#include "btlogger.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/animator_driven_hitcapsule_set.h"
#include "game_system_logic/component/animator_root_motion.h"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "physics_engine/physics_engine.h"  // For `k_simulation_delta_time`.
#include "renderer/model_animator.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"


void BT::system::animator_driven_hitcapsule_sets_update()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto view{ reg.view<component::Animator_driven_hitcapsule_set const,
                        component::Transform const,
                        component::Created_render_object_reference const>() };
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };

    // Work with tagged entities.
    for (auto entity : view)
    {
        auto const& rend_obj_ref{ view.get<component::Created_render_object_reference const>(
            entity) };
        auto& rend_obj{
            *rend_obj_pool.checkout_render_obj_by_key({ rend_obj_ref.render_obj_uuid_ref }).front()
        };

        // Update whether capsules are enabled and keep capsules attached to connecting bone in
        // animator.
        auto& animator{ *rend_obj.get_model_animator() };
        animator.update(Model_animator::SIMULATION_PROFILE,
                        Physics_engine::k_simulation_delta_time);

        auto& anim_afa_data_handle{ animator.get_anim_frame_action_data_handle() };

        anim_afa_data_handle.assign_hitcapsule_enabled_flags();

        std::vector<mat4s> joint_matrices;
        if (animator.get_is_using_root_motion())
        {   // @NOTE: This below will lag behind 1 sim-tick. @TODO: @THEA: @NOCHECKIN: FIX THIS!!!!!
            auto& anim_root_motion{ reg.get<component::Animator_root_motion>(entity) };
            anim_root_motion.turn_speed =
                anim_afa_data_handle
                    .get_float_data_handle(anim_frame_action::CTRL_DATA_LABEL_turn_speed)
                    .get_val();
            anim_root_motion.can_do_turnaround_anim =
                anim_afa_data_handle
                    .get_bool_data_handle(anim_frame_action::CTRL_DATA_LABEL_can_do_turnaround_anim)
                    .get_val();
            animator.get_anim_floored_frame_pose_with_root_motion(
                Model_animator::SIMULATION_PROFILE,
                anim_root_motion.delta_pos,  // Different system will use this information.
                joint_matrices);
        }
        else
            animator.get_anim_floored_frame_pose(Model_animator::SIMULATION_PROFILE,
                                                 joint_matrices);

        anim_afa_data_handle.update_hitcapsule_transforms(
            rend_obj.render_transform(),
            joint_matrices);

        rend_obj_pool.return_render_objs({ &rend_obj });
    }
}
