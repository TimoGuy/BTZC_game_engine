#include "hitcapsule_attack_processing.h"

#include "animation_frame_action_tool/runtime_data.h"
#include "btlogger.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/combat_stats.h"
#include "game_system_logic/component/health_stats.h"
#include "game_system_logic/component/render_object_settings.h"
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
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };

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

        // Update attack timer.
        bool attack_process_allowed{ false };
        if (defe_health_stats.prev_atk_received_time +
                defe_health_stats.atk_receive_debounce_time <=
            s_attack_timer)
        {   // Attack allowed!!
            attack_process_allowed = true;
            defe_health_stats.prev_atk_received_time = s_attack_timer;
        }

        // Skip this attack pair if attack is not allowed.
        if (!attack_process_allowed)
            continue;

        // Attack process logic.
        struct Attack_hit_result
        {
            struct Result_data
            {
                int32_t delta_hit_pts{ 0 };
                int32_t delta_posture_pts{ 0 };
                bool can_enter_posture_break{ true };
            };
            Result_data defender;
            Result_data offender;
        } atk_res;

        atk_res.defender.delta_hit_pts     = -offe_combat_stats.dmg_pts;
        atk_res.defender.delta_posture_pts = offe_combat_stats.posture_dmg_pts;

        if (auto try_get_defe_combat_stats{
                reg.try_get<component::Base_combat_stats_data const>(defender_ecs_entity) };
            try_get_defe_combat_stats != nullptr)
        {   // Defend against offender's attack.
            auto& defe_combat_stats{ *try_get_defe_combat_stats };

            atk_res.defender.delta_hit_pts += defe_combat_stats.dmg_def_pts;
            atk_res.defender.delta_posture_pts -= defe_combat_stats.posture_dmg_def_pts;
        }

        if (auto try_get_rend_obj_ref{
                reg.try_get<component::Created_render_object_reference const>(
                    defender_ecs_entity) };
            try_get_rend_obj_ref != nullptr)
        {   // Check for parry or guard.
            bool is_parry_active;
            bool is_guard_active;
            {
                auto& rend_obj{ *rend_obj_pool
                                     .checkout_render_obj_by_key(
                                         { try_get_rend_obj_ref->render_obj_uuid_ref })
                                     .front() };
                auto& animator{ *rend_obj.get_model_animator() };

                auto& afa_data_handle{ animator.get_anim_frame_action_data_handle() };
                is_parry_active =
                    afa_data_handle
                        .get_bool_data_handle(anim_frame_action::CTRL_DATA_LABEL_is_parry_active)
                        .get_val();
                is_guard_active =
                    afa_data_handle
                        .get_bool_data_handle(anim_frame_action::CTRL_DATA_LABEL_is_guard_active)
                        .get_val();

                rend_obj_pool.return_render_objs({ &rend_obj });
            }

            // Parry attack.
            if (is_parry_active)
            {
                atk_res.defender.delta_hit_pts = 0;
                atk_res.defender.delta_posture_pts *= 0.5;
                atk_res.defender.can_enter_posture_break = false;

                // Sendback posture pts to offender.
                atk_res.offender.delta_posture_pts = atk_res.defender.delta_posture_pts;
            }
            // Guard attack.
            else if (is_guard_active)
            {
                atk_res.defender.delta_hit_pts = 0;
            }
        }

        // Apply damage results.
        static auto const s_apply_dmg_results_fn =
            [](component::Health_stats_data& health_stats,
               Attack_hit_result::Result_data const& atk_res) {
                if (health_stats.is_invincible)
                    return;

                health_stats.health_pts += atk_res.delta_hit_pts;
                health_stats.health_pts = std::max(0, health_stats.health_pts);

                health_stats.posture_pts += atk_res.delta_posture_pts;
                health_stats.posture_pts = std::min(health_stats.max_posture_pts +
                                                        (atk_res.can_enter_posture_break ? 0 : -1),
                                                    health_stats.health_pts);
            };

        s_apply_dmg_results_fn(offe_health_stats, atk_res.offender);
        s_apply_dmg_results_fn(defe_health_stats, atk_res.defender);
    }

    // Update attack timer.
    s_attack_timer += delta_time;
}
