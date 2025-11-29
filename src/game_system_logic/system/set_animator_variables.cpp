#include "set_animator_variables.h"

#include "game_system_logic/component/animator_root_motion.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/entity_container.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"

#include <cassert>


void BT::system::set_animator_variables()
{
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    {   // Character mvt animators.
        auto view{ reg.view<component::Character_mvt_animated_state>() };
        for (auto entity : view)
        {   // Get animator.
            auto& char_mvt_anim_state{ view.get<component::Character_mvt_animated_state>(entity) };

            auto affecting_rend_obj_ecs_entity{ entity_container.find_entity(
                char_mvt_anim_state.affecting_animator_uuid) };
            auto const affecting_rend_obj_ref{
                reg.try_get<component::Created_render_object_reference const>(
                    affecting_rend_obj_ecs_entity)
            };
            if (!affecting_rend_obj_ref)
                continue;  // Cancel bc no created render object.

            auto& affecting_rend_obj{ *rend_obj_pool
                                           .checkout_render_obj_by_key(
                                               { affecting_rend_obj_ref->render_obj_uuid_ref })
                                           .front() };

            auto animator{ affecting_rend_obj.get_model_animator() };
            if (!animator)
            {   // Cancel bc animator doesn't exist.
                rend_obj_pool.return_render_objs({ &affecting_rend_obj });
                continue;
            }

            // Set animator vars.
            animator->set_bool_variable("is_moving",
                                        char_mvt_anim_state.write_to_animator_data.is_moving);

            if (char_mvt_anim_state.write_to_animator_data.on_turnaround)
                animator->set_trigger_variable("on_turnaround");
            char_mvt_anim_state.write_to_animator_data.on_turnaround = false;

            animator->set_bool_variable("is_grounded",
                                        char_mvt_anim_state.write_to_animator_data.is_grounded);

            if (char_mvt_anim_state.write_to_animator_data.on_jump)
                animator->set_trigger_variable("on_jump");
            char_mvt_anim_state.write_to_animator_data.on_jump = false;

            if (char_mvt_anim_state.write_to_animator_data.on_attack)
                animator->set_trigger_variable("on_attack");
            char_mvt_anim_state.write_to_animator_data.on_attack = false;

            // Update aniamtor.
            animator->update(Model_animator::SIMULATION_PROFILE,
                             Physics_engine::k_simulation_delta_time);

            // Read animator root motion AFA data.
            if (animator->get_is_using_root_motion())
            {
                auto& anim_root_motion{ reg.get<component::Animator_root_motion>(
                    affecting_rend_obj_ecs_entity) };
                auto& anim_afa_data_handle{ animator->get_anim_frame_action_data_handle() };

                anim_root_motion.turn_speed =
                    anim_afa_data_handle
                        .get_float_data_handle(anim_frame_action::CTRL_DATA_LABEL_turn_speed)
                        .get_val();
                anim_root_motion.can_do_turnaround_anim =
                    anim_afa_data_handle
                        .get_bool_data_handle(
                            anim_frame_action::CTRL_DATA_LABEL_can_do_turnaround_anim)
                        .get_val();
                anim_root_motion.mvt_input.enabled =
                    anim_afa_data_handle
                        .get_bool_data_handle(anim_frame_action::CTRL_DATA_LABEL_mvt_input_enabled)
                        .get_val();
                anim_root_motion.mvt_input.max_speed =
                    anim_afa_data_handle
                        .get_float_data_handle(anim_frame_action::CTRL_DATA_LABEL_mvt_input_max_speed)
                        .get_val();
                anim_root_motion.mvt_input.accel =
                    anim_afa_data_handle
                        .get_float_data_handle(anim_frame_action::CTRL_DATA_LABEL_mvt_input_accel)
                        .get_val();
                anim_root_motion.mvt_input.decel =
                    anim_afa_data_handle
                        .get_float_data_handle(anim_frame_action::CTRL_DATA_LABEL_mvt_input_decel)
                        .get_val();
                animator->get_anim_root_motion_delta_pos(Model_animator::SIMULATION_PROFILE,
                                                         anim_root_motion.delta_pos);
            }

            // Finish.
            rend_obj_pool.return_render_objs({ &affecting_rend_obj });
        }
    }
}
