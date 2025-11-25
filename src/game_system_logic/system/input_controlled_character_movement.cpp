#include "input_controlled_character_movement.h"

#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "Jolt/Math/Vec3.h"
#include "btglm.h"
#include "game_system_logic/component/character_movement.h"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "physics_engine/raycast_helper.h"
#include "service_finder/service_finder.h"

#include <cassert>


namespace
{

using namespace BT;

/// Struct holding origin offset and input direction.
struct Origin_offset_and_input_dir
{
    JPH::Vec3 origin_offset;
    JPH::Vec3 input_dir;
};

/// Calculates an origin offset and input direction from the parameters of the character controller.
Origin_offset_and_input_dir calc_check_origin_point_and_input_dir(float_t facing_angle,
                                                                  float_t char_con_radius)
{
    JPH::Vec3 flat_input_dir{ sinf(facing_angle), 0.0f, cosf(facing_angle) };
    float_t cancel_ratio{ char_con_radius /
                          std::max(abs(flat_input_dir.GetX()), abs(flat_input_dir.GetZ())) };
    JPH::Vec3 resized_to_radius_box_velo{ flat_input_dir * cancel_ratio };

    return { resized_to_radius_box_velo, flat_input_dir };
}

void process_midair_jump_interactions(
    component::Character_mvt_state::Airborne_state const& airborne_state,
    component::Character_mvt_state::Settings const& mvt_settings,
    Physics_object_type_impl_ifc* char_con_impl,
    JPH::Vec3 up_direction,
    JPH::Vec3& new_velocity)
{
    // Wall interactions.
    // @NOTE: Check for a ledge climb first, and then check for a wall
    //   jump if the ledge climb fails.

    #define REFACTOR_WALL_INTERACTIONS 1
    #if REFACTOR_WALL_INTERACTIONS
    bool ledge_climb_failed{ false };

    float_t char_con_radius{ char_con_impl->get_cc_radius() };

    // Calc check origin point (since char collider is a cube).
    auto opaid{ calc_check_origin_point_and_input_dir(airborne_state.input_facing_angle,
                                                      char_con_radius) };
    JPH::RVec3 check_origin_point{ char_con_impl->read_transform().position + opaid.origin_offset };
    JPH::Vec3 flat_input_dir{ opaid.input_dir };

    // Check for ledge climb.
    float_t max_ledge_search_length{ 1.5f };
    float_t min_ledge_search_length{ char_con_radius };
    assert(max_ledge_search_length > min_ledge_search_length);
    assert(max_ledge_search_length > 0.0f);
    assert(min_ledge_search_length > 0.0f);
    
    bool passed_reach_empty_test{ false };
    float_t rea_emp_test_passed_check_reach_height;
    JPH::RVec3 rea_emp_test_passed_check_origin;

    float_t char_con_height{ char_con_impl->get_cc_height() };
    JPH::Vec3 flat_normal_input_dir{ flat_input_dir.Normalized() };
    float_t reach_distance{ char_con_radius };

    for (auto reach_extra_height : { 0.0f, 0.5f, 1.0f, 1.5f })
    {
        JPH::RVec3 top_check_origin_point{
            check_origin_point
            + JPH::Vec3{ 0.0f,
                         (char_con_height * 0.5f) + reach_extra_height,
                         0.0f } };
        auto data{ Raycast_helper::raycast(top_check_origin_point,
                                           flat_normal_input_dir * max_ledge_search_length) };
        if (!data.success ||
            data.hit_distance > min_ledge_search_length)
        {
            // Empty space found, reach-empty test passed!
            passed_reach_empty_test = true;
            rea_emp_test_passed_check_reach_height = reach_extra_height;
            rea_emp_test_passed_check_origin = top_check_origin_point;
            break;
        }
    }

    bool passed_ledge_search_test{ false };
    JPH::RVec3 led_sea_test_passed_target_pos;

    if (passed_reach_empty_test)
    {
        float_t search_dist{ max_ledge_search_length - min_ledge_search_length };

        for (auto search_depth : { min_ledge_search_length + 0.0f * search_dist,
                                   min_ledge_search_length + 0.333f * search_dist,
                                   min_ledge_search_length + 0.667f * search_dist,
                                   min_ledge_search_length + 1.0f * search_dist })
        {
            JPH::RVec3 ledge_check_origin_point{ rea_emp_test_passed_check_origin
                                                    + search_depth * flat_normal_input_dir };
            auto data{ Raycast_helper::raycast(ledge_check_origin_point,
                                               JPH::Vec3{ 0.0f,
                                                          -(rea_emp_test_passed_check_reach_height
                                                                + char_con_height),
                                                          0.0f }) };
            if (data.success)
            {
                passed_ledge_search_test = true;
                led_sea_test_passed_target_pos = (data.hit_point - opaid.origin_offset);
            }
        }
    }

    if (passed_reach_empty_test && passed_ledge_search_test)
    {
        // Commit to ledge climb.
        // @TEMPORARY: I think in the future a simple timed animation could be good here.
        char_con_impl->move_kinematic({ led_sea_test_passed_target_pos,
                                         char_con_impl->read_transform().rotation });
        new_velocity = JPH::Vec3::sZero();
    }
    else
    {
        ledge_climb_failed = true;
    }
    ////////////////////////

    if (ledge_climb_failed && new_velocity.GetY() <= 0.0f)
    {
        // Try wall jump.
        // @NOTE: This block may not execute if the player is not falling. Especially
        //   if looking at the debug lines and it doesn't seem right, check if the player
        //   is falling when pressing the jump btn again.  -Thea 2025/07/04
        constexpr uint32_t k_num_circle_raycasts{ 9 };
        for (uint32_t i = 0; i < k_num_circle_raycasts; i++)
        {
            auto opaid{
                calc_check_origin_point_and_input_dir(airborne_state.input_facing_angle
                                                          + (i * glm_rad(360.0f) / k_num_circle_raycasts),
                                                      char_con_radius) };

            auto data{ Raycast_helper::raycast(char_con_impl->read_transform().position + opaid.origin_offset,
                                               1.5f * opaid.input_dir) };
            if (data.success)
            {
                // Commit to wall jump.
                // @TEMPORARY: I think holding the player to the wall for just a little bit would be good to help the animation.
                JPH::Vec3 curr_up_velo{ up_direction * up_direction.Dot(new_velocity) };
                new_velocity += -curr_up_velo + mvt_settings.jump_speed * up_direction;
                break;
            }
        }
    }

    #endif  // REFACTOR_WALL_INTERACTIONS
}

/// Swithes ground turn speed depending on the running speed/linear speed.
float_t find_grounded_turn_speed(component::Character_mvt_state::Settings const& mvt_settings,
                                 float_t linear_speed)
{
    float_t turn_speed{ 0.0f };
    for (auto& context : mvt_settings.grounded_turn_speeds)
        if (linear_speed <= context.max_speed_of_context)
        {
            turn_speed = context.turn_speed;
            break;
        }

    return turn_speed;
}

/// Processes input to turn the character when in the grounded state.
void apply_grounded_facing_angle(component::Character_mvt_state::Grounded_state& grounded_state,
                                 component::Character_mvt_state::Settings const& mvt_settings,
                                 JPH::Vec3Arg input_velocity)
{
    float_t desired_facing_angle{ atan2f(input_velocity.GetX(), input_velocity.GetZ()) };

    float_t delta_direction{ desired_facing_angle - grounded_state.facing_angle };
    while (delta_direction > glm_rad(180.0f)) delta_direction -= glm_rad(360.0f);
    while (delta_direction <= glm_rad(-180.0f)) delta_direction += glm_rad(360.0f);

    float_t turn_speed{ find_grounded_turn_speed(mvt_settings, grounded_state.speed) };
    bool is_quick_turn_speed{ turn_speed > 1000.0f };  // Idk just some number.

    if (is_quick_turn_speed)
    {
        // Disable turnaround mode.
        grounded_state.turnaround_enabled = false;
    }

    constexpr float_t k_turn_around_back_angle{ 45.0f };
    bool turnaround_mode{ grounded_state.turnaround_enabled ||
                          (!is_quick_turn_speed &&
                           abs(delta_direction) >
                               glm_rad(180.0f - (k_turn_around_back_angle * 0.5f))) };
    if (turnaround_mode)
    {
        // Lock in turnaround.
        grounded_state.turnaround_enabled = true;
    }
    else
    {
        // Process grounded turning.
        float_t max_turn_delta{ turn_speed * Physics_engine::k_simulation_delta_time };
        if (abs(delta_direction) > max_turn_delta)
        {
            // Limit turn speed.
            delta_direction = max_turn_delta * glm_signf(delta_direction);
        }

        // Apply new facing angle.
        grounded_state.facing_angle += delta_direction;
    }
}

/// Processes the linear speed after acceleration/deceleration accounted for.
void apply_grounded_linear_speed(component::Character_mvt_state::Grounded_state& grounded_state,
                                 component::Character_mvt_state::Settings const& mvt_settings,
                                 JPH::Vec3Arg input_velocity)
{
    float_t desired_speed{ glm_vec2_norm(vec2{ input_velocity.GetX(), input_velocity.GetZ() }) };

    if (grounded_state.turnaround_enabled)
    {   // Zero desired speed if doing turnaround.
        desired_speed = 0.0f;
    }

    float_t delta_speed{ desired_speed - grounded_state.speed };
    float_t acceleration{ delta_speed < 0.0f ? -mvt_settings.grounded_deceleration *
                                                   Physics_engine::k_simulation_delta_time
                                             : mvt_settings.grounded_acceleration *
                                                   Physics_engine::k_simulation_delta_time };
    if (abs(delta_speed) > abs(acceleration))
    {
        // Limit delta speed to acceleration.
        delta_speed = acceleration;
    }

    // Apply new linear speed.
    grounded_state.speed += delta_speed;
}

/// Result from movement logic.
struct Char_mvt_logic_results
{
    bool is_grounded;
    JPH::Quat up_rotation;
    JPH::Vec3 new_velocity;
    float_t display_facing_angle;
};

/// Character controller movement logic.
Char_mvt_logic_results character_controller_movement_logic(
    component::Character_world_space_input const& char_ws_input,
    component::Character_mvt_state& char_mvt_state,
    component::Character_mvt_animated_state* char_mvt_anim_state,
    Physics_object& phys_obj)
{   // Get current character controller state.
    auto char_con_impl{ phys_obj.get_impl() };

    JPH::Vec3 ground_velocity;
    JPH::Vec3 linear_velocity;
    JPH::Vec3 up_direction;
    JPH::Quat up_rotation;
    bool is_supported;
    JPH::CharacterVirtual::EGroundState ground_state;
    JPH::Vec3 ground_normal;
    bool is_crouched;
    char_con_impl->tick_fetch_cc_status(ground_velocity,
                                        linear_velocity,
                                        up_direction,
                                        up_rotation,
                                        is_supported,
                                        ground_state,
                                        ground_normal,
                                        is_crouched);

    up_rotation =
        JPH::Quat::sEulerAngles(JPH::Vec3(0, 0, 0));  // @NOCHECKIN: Overriding the up rot.

    // Change input into desired velocity.
    auto const& mvt_settings{ char_mvt_state.settings };

    JPH::Vec3 desired_velocity{ char_ws_input.ws_flat_clamped_input.x,
                                0.0f,
                                char_ws_input.ws_flat_clamped_input.z };
    desired_velocity *= (char_con_impl->get_cc_stance() ? mvt_settings.crouched_speed
                                                        : mvt_settings.standing_speed);

    // Extra input checks.
    bool on_jump_press{ char_ws_input.jump_pressed && !char_ws_input.prev_jump_pressed };
    bool on_crouch_press{ char_ws_input.crouch_pressed && !char_ws_input.prev_crouch_pressed };

    // Find movement state.
    JPH::Vec3 current_vertical_velocity = linear_velocity.Dot(up_direction) * up_direction;
    bool is_grounded{ ground_state == JPH::CharacterVirtual::EGroundState::OnGround &&
                      (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f &&
                      !char_con_impl->is_cc_slope_too_steep(ground_normal) };

    // Calc and apply desired velocity.
    JPH::Vec3 new_velocity;
    if (is_grounded)
    {   // Grounded.
        new_velocity = ground_velocity;

        if (on_crouch_press || (char_con_impl->get_cc_stance() && on_jump_press))
        {   // Toggle crouching.
            char_con_impl->set_cc_stance(!char_con_impl->get_cc_stance());
        }
        else if (!char_con_impl->get_cc_stance() && on_jump_press)
        {   // Jump.
            new_velocity += mvt_settings.jump_speed * up_direction;
        }
    }
    else
    {   // Start with previous tick velocity.
        new_velocity = current_vertical_velocity;

        if (char_con_impl->get_cc_stance())
        {   // Leave crouching stance automatically.
            char_con_impl->set_cc_stance(false);
        }

        if (on_jump_press)
        {
            process_midair_jump_interactions(char_mvt_state.airborne_state,
                                             mvt_settings,
                                             char_con_impl,
                                             up_direction,
                                             new_velocity);
        }
    }

    // Desired velocity.
    float_t display_facing_angle;
    if (is_grounded)
    {   // Grounded turn & speed movement.
        auto& grounded_state{ char_mvt_state.grounded_state };

        auto input_norm2{ glm_vec3_norm2(
            const_cast<float_t*>(char_ws_input.ws_flat_clamped_input.raw)) };
        if (input_norm2 > 1e-6f * 1e-6f)
            apply_grounded_facing_angle(grounded_state, mvt_settings, desired_velocity);

        if (char_mvt_anim_state)
            char_mvt_anim_state->write_to_animator_data.is_moving = (input_norm2 > 0.5f * 0.5f);

        apply_grounded_linear_speed(grounded_state, mvt_settings, desired_velocity);

        new_velocity += JPH::Vec3{ sinf(grounded_state.facing_angle) * grounded_state.speed,
                                   0.0f,
                                   cosf(grounded_state.facing_angle) * grounded_state.speed };

        // Keep airborne state up to date.
        char_mvt_state.airborne_state.input_facing_angle = grounded_state.facing_angle;

        display_facing_angle = grounded_state.facing_angle;
    }
    else
    {   // Move towards desired velocity.
        auto& airborne_state{ char_mvt_state.airborne_state };

        JPH::Vec3 flat_linear_velo{ linear_velocity.GetX(), 0.0f, linear_velocity.GetZ() };
        JPH::Vec3 delta_velocity{ desired_velocity - flat_linear_velo };

        float_t airborne_accel{ mvt_settings.airborne_acceleration *
                                Physics_engine::k_simulation_delta_time };
        if (delta_velocity.LengthSq() > airborne_accel * airborne_accel)
        {
            delta_velocity = delta_velocity.Normalized() * airborne_accel;
        }

        JPH::Vec3 effective_velocity{ flat_linear_velo + delta_velocity };
        new_velocity += effective_velocity;

        if (glm_vec3_norm2(const_cast<float_t*>(char_ws_input.ws_flat_clamped_input.raw)) >
            1e-6f * 1e-6f)
        {   // Move towards input angle.
            float_t desired_facing_angle{ atan2f(char_ws_input.ws_flat_clamped_input.x,
                                                 char_ws_input.ws_flat_clamped_input.z) };
            float_t delta_direction{ desired_facing_angle - airborne_state.input_facing_angle };

            while (delta_direction > glm_rad(180.0f))
                delta_direction -= glm_rad(360.0f);
            while (delta_direction <= glm_rad(-180.0f))
                delta_direction += glm_rad(360.0f);

            float_t max_turn_delta{ mvt_settings.airborne_turn_speed *
                                    Physics_engine::k_simulation_delta_time };
            if (abs(delta_direction) > max_turn_delta)
            {   // Limit turn speed.
                delta_direction = max_turn_delta * glm_signf(delta_direction);
            }

            airborne_state.input_facing_angle += delta_direction;
        }

        // Keep grounded state up to date.
        char_mvt_state.grounded_state.speed = effective_velocity.Length();
        if (effective_velocity.IsNearZero())
            char_mvt_state.grounded_state.facing_angle = airborne_state.input_facing_angle;
        else
            char_mvt_state.grounded_state.facing_angle =
                atan2f(effective_velocity.GetX(), effective_velocity.GetZ());

        char_mvt_state.grounded_state.turnaround_enabled = false;

        display_facing_angle = airborne_state.input_facing_angle;
    }

    return { is_grounded, up_rotation, new_velocity, display_facing_angle };
}

/// Applies desired velocity to character controller.
void apply_velocity_to_char_con(
    component::Character_mvt_state::Grounded_state const& grounded_state,
    Physics_object& phys_obj,
    bool is_grounded,
    JPH::QuatArg up_rotation,
    JPH::Vec3Arg physics_gravity,
    JPH::Vec3Arg velocity)
{
    JPH::Vec3 velo_w_grav{ velocity };
    velo_w_grav += (up_rotation * physics_gravity) * Physics_engine::k_simulation_delta_time;

    phys_obj.get_impl()->set_cc_allow_sliding(is_grounded && (grounded_state.speed > 1e-6f));
    phys_obj.get_impl()->set_cc_velocity(velo_w_grav);
}

}  // namespace


void BT::system::input_controlled_character_movement()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Character_world_space_input const,
                        component::Character_mvt_state,
                        component::Created_physics_object_reference const>() };

    // Process all character movements.
    for (auto entity : view)
    {   // Get input and character movement state.
        auto const& char_ws_input{ view.get<component::Character_world_space_input const>(entity) };
        auto& char_mvt_state{ view.get<component::Character_mvt_state>(entity) };

        // Get char anim state.
        auto char_mvt_anim_state{ reg.try_get<component::Character_mvt_animated_state>(entity) };

        // Process input into character movement logic.
        auto& phys_engine{ service_finder::find_service<Physics_engine>() };
        auto phys_obj_uuid{
            view.get<component::Created_physics_object_reference const>(entity).physics_obj_uuid_ref
        };
        auto& phys_obj{ *phys_engine.checkout_physics_object(phys_obj_uuid) };

        auto mvt_logic_result = character_controller_movement_logic(char_ws_input,
                                                                    char_mvt_state,
                                                                    char_mvt_anim_state,
                                                                    phys_obj);

        // Apply movement logic outputs to physics object character controller inputs.
        auto const& physics_gravity{
            reinterpret_cast<JPH::PhysicsSystem*>(
                service_finder::find_service<Physics_engine>().get_physics_system_ptr())
                ->GetGravity()
        };
        apply_velocity_to_char_con(char_mvt_state.grounded_state,
                                   phys_obj,
                                   mvt_logic_result.is_grounded,
                                   mvt_logic_result.up_rotation,
                                   physics_gravity,
                                   mvt_logic_result.new_velocity);

        phys_engine.return_physics_object(&phys_obj);

        // Try writing a new facing direction.
        if (auto poss_display_repr_ref{
                reg.try_get<component::Display_repr_transform_ref>(entity) };
            poss_display_repr_ref != nullptr)
        {   // Calculate rotation.
            versors rot;
            glm_quat(rot.raw, mvt_logic_result.display_facing_angle, 0.0f, 1.0f, 0.0f);

            // Write to display repr entity transform.
            auto display_repr_ecs_ent{ entity_container.find_entity(
                poss_display_repr_ref->display_repr_uuid) };
            component::submit_transform_change_only_rotation_helper(reg, display_repr_ecs_ent, rot);
        }
    }
}
