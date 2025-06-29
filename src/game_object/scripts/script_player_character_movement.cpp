#include "cglm/util.h"
#include "scripts.h"

#include "../input_handler/input_handler.h"
#include "../physics_engine/physics_engine.h"
#include "../renderer/camera.h"
#include "../renderer/renderer.h"
#include "cglm/vec3.h"
#include "Jolt/Jolt.h"
#include "Jolt/Math/Vec3.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include <array>
#include <memory>

using std::array;
using std::unique_ptr;


namespace BT::Scripts
{

class Script_player_character_movement : public Script_ifc
{
public:
    Script_player_character_movement(Input_handler const& input_handler,
                                     Physics_engine& phys_engine,
                                     Renderer& renderer,
                                     UUID phys_obj_key)
        : m_input_handler{ input_handler }
        , m_phys_engine{ phys_engine }
        , m_renderer{ renderer }
        , m_phys_obj_key{ phys_obj_key }
    {
    }

    Input_handler const& m_input_handler;
    Physics_engine& m_phys_engine;
    Renderer& m_renderer;
    UUID m_phys_obj_key;
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
    } m_airborne_state;

    // Script_ifc.
    Script_type get_type() override
    {
        return SCRIPT_TYPE_player_character_movement;
    }

    void serialize_datas(json& node_ref) override
    {
        node_ref["phys_obj_key"] = UUID_helper::to_pretty_repr(m_phys_obj_key);
    }

    void on_pre_physics(float_t physics_delta_time) override;

    // Helper funcs.
    float_t find_grounded_turn_speed(float_t linear_speed);
    void apply_grounded_facing_angle(JPH::Vec3Arg input_velocity, float_t physics_delta_time);
    void apply_grounded_linear_speed(JPH::Vec3Arg input_velocity, float_t physics_delta_time);

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
                                              UUID_helper::to_UUID(node_ref["phys_obj_key"]) });
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
            new_velocity += 35.0f * up_direction;
        }
    }
    else
    {
        if (character_impl->get_cc_stance())
        {
            // Leave crouching stance automatically.
            character_impl->set_cc_stance(false);
        }

        if (current_vertical_velocity.GetY() < 0.0f &&
            character_impl->has_cc_wall_contact() &&
            on_jump_press)
        {
            // Wall jump.
            new_velocity += 35.0f * up_direction;
        }

        // Keep previous tick velocity.
        new_velocity = current_vertical_velocity;
    }

    // Desired velocity.
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

        // Keep grounded state up to date.
        m_grounded_state.speed = effective_velocity.Length();
        if (!effective_velocity.IsNearZero())
            m_grounded_state.facing_angle = atan2f(effective_velocity.GetX(), effective_velocity.GetZ());
        m_grounded_state.turnaround_enabled = false;
    }

    // Apply to character.
    new_velocity += (up_rotation * phys_system->GetGravity()) * physics_delta_time;
    character_impl->set_cc_allow_sliding(is_grounded && (m_grounded_state.speed > 1e-6f));
    character_impl->set_cc_velocity(new_velocity);

    // Finish.
    m_phys_engine.return_physics_object(phys_obj);



















#if 0
    bool player_controls_horizontal_velocity = sControlMovementDuringJump || mCharacter->IsSupported();
    if (player_controls_horizontal_velocity)
    {
        // Smooth the player input
        mDesiredVelocity = sEnableCharacterInertia? 0.25f * inMovementDirection * sCharacterSpeed + 0.75f * mDesiredVelocity : inMovementDirection * sCharacterSpeed;

        // True if the player intended to move
        mAllowSliding = !inMovementDirection.IsNearZero();
    }
    else
    {
        // While in air we allow sliding
        mAllowSliding = true;
    }

    // Update the character rotation and its up vector to match the up vector set by the user settings
    Quat character_up_rotation = Quat::sEulerAngles(Vec3(sUpRotationX, 0, sUpRotationZ));
    mCharacter->SetUp(character_up_rotation.RotateAxisY());
    mCharacter->SetRotation(character_up_rotation);

    // A cheaper way to update the character's ground velocity,
    // the platforms that the character is standing on may have changed velocity
    mCharacter->UpdateGroundVelocity();
    JPH::Vec3 ground_velocity = mCharacter->GetGroundVelocity();

    // Determine new basic velocity
    Vec3 current_vertical_velocity = mCharacter->GetLinearVelocity().Dot(mCharacter->GetUp()) * mCharacter->GetUp();
    Vec3 new_velocity;
    bool moving_towards_ground = (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f;
    if (mCharacter->GetGroundState() == CharacterVirtual::EGroundState::OnGround    // If on ground
        && (sEnableCharacterInertia?
            moving_towards_ground                                                    // Inertia enabled: And not moving away from ground
            : !mCharacter->IsSlopeTooSteep(mCharacter->GetGroundNormal())))            // Inertia disabled: And not on a slope that is too steep
    {
        // Assume velocity of ground when on ground
        new_velocity = ground_velocity;

        // Jump
        if (inJump && moving_towards_ground)
            new_velocity += sJumpSpeed * mCharacter->GetUp();
    }
    else
        new_velocity = current_vertical_velocity;

    // Gravity
    new_velocity += (character_up_rotation * mPhysicsSystem->GetGravity()) * inDeltaTime;

    if (player_controls_horizontal_velocity)
    {
        // Player input
        new_velocity += character_up_rotation * mDesiredVelocity;
    }
    else
    {
        // Preserve horizontal velocity
        Vec3 current_horizontal_velocity = mCharacter->GetLinearVelocity() - current_vertical_velocity;
        new_velocity += current_horizontal_velocity;
    }

    // Update character velocity
    mCharacter->SetLinearVelocity(new_velocity);

    // Stance switch
    if (inSwitchStance)
    {
        bool is_standing = mCharacter->GetShape() == mStandingShape;
        const Shape *shape = is_standing? mCrouchingShape : mStandingShape;
        if (mCharacter->SetShape(shape, 1.5f * mPhysicsSystem->GetPhysicsSettings().mPenetrationSlop, mPhysicsSystem->GetDefaultBroadPhaseLayerFilter(Layers::MOVING), mPhysicsSystem->GetDefaultLayerFilter(Layers::MOVING), { }, { }, *mTempAllocator))
        {
            const Shape *inner_shape = is_standing? mInnerCrouchingShape : mInnerStandingShape;
            mCharacter->SetInnerBodyShape(inner_shape);
        }
    }
#endif // 0
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
        float_t turn_delta{ turn_speed * physics_delta_time };
        if (abs(delta_direction) > turn_delta)
        {
            // Limit turn speed.
            delta_direction = turn_delta * glm_signf(delta_direction);
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
