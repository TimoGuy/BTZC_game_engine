#include "hitcapsule_attack_processing.h"

#include "btlogger.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/combat_stats.h"
#include "game_system_logic/component/health_stats.h"
#include "game_system_logic/entity_container.h"
#include "hitbox_interactor/hitcapsule.h"
#include "physics_engine/physics_engine.h"  // For `k_simulation_delta_time`.
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"


void BT::system::hitcapsule_attack_processing(float_t delta_time)
{
    static double_t s_attack_timer{ 0 };

    // Update hitcapsules.
    auto attack_pairs{
        service_finder::find_service<Hitcapsule_group_overlap_solver>().update_overlaps()
    };

    // Process all attacks.
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    auto offender_view{
        reg.view<component::Base_combat_stats_data const, component::Health_stats_data>()
    };
    auto defender_view{ reg.view<component::Health_stats_data>() };  // `Base_combat_stats_data` is optional.

    for (auto&& [offender_uuid, defender_uuid] : attack_pairs)
    {   // Get offender stats.
        auto offender_ecs_entity{ entity_container.find_entity(offender_uuid) };
        auto const& offe_combat_stats{ offender_view.get<component::Base_combat_stats_data const>(
            offender_ecs_entity) };
        auto& offe_health_stats{ offender_view.get<component::Health_stats_data>(
            offender_ecs_entity) };

        // Get defender stats.
        auto defender_ecs_entity{ entity_container.find_entity(defender_uuid) };
        auto& defe_health_stats{ offender_view.get<component::Health_stats_data>(
            defender_ecs_entity) };

        // @TODO: START HERE!!!!!
        assert(false);  // Use `s_attack_timer` to update last time an attack was received. (Use `atk_receive_debounce_time` and `prev_atk_received_time`)
    }

    // Update attack timer.
    s_attack_timer += delta_time;








#if 0
    auto view{ service_finder::find_service<Entity_container>()
                   .get_ecs_registry()
                   .view<component::Animator_driven_hitcapsule_set const,
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

        animator.get_anim_frame_action_data_handle().assign_hitcapsule_enabled_flags();

        std::vector<mat4s> joint_matrices;
        animator.get_anim_floored_frame_pose(Model_animator::SIMULATION_PROFILE, joint_matrices);

        animator.get_anim_frame_action_data_handle().update_hitcapsule_transforms(
            rend_obj.render_transform(),
            joint_matrices);

        rend_obj_pool.return_render_objs({ &rend_obj });
    }
#endif  // 0
}
