#include "set_animator_variables.h"

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

            auto const affecting_rend_obj_ref{
                reg.try_get<component::Created_render_object_reference const>(
                    entity_container.find_entity(char_mvt_anim_state.affecting_animator_uuid))
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
            animator->set_bool_variable("is_moving", char_mvt_anim_state.is_moving);

            if (char_mvt_anim_state.on_attack)
                animator->set_trigger_variable("on_attack");
            char_mvt_anim_state.on_attack = false;

            // Finish.
            rend_obj_pool.return_render_objs({ &affecting_rend_obj });
        }
    }
}
