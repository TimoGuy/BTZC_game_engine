#include "scripts.h"

#include "../input_handler/input_handler.h"
#include "../physics_engine/physics_engine.h"
#include "../renderer/camera.h"
#include "../renderer/renderer.h"
#include "cglm/vec3.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include <memory>

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
    if (camera->is_capture_fly())
        glm_vec3_zero(move_input_world);
    else if (glm_vec3_norm2(move_input_world) > 1.0f)
        glm_vec3_normalize(move_input_world);

    character_impl->set_cc_allow_sliding(glm_vec3_norm2(move_input_world) > 1e-6f * 1e-6f);

    // Change input into desired velocity.
    constexpr float_t k_standing_speed{ 15.0f };
    constexpr float_t k_crouched_speed{ 5.0f };
    JPH::Vec3 desired_velocity{ move_input_world[0],
                                move_input_world[1],
                                move_input_world[2] };
    desired_velocity *= (character_impl->get_cc_stance() ?
                             k_crouched_speed :
                             k_standing_speed);

    // Extra input checks.
    bool on_jump_press{ input_state.jump.val && !m_prev_jump_pressed };
    m_prev_jump_pressed = input_state.jump.val;

    bool on_crouch_press{ input_state.crouch.val && !m_prev_crouch_pressed };
    m_prev_crouch_pressed = input_state.crouch.val;

    // Determine new basic velocity.
    JPH::Vec3 current_vertical_velocity = linear_velocity.Dot(up_direction) * up_direction;
    JPH::Vec3 new_velocity;
    bool is_grounded{ ground_state == JPH::CharacterVirtual::EGroundState::OnGround &&
                      (current_vertical_velocity.GetY() - ground_velocity.GetY()) < 0.1f &&
                      !character_impl->is_cc_slope_too_steep(ground_normal) };
    if (is_grounded)
    {
        // Assume velocity of ground.
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
            // Leave crouching stance.
            character_impl->set_cc_stance(false);
        }

        // Keep previous tick velocity.
        new_velocity = current_vertical_velocity;
    }

    // Gravity.
    new_velocity += (up_rotation * phys_system->GetGravity()) * physics_delta_time;

    // Input velocity.
    new_velocity += desired_velocity;

    // Set input velocity.
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
