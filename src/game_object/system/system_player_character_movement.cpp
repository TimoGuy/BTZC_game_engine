// clang-format off
#include "concrete_systems.h"
// clang-format on

#include "../../input_handler/input_handler.h"
#include "../../physics_engine/physics_engine.h"
#include "../../physics_engine/raycast_helper.h"
#include "../../renderer/camera.h"
#include "../../renderer/renderer.h"
#include "../../service_finder/service_finder.h"
#include "../component_registry.h"
#include "../components.h"
#include "Jolt/Jolt.h"
#include "Jolt/Math/Real.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "system_ifc.h"


namespace
{
enum : int32_t
{
    Q_IDX_COMP_LISTS_WITH_CHAR_CON_MVT_STATE,
};
}  // namespace


BT::component_system::system::System_player_character_movement::System_player_character_movement()
    : System_ifc({
          Component_list_query::compile_query_string(
              "(Component_physics_object && Component_char_con_movement_state)"),
      })
{   // @NOTE: Do not remove adding concrete class to service finder!!
    BT_SERVICE_FINDER_ADD_SERVICE(System_player_character_movement, this);
}

namespace
{
// Helper funcs.
using namespace BT;
using Component_char_con_movement_state = BT::component_system::Component_char_con_movement_state;

struct Origin_offset_and_input_dir
{
    JPH::Vec3 origin_offset;
    JPH::Vec3 input_dir;
};

Origin_offset_and_input_dir calc_check_origin_point_and_input_dir(float_t facing_angle,
                                                                  float_t char_con_radius)
{
    JPH::Vec3 flat_input_dir{ sinf(facing_angle),
                              0.0f,
                              cosf(facing_angle) };
    float_t cancel_ratio{ char_con_radius
                          / std::max(abs(flat_input_dir.GetX()),
                                     abs(flat_input_dir.GetZ())) };
    JPH::Vec3 resized_to_radius_box_velo{ flat_input_dir * cancel_ratio };

    return { resized_to_radius_box_velo, flat_input_dir };
}

float_t find_grounded_turn_speed(Component_char_con_movement_state& comp_char_con_mvt_state)
{
    float_t turn_speed{ 0.0f };
    for (auto& context : comp_char_con_mvt_state.grounded_turn_speeds)
        if (comp_char_con_mvt_state.grounded_state.speed <= context.max_speed_of_context)
        {
            turn_speed = context.turn_speed;
            break;
        }

    return turn_speed;
}

void apply_grounded_facing_angle(Component_char_con_movement_state& comp_char_con_mvt_state,
                                 JPH::Vec3Arg input_velocity)
{
    float_t desired_facing_angle{ atan2f(input_velocity.GetX(), input_velocity.GetZ()) };

    float_t delta_direction{ desired_facing_angle - comp_char_con_mvt_state.grounded_state.facing_angle };
    while (delta_direction > glm_rad(180.0f)) delta_direction -= glm_rad(360.0f);
    while (delta_direction <= glm_rad(-180.0f)) delta_direction += glm_rad(360.0f);

    float_t turn_speed{ find_grounded_turn_speed(comp_char_con_mvt_state) };
    bool is_quick_turn_speed{ turn_speed > 1000.0f };  // Idk just some number.

    if (is_quick_turn_speed)
    {
        // Disable turnaround mode.
        comp_char_con_mvt_state.grounded_state.turnaround_enabled = false;
    }

    constexpr float_t k_turn_around_back_angle{ 45.0f };
    bool turnaround_mode{ comp_char_con_mvt_state.grounded_state.turnaround_enabled ||
                          (!is_quick_turn_speed &&
                           abs(delta_direction) >
                               glm_rad(180.0f - (k_turn_around_back_angle * 0.5f))) };
    if (turnaround_mode)
    {
        // Lock in turnaround.
        comp_char_con_mvt_state.grounded_state.turnaround_enabled = true;
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
        comp_char_con_mvt_state.grounded_state.facing_angle += delta_direction;
    }
}

void apply_grounded_linear_speed(Component_char_con_movement_state& comp_char_con_mvt_state,
                                 JPH::Vec3Arg input_velocity)
{
    float_t desired_speed{ glm_vec2_norm(vec2{ input_velocity.GetX(), input_velocity.GetZ() }) };

    if (comp_char_con_mvt_state.grounded_state.turnaround_enabled)
    {   // Zero desired speed if doing turnaround.
        desired_speed = 0.0f;
    }

    float_t delta_speed{ desired_speed - comp_char_con_mvt_state.grounded_state.speed };
    float_t acceleration{ delta_speed < 0.0f ?
                          -comp_char_con_mvt_state.grounded_deceleration * Physics_engine::k_simulation_delta_time :
                          comp_char_con_mvt_state.grounded_acceleration * Physics_engine::k_simulation_delta_time };
    if (abs(delta_speed) > abs(acceleration))
    {
        // Limit delta speed to acceleration.
        delta_speed = acceleration;
    }

    // Apply new linear speed.
    comp_char_con_mvt_state.grounded_state.speed += delta_speed;
}

void process_midair_jump_interactions(
    Component_char_con_movement_state& comp_char_con_mvt_state,
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
    auto opaid{ calc_check_origin_point_and_input_dir(comp_char_con_mvt_state.airborne_state.input_facing_angle,
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
                calc_check_origin_point_and_input_dir(comp_char_con_mvt_state.airborne_state.input_facing_angle
                                                          + (i * glm_rad(360.0f) / k_num_circle_raycasts),
                                                      char_con_radius) };

            auto data{ Raycast_helper::raycast(char_con_impl->read_transform().position + opaid.origin_offset,
                                               1.5f * opaid.input_dir) };
            if (data.success)
            {
                // Commit to wall jump.
                // @TEMPORARY: I think holding the player to the wall for just a little bit would be good to help the animation.
                JPH::Vec3 curr_up_velo{ up_direction * up_direction.Dot(new_velocity) };
                new_velocity += -curr_up_velo + comp_char_con_mvt_state.jump_speed * up_direction;
                break;
            }
        }
    }

    #endif  // REFACTOR_WALL_INTERACTIONS
}

}  // namespace

void BT::component_system::system::System_player_character_movement::invoke_system_inner(
    Component_lists_per_query&& comp_lists_per_query) const /*override*/
{   // Get data from services.
    auto const& input_state{ service_finder::find_service<Input_handler>().get_input_state() };
    auto camera{ service_finder::find_service<Renderer>().get_camera_obj() };
    auto const& physics_gravity{
        reinterpret_cast<JPH::PhysicsSystem*>(
            service_finder::find_service<Physics_engine>().get_physics_system_ptr())
            ->GetGravity()
    };

    for (auto comp_list : comp_lists_per_query[Q_IDX_COMP_LISTS_WITH_CHAR_CON_MVT_STATE])
    {   // Get component handles.
        auto& comp_phys_obj{
            comp_list->get_component_handle<Component_physics_object>()
        };
        auto& comp_char_con_mvt_state{
            comp_list->get_component_handle<Component_char_con_movement_state>()
        };

        // @NOCHECKIN: @THEA.
        // Physics_object* phys_obj{ m_phys_engine.checkout_physics_object(m_phys_obj_key) };

        // Tick and get character status.
        auto char_con_impl{ comp_phys_obj.phys_obj->get_impl() };

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

        up_rotation = JPH::Quat::sEulerAngles(JPH::Vec3(0, 0, 0));  // @NOCHECKIN: Overriding the up rot.

        // Transform input to world space.
        vec3 cam_forward;
        camera->get_view_direction(cam_forward);
        cam_forward[1] = 0.0f;
        glm_vec3_normalize(cam_forward);

        vec3 cam_right;
        glm_vec3_crossn(cam_forward, vec3{ 0.0f, 1.0f, 0.0f }, cam_right);

        vec3 move_input_world = GLM_VEC3_ZERO_INIT;
        glm_vec3_muladds(cam_right, input_state.move.x.val, move_input_world);
        glm_vec3_muladds(cam_forward, input_state.move.y.val, move_input_world);
        move_input_world[1] = 0.0f;

        if (!camera->is_follow_orbit())
            // Remove all input if not the orbit camera mode.
            glm_vec3_zero(move_input_world);
        else if (glm_vec3_norm2(move_input_world) > 1.0f)
            // Clamp top input amount.
            glm_vec3_normalize(move_input_world);

        // Change input into desired velocity.
        JPH::Vec3 desired_velocity{ move_input_world[0], 0.0f, move_input_world[2] };
        desired_velocity *=
            (char_con_impl->get_cc_stance() ? comp_char_con_mvt_state.crouched_speed
                                            : comp_char_con_mvt_state.standing_speed);

        // Extra input checks.
        bool on_jump_press{ input_state.jump.val &&
                            !comp_char_con_mvt_state.prev_jump_pressed };
        comp_char_con_mvt_state.prev_jump_pressed = input_state.jump.val;

        bool on_crouch_press{ input_state.crouch.val &&
                              !comp_char_con_mvt_state.prev_crouch_pressed };
        comp_char_con_mvt_state.prev_crouch_pressed = input_state.crouch.val;

        ////////////////////////////////////////////////////////////////////////////////////////////

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
                new_velocity += comp_char_con_mvt_state.jump_speed * up_direction;
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
                process_midair_jump_interactions(comp_char_con_mvt_state, char_con_impl, up_direction, new_velocity);
            }
        }

        // Desired velocity.
        float_t display_facing_angle;
        if (is_grounded)
        {   // Grounded turn & speed movement.
            if (glm_vec3_norm2(move_input_world) > 1e-6f * 1e-6f)
                apply_grounded_facing_angle(comp_char_con_mvt_state, desired_velocity);

            apply_grounded_linear_speed(comp_char_con_mvt_state, desired_velocity);

            new_velocity += JPH::Vec3{ sinf(comp_char_con_mvt_state.grounded_state.facing_angle) *
                                           comp_char_con_mvt_state.grounded_state.speed,
                                       0.0f,
                                       cosf(comp_char_con_mvt_state.grounded_state.facing_angle) *
                                           comp_char_con_mvt_state.grounded_state.speed };

            // Keep airborne state up to date.
            comp_char_con_mvt_state.airborne_state.input_facing_angle = comp_char_con_mvt_state.grounded_state.facing_angle;

            display_facing_angle = comp_char_con_mvt_state.grounded_state.facing_angle;
        }
        else
        {   // Move towards desired velocity.
            JPH::Vec3 flat_linear_velo{ linear_velocity.GetX(),
                                        0.0f,
                                        linear_velocity.GetZ() };
            JPH::Vec3 delta_velocity{ desired_velocity - flat_linear_velo };

            float_t airborne_accel{ comp_char_con_mvt_state.airborne_acceleration *
                                    Physics_engine::k_simulation_delta_time };
            if (delta_velocity.LengthSq() > airborne_accel * airborne_accel)
            {
                delta_velocity = delta_velocity.Normalized() * airborne_accel;
            }

            JPH::Vec3 effective_velocity{ flat_linear_velo + delta_velocity };
            new_velocity += effective_velocity;

            if (glm_vec3_norm2(move_input_world) > 1e-6f * 1e-6f)
            {   // Move towards input angle.
                float_t desired_facing_angle{ atan2f(move_input_world[0], move_input_world[2]) };
                float_t delta_direction{
                    desired_facing_angle - comp_char_con_mvt_state.airborne_state.input_facing_angle
                };
                while (delta_direction > glm_rad(180.0f)) delta_direction -= glm_rad(360.0f);
                while (delta_direction <= glm_rad(-180.0f)) delta_direction += glm_rad(360.0f);

                float_t max_turn_delta{ comp_char_con_mvt_state.airborne_turn_speed *
                                        Physics_engine::k_simulation_delta_time };
                if (abs(delta_direction) > max_turn_delta)
                {
                    // Limit turn speed.
                    delta_direction = max_turn_delta * glm_signf(delta_direction);
                }

                comp_char_con_mvt_state.airborne_state.input_facing_angle += delta_direction;
            }

            // Keep grounded state up to date.
            comp_char_con_mvt_state.grounded_state.speed = effective_velocity.Length();
            if (effective_velocity.IsNearZero())
                comp_char_con_mvt_state.grounded_state.facing_angle =
                    comp_char_con_mvt_state.airborne_state.input_facing_angle;
            else
                comp_char_con_mvt_state.grounded_state.facing_angle =
                    atan2f(effective_velocity.GetX(), effective_velocity.GetZ());

            comp_char_con_mvt_state.grounded_state.turnaround_enabled = false;

            display_facing_angle = comp_char_con_mvt_state.airborne_state.input_facing_angle;
        }

        // Apply to character.
        new_velocity += (up_rotation * physics_gravity) * Physics_engine::k_simulation_delta_time;
        char_con_impl->set_cc_allow_sliding(is_grounded &&
                                            (comp_char_con_mvt_state.grounded_state.speed > 1e-6f));
        char_con_impl->set_cc_velocity(new_velocity);

        // Finish.
        // m_phys_engine.return_physics_object(phys_obj);



        // @TODO: @NOCHEKCIN: Implement a super simple facing direction.
        assert(false);

        // {   // Write new facing direction.
        //     auto& rs{ m_rendering_state };
        //     lock_guard<mutex> lock{ rs.access_mutex };
        //     rs.facing_angle_render_triple_buffer[rs.write_pos] = display_facing_angle;

        //     auto temp{ rs.read_a_pos };
        //     rs.read_a_pos = rs.read_b_pos;
        //     rs.read_b_pos = rs.write_pos;
        //     rs.write_pos = temp;
        // }
    }
}





#if 0
#include "cglm/quat.h"
#include "scripts.h"

#include "../game_object.h"
#include "../input_handler/input_handler.h"
#include "../physics_engine/physics_engine.h"
#include "../physics_engine/raycast_helper.h"
#include "../renderer/camera.h"
#include "../renderer/renderer.h"
#include "cglm/util.h"
#include "cglm/vec3.h"
#include "Jolt/Jolt.h"
#include "Jolt/Math/Real.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "logger.h"
#include <array>
#include <memory>
#include <mutex>

using std::array;
using std::lock_guard;
using std::mutex;
using std::unique_ptr;


namespace BT::Scripts
{

class Script_player_character_movement : public Script_ifc
{
public:
    Script_player_character_movement(Input_handler const& input_handler,
                                     Physics_engine& phys_engine,
                                     Renderer& renderer,
                                     Game_object_pool& game_obj_pool,
                                     UUID phys_obj_key,
                                     UUID apply_facing_angle_game_obj_key)
        : m_input_handler{ input_handler }
        , m_phys_engine{ phys_engine }
        , m_renderer{ renderer }
        , m_game_obj_pool{ game_obj_pool }
        , m_phys_obj_key{ phys_obj_key }
        , m_apply_facing_angle_game_obj_key{ apply_facing_angle_game_obj_key }
    {
    }

    Input_handler const& m_input_handler;
    Physics_engine& m_phys_engine;
    Renderer& m_renderer;
    Game_object_pool& m_game_obj_pool;
    UUID m_phys_obj_key;
    UUID m_apply_facing_angle_game_obj_key;
    bool m_prev_jump_pressed{ false };
    bool m_prev_crouch_pressed{ false };

    // enum Movement_state
    // {
    //     GROUNDED = 0,
    //     AIRBORNE,
    // } m_current_state{ Movement_state(0) };

    struct Grounded_state
    {
        float_t speed{ 0.0f };
        float_t facing_angle{ 0.0f };
        bool    turnaround_enabled{ false };
    } m_grounded_state;

    struct Airborne_state
    {
        float_t input_facing_angle{ 0.0f };
    } m_airborne_state;

    struct Rendering_state
    {
        // @NOTE: This structure may be used in a lot of places perhaps.
        //   Especially w/ multithreaded applications.
        mutex access_mutex;
        array<float_t, 3> facing_angle_render_triple_buffer{ 0.0f, 0.0f, 0.0f };
        uint8_t read_a_pos{ 0 };
        uint8_t read_b_pos{ 1 };
        uint8_t write_pos{ 2 };
    } m_rendering_state;

    // Script_ifc.
    Script_type get_type() override
    {
        return SCRIPT_TYPE_player_character_movement;
    }

    void serialize_datas(json& node_ref) override
    {
        node_ref["phys_obj_key"] = UUID_helper::to_pretty_repr(m_phys_obj_key);
        node_ref["apply_facing_angle_game_obj_key"] = UUID_helper::to_pretty_repr(m_apply_facing_angle_game_obj_key);
    }

    void on_pre_physics(float_t physics_delta_time) override;
    void on_pre_render(float_t delta_time) override;

    // Helper funcs.
    float_t find_grounded_turn_speed(float_t linear_speed);
    void apply_grounded_facing_angle(JPH::Vec3Arg input_velocity, float_t physics_delta_time);
    void apply_grounded_linear_speed(JPH::Vec3Arg input_velocity, float_t physics_delta_time);

    void process_midair_jump_interactions(Physics_object_type_impl_ifc* character_impl,
                                          JPH::Vec3 up_direction,
                                          JPH::Vec3& new_velocity);
    struct Origin_offset_and_input_dir
    {
        JPH::Vec3 origin_offset;
        JPH::Vec3 input_dir;
    };
    static Origin_offset_and_input_dir calc_check_origin_point_and_input_dir(
        float_t facing_angle,
        float_t char_con_radius);

private:
    struct Settings
    {
        float_t crouched_speed{ 5.0f };
        float_t standing_speed{ 15.0f };

        float_t grounded_acceleration{ 80.0f };
        float_t grounded_deceleration{ 120.0f };

        struct Contextual_turn_speed
        {
            float_t turn_speed;
            float_t max_speed_of_context;
        };
        array<Contextual_turn_speed, 3> grounded_turn_speeds{
            Contextual_turn_speed{ 1000000.0f, crouched_speed + 0.1f },
            Contextual_turn_speed{ 10.0f, standing_speed + 0.1f },
            Contextual_turn_speed{ 5.0f, 50.0f } };

        float_t airborne_acceleration{ 60.0f };
        float_t airborne_turn_speed{ 7.5f };
        float_t jump_speed{ 30.0f };
    } m_settings;
};

}  // namespace BT


// Create script.
unique_ptr<BT::Scripts::Script_ifc>
BT::Scripts::Factory_impl_funcs
    ::create_script_player_character_movement_from_serialized_datas(Input_handler* input_handler,
                                                                    Physics_engine* phys_engine,
                                                                    Renderer* renderer,
                                                                    Game_object_pool* game_obj_pool,
                                                                    json const& node_ref)
{
    return unique_ptr<Script_ifc>(
        new Script_player_character_movement{ *input_handler,
                                              *phys_engine,
                                              *renderer,
                                              *game_obj_pool,
                                              UUID_helper::to_UUID(node_ref["phys_obj_key"]),
                                              UUID_helper::to_UUID(node_ref["apply_facing_angle_game_obj_key"]) });
}


// Script_player_character_movement.
void BT::Scripts::Script_player_character_movement::on_pre_physics(float_t physics_delta_time)
{
    auto const& input_state{ m_input_handler.get_input_state() };
    auto camera{ m_renderer.get_camera_obj() };

    Physics_object* phys_obj{ m_phys_engine.checkout_physics_object(m_phys_obj_key) };
    auto character_impl{ phys_obj->get_impl() };

    auto phys_system{ reinterpret_cast<JPH::PhysicsSystem*>(m_phys_engine.get_physics_system_ptr()) };

    // Tick and get character status.
    JPH::Vec3 ground_velocity;
    JPH::Vec3 linear_velocity;
    JPH::Vec3 up_direction;
    JPH::Quat up_rotation;
    bool is_supported;
    JPH::CharacterVirtual::EGroundState ground_state;
    JPH::Vec3 ground_normal;
    bool is_crouched;
    character_impl->tick_fetch_cc_status(ground_velocity,
                                         linear_velocity,
                                         up_direction,
                                         up_rotation,
                                         is_supported,
                                         ground_state,
                                         ground_normal,
                                         is_crouched);

    up_rotation = JPH::Quat::sEulerAngles(JPH::Vec3(0, 0, 0));  // @NOCHECKIN: Overriding the up rot.

    // Transform input to world space.
    vec3 cam_forward;
    camera->get_view_direction(cam_forward);
    cam_forward[1] = 0.0f;
    glm_vec3_normalize(cam_forward);

    vec3 cam_right;
    glm_vec3_crossn(cam_forward, vec3{ 0.0f, 1.0f, 0.0f }, cam_right);

    vec3 move_input_world = GLM_VEC3_ZERO_INIT;
    glm_vec3_muladds(cam_right, input_state.move.x.val, move_input_world);
    glm_vec3_muladds(cam_forward, input_state.move.y.val, move_input_world);
    move_input_world[1] = 0.0f;

    if (!camera->is_follow_orbit())
        // Remove all input if not the orbit camera mode.
        glm_vec3_zero(move_input_world);
    else if (glm_vec3_norm2(move_input_world) > 1.0f)
        // Clamp top input amount.
        glm_vec3_normalize(move_input_world);

    // Change input into desired velocity.
    JPH::Vec3 desired_velocity{ move_input_world[0],
                                0.0f,
                                move_input_world[2] };
    desired_velocity *= (character_impl->get_cc_stance() ?
                             m_settings.crouched_speed :
                             m_settings.standing_speed);

    // Extra input checks.
    bool on_jump_press{ input_state.jump.val && !m_prev_jump_pressed };
    m_prev_jump_pressed = input_state.jump.val;

    bool on_crouch_press{ input_state.crouch.val && !m_prev_crouch_pressed };
    m_prev_crouch_pressed = input_state.crouch.val;

    // Find movement state.
    JPH::Vec3 current_vertical_velocity = linear_velocity.Dot(up_direction) * up_direction;
    bool is_grounded{ ground_state == JPH::CharacterVirtual::EGroundState::OnGround &&
                      (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f &&
                      !character_impl->is_cc_slope_too_steep(ground_normal) };

    // Calc and apply desired velocity.
    JPH::Vec3 new_velocity;
    if (is_grounded)
    {
        // Grounded.
        new_velocity = ground_velocity;

        if (on_crouch_press || (character_impl->get_cc_stance() && on_jump_press))
        {
            // Toggle crouching.
            character_impl->set_cc_stance(!character_impl->get_cc_stance());
        }
        else if (!character_impl->get_cc_stance() && on_jump_press)
        {
            // Jump.
            new_velocity += m_settings.jump_speed * up_direction;
        }
    }
    else
    {
        // Start with previous tick velocity.
        new_velocity = current_vertical_velocity;

        if (character_impl->get_cc_stance())
        {
            // Leave crouching stance automatically.
            character_impl->set_cc_stance(false);
        }

        if (on_jump_press)
        {
            process_midair_jump_interactions(character_impl,
                                             up_direction,
                                             new_velocity);
        }
    }

    // Desired velocity.
    float_t display_facing_angle;
    if (is_grounded)
    {
        // Grounded turn & speed movement.
        if (glm_vec3_norm2(move_input_world) > 1e-6f * 1e-6f)
            apply_grounded_facing_angle(desired_velocity, physics_delta_time);
        apply_grounded_linear_speed(desired_velocity, physics_delta_time);
        new_velocity +=
            JPH::Vec3{ sinf(m_grounded_state.facing_angle) * m_grounded_state.speed,
                       0.0f,
                       cosf(m_grounded_state.facing_angle) * m_grounded_state.speed };

        // Keep airborne state up to date.
        m_airborne_state.input_facing_angle = m_grounded_state.facing_angle;

        display_facing_angle = m_grounded_state.facing_angle;
    }
    else
    {
        // Move towards desired velocity.
        JPH::Vec3 flat_linear_velo{ linear_velocity.GetX(),
                                    0.0f,
                                    linear_velocity.GetZ() };
        JPH::Vec3 delta_velocity{ desired_velocity - flat_linear_velo };

        float_t airborne_accel{ m_settings.airborne_acceleration * physics_delta_time };
        if (delta_velocity.LengthSq() > airborne_accel * airborne_accel)
        {
            delta_velocity = delta_velocity.Normalized() * airborne_accel;
        }

        JPH::Vec3 effective_velocity{ flat_linear_velo + delta_velocity };
        new_velocity += effective_velocity;

        if (glm_vec3_norm2(move_input_world) > 1e-6f * 1e-6f)
        {
            // Move towards input angle.
            float_t desired_facing_angle{ atan2f(move_input_world[0], move_input_world[2]) };
            float_t delta_direction{ desired_facing_angle - m_airborne_state.input_facing_angle };
            while (delta_direction > glm_rad(180.0f)) delta_direction -= glm_rad(360.0f);
            while (delta_direction <= glm_rad(-180.0f)) delta_direction += glm_rad(360.0f);

            float_t max_turn_delta{ m_settings.airborne_turn_speed * physics_delta_time };
            if (abs(delta_direction) > max_turn_delta)
            {
                // Limit turn speed.
                delta_direction = max_turn_delta * glm_signf(delta_direction);
            }

            m_airborne_state.input_facing_angle += delta_direction;
        }

        // Keep grounded state up to date.
        m_grounded_state.speed = effective_velocity.Length();
        if (effective_velocity.IsNearZero())
            m_grounded_state.facing_angle = m_airborne_state.input_facing_angle;
        else
            m_grounded_state.facing_angle = atan2f(effective_velocity.GetX(), effective_velocity.GetZ());
        m_grounded_state.turnaround_enabled = false;

        display_facing_angle = m_airborne_state.input_facing_angle;
    }

    // Apply to character.
    new_velocity += (up_rotation * phys_system->GetGravity()) * physics_delta_time;
    character_impl->set_cc_allow_sliding(is_grounded && (m_grounded_state.speed > 1e-6f));
    character_impl->set_cc_velocity(new_velocity);

    // Finish.
    m_phys_engine.return_physics_object(phys_obj);

    {   // Write new facing direction.
        auto& rs{ m_rendering_state };
        lock_guard<mutex> lock{ rs.access_mutex };
        rs.facing_angle_render_triple_buffer[rs.write_pos] = display_facing_angle;

        auto temp{ rs.read_a_pos };
        rs.read_a_pos = rs.read_b_pos;
        rs.read_b_pos = rs.write_pos;
        rs.write_pos = temp;
    }
}

void BT::Scripts::Script_player_character_movement::on_pre_render(float_t delta_time)
{
    // Read facing angles.
    float_t facing_angles[2];
    {
        lock_guard<mutex> lock{ m_rendering_state.access_mutex };
        facing_angles[0] = m_rendering_state.facing_angle_render_triple_buffer[m_rendering_state.read_a_pos];
        facing_angles[1] = m_rendering_state.facing_angle_render_triple_buffer[m_rendering_state.read_b_pos];
    }

    // Lerp angle.
    float_t delta_facing_angle{ facing_angles[1] - facing_angles[0] };
    while (delta_facing_angle > glm_rad(180.0f)) delta_facing_angle -= glm_rad(360.0f);
    while (delta_facing_angle <= glm_rad(-180.0f)) delta_facing_angle += glm_rad(360.0f);

    float_t lerped_facing_angle{
        facing_angles[0] +
            delta_facing_angle * m_phys_engine.get_interpolation_alpha() };

    // Write angle to game object.
    versor new_rot;
    glm_quat(new_rot, lerped_facing_angle, 0.0f, 1.0f, 0.0f);

    auto game_obj{ m_game_obj_pool.get_one_no_lock(m_apply_facing_angle_game_obj_key) };
    game_obj->get_transform_handle().set_local_rot(new_rot);
    game_obj->propagate_transform_changes();
}


// Helper funcs.
float_t BT::Scripts::Script_player_character_movement::find_grounded_turn_speed(float_t linear_speed)
{
    float_t turn_speed{ 0.0f };
    for (auto& context : m_settings.grounded_turn_speeds)
        if (linear_speed <= context.max_speed_of_context)
        {
            turn_speed = context.turn_speed;
            break;
        }

    return turn_speed;
}

void BT::Scripts::Script_player_character_movement::apply_grounded_facing_angle(
    JPH::Vec3Arg input_velocity,
    float_t physics_delta_time)
{
    float_t desired_facing_angle{ atan2f(input_velocity.GetX(), input_velocity.GetZ()) };

    float_t delta_direction{ desired_facing_angle - m_grounded_state.facing_angle };
    while (delta_direction > glm_rad(180.0f)) delta_direction -= glm_rad(360.0f);
    while (delta_direction <= glm_rad(-180.0f)) delta_direction += glm_rad(360.0f);

    float_t turn_speed{ find_grounded_turn_speed(m_grounded_state.speed) };
    bool is_quick_turn_speed{ turn_speed > 1000.0f };  // Idk just some number.

    if (is_quick_turn_speed)
    {
        // Disable turnaround mode.
        m_grounded_state.turnaround_enabled = false;
    }

    constexpr float_t k_turn_around_back_angle{ 45.0f };
    bool turnaround_mode{ m_grounded_state.turnaround_enabled ||
                          (!is_quick_turn_speed &&
                           abs(delta_direction) >
                               glm_rad(180.0f - (k_turn_around_back_angle * 0.5f))) };
    if (turnaround_mode)
    {
        // Lock in turnaround.
        m_grounded_state.turnaround_enabled = true;
    }
    else
    {
        // Process grounded turning.
        float_t max_turn_delta{ turn_speed * physics_delta_time };
        if (abs(delta_direction) > max_turn_delta)
        {
            // Limit turn speed.
            delta_direction = max_turn_delta * glm_signf(delta_direction);
        }

        // Apply new facing angle.
        m_grounded_state.facing_angle += delta_direction;
    }
}

void BT::Scripts::Script_player_character_movement::apply_grounded_linear_speed(
    JPH::Vec3Arg input_velocity,
    float_t physics_delta_time)
{
    float_t desired_speed{ glm_vec2_norm(vec2{ input_velocity.GetX(), input_velocity.GetZ() }) };

    if (m_grounded_state.turnaround_enabled)
    {   // Zero desired speed if doing turnaround.
        desired_speed = 0.0f;
    }

    float_t delta_speed{ desired_speed - m_grounded_state.speed };
    float_t acceleration{ delta_speed < 0.0f ?
                          -m_settings.grounded_deceleration * physics_delta_time :
                          m_settings.grounded_acceleration * physics_delta_time };
    if (abs(delta_speed) > abs(acceleration))
    {
        // Limit delta speed to acceleration.
        delta_speed = acceleration;
    }

    // Apply new linear speed.
    m_grounded_state.speed += delta_speed;
}

void BT::Scripts::Script_player_character_movement::process_midair_jump_interactions(
    Physics_object_type_impl_ifc* character_impl,
    JPH::Vec3 up_direction,
    JPH::Vec3& new_velocity)
{
    // Wall interactions.
    // @NOTE: Check for a ledge climb first, and then check for a wall
    //   jump if the ledge climb fails.

    #define REFACTOR_WALL_INTERACTIONS 1
    #if REFACTOR_WALL_INTERACTIONS
    bool ledge_climb_failed{ false };

    float_t char_con_radius{ character_impl->get_cc_radius() };

    // Calc check origin point (since char collider is a cube).
    auto opaid{ calc_check_origin_point_and_input_dir(m_airborne_state.input_facing_angle,
                                                      char_con_radius) };
    JPH::RVec3 check_origin_point{ character_impl->read_transform().position + opaid.origin_offset };
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

    float_t char_con_height{ character_impl->get_cc_height() };
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
        character_impl->move_kinematic({ led_sea_test_passed_target_pos,
                                         character_impl->read_transform().rotation });
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
                calc_check_origin_point_and_input_dir(m_airborne_state.input_facing_angle
                                                          + (i * glm_rad(360.0f) / k_num_circle_raycasts),
                                                      char_con_radius) };

            auto data{ Raycast_helper::raycast(character_impl->read_transform().position + opaid.origin_offset,
                                               1.5f * opaid.input_dir) };
            if (data.success)
            {
                // Commit to wall jump.
                // @TEMPORARY: I think holding the player to the wall for just a little bit would be good to help the animation.
                JPH::Vec3 curr_up_velo{ up_direction * up_direction.Dot(new_velocity) };
                new_velocity += -curr_up_velo + m_settings.jump_speed * up_direction;
                break;
            }
        }
    }

    #endif  // REFACTOR_WALL_INTERACTIONS
}

BT::Scripts::Script_player_character_movement::Origin_offset_and_input_dir
BT::Scripts::Script_player_character_movement
    ::calc_check_origin_point_and_input_dir(float_t facing_angle,
                                            float_t char_con_radius)
{
    JPH::Vec3 flat_input_dir{ sinf(facing_angle),
                              0.0f,
                              cosf(facing_angle) };
    float_t cancel_ratio{ char_con_radius
                          / std::max(abs(flat_input_dir.GetX()),
                                     abs(flat_input_dir.GetZ())) };
    JPH::Vec3 resized_to_radius_box_velo{ flat_input_dir * cancel_ratio };

    return { resized_to_radius_box_velo, flat_input_dir };
}
#endif  // 0
