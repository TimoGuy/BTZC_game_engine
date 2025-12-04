#include "player_character_lock_onto_target.h"

#include "btglm.h"
#include "btlogger.h"
#include "cglm/vec2.h"
#include "game_system_logic/component/follow_camera.h"
#include "game_system_logic/component/health_stats.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "input_handler/input_handler.h"
#include "renderer/camera.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"
#include "uuid/uuid.h"

#include <cassert>


void BT::system::player_character_lock_onto_target()
{   // Exit early if simulation not running.
    if (!service_finder::find_service<world::World_properties_container>()
             .get_data_handle()
             .is_simulation_running)
        return;
    
    // Exit early if not in right camera view mode.
    auto& camera{ *service_finder::find_service<Renderer>().get_camera_obj() };
    if (!camera.is_follow_orbit())
        return;
    
    // ECS parts.
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };

    // Get locked on entity.
    bool found_player_character{ false };
    UUID* locked_on_entity{ nullptr };
    {
        size_t count{ 0 };
        for (auto&& [ecs_entity, follow_cam_follow_ref, transform] :
            reg.view<component::Follow_camera_follow_ref, component::Transform const>().each())
        {
            found_player_character = true;
            locked_on_entity = &follow_cam_follow_ref.state.locked_on_entity;
            count++;
        }
        assert(count <= 1);  // Enforce only one iteration (or exit system if no found pointer).
    }

    // Exit early if no player character.
    if (!found_player_character)
        return;

    // Input checks.
    bool on_lockon_press;
    {
        static bool s_prev_lockon_pressed{ false };
        bool lockon_pressed{
            service_finder::find_service<Input_handler>().get_input_state().cam_lock_on.val
        };
        on_lockon_press = (!s_prev_lockon_pressed && lockon_pressed);

        s_prev_lockon_pressed = lockon_pressed;
    }

    // Remove other character reference if (1) clicked lock off or (2) reference is broken.
    if (!locked_on_entity->is_nil())
    {
        if (on_lockon_press || !entity_container.entity_exists(*locked_on_entity))
        {   // Remove character reference.
            *locked_on_entity = UUID();
        }
    }
    // Look for other character if clicking lockon.
    else if (on_lockon_press)
    {
        UUID best_entity;
        float_t best_dot_prod{ std::sinf(glm_rad(45.0f)) };

        // Get facing dir of camera.
        vec3 cam_position;
        vec3 cam_facing_dir;
        {
            camera.get_position(cam_position);
            camera.get_view_direction(cam_facing_dir);

            assert(glm_eq(glm_vec3_norm2(cam_facing_dir), 1.0f));
        }

        for (auto&& [ecs_entity, transform, char_mvt_state] :
             reg.view<component::Transform const, component::Health_stats_data const>(
                    entt::exclude<component::Follow_camera_follow_ref>)
                 .each())
        {   // Loop thru to find best entity to focus on.
            vec3 trans_pos{ static_cast<float_t>(transform.position.x),  // @TODO: Conform to `write_render_transforms.cpp`
                            static_cast<float_t>(transform.position.y),
                            static_cast<float_t>(transform.position.z) };

            vec3 facing_to_trans;
            glm_vec3_sub(trans_pos, cam_position, facing_to_trans);
            glm_vec3_normalize(facing_to_trans);

            auto facing_dot_prod{ glm_vec3_dot(facing_to_trans, cam_facing_dir) };
            if (facing_dot_prod > best_dot_prod)
            {   // Found new best facing dir!
                best_entity   = entity_container.find_entity_uuid(ecs_entity);
                best_dot_prod = facing_dot_prod;
            }
        }

        if (!best_entity.is_nil())
        {   // Apply best entity.
            *locked_on_entity = best_entity;
        }
    }

    // Exit early if no locked on entity.
    if (locked_on_entity->is_nil())
        return;

    ////////////////////////////////////////////////////////////////////////////////////////////////

    // Move camera direction to following transform.
    vec3 follow_pos;
    camera.get_follow_orbit_follow_pos(follow_pos);

    vec3 locked_on_pos;
    {
        auto& transform{ reg.get<component::Transform const>(
            entity_container.find_entity(*locked_on_entity)) };

        // @TODO: Conform to `write_render_transforms.cpp`
        locked_on_pos[0] = static_cast<float_t>(transform.position.x);
        locked_on_pos[1] = static_cast<float_t>(transform.position.y) - 1.0f;  // @HARDCODE: @HACK: This just turns down the camera angle.  -Thea 2025/12/03
        locked_on_pos[2] = static_cast<float_t>(transform.position.z);
    }

    vec3 delta_pos;
    glm_vec3_sub(locked_on_pos, follow_pos, delta_pos);

    vec2 new_orbits;
    new_orbits[0] = std::atan2f(delta_pos[0], delta_pos[2]);
    new_orbits[1] = -std::atan2f(delta_pos[1], glm_vec2_norm(vec2{ delta_pos[0], delta_pos[2] }));
    camera.set_follow_orbit_orbits(new_orbits);
}
