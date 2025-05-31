#include "pre_physics_scripts.h"

#include "../input_handler/input_handler.h"
#include "../physics_engine/physics_engine.h"
#include "../renderer/camera.h"
#include "cglm/vec3.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "logger.h"
#include "serialization.h"


void BT::Pre_physics_script::script_player_character_movement_serialize(
    Input_handler* input_handler,
    Physics_engine* phys_engine,
    Renderer* renderer,
    Scene_serialization_mode mode,
    json& node_ref,
    vector<uint64_t> const& datas,
    size_t& in_out_read_data_idx)
{
    if (mode == Scene_serialization_mode::SCENE_SERIAL_MODE_SERIALIZE)
    {
        node_ref[] = Serial::pop_u64(datas, in_out_read_data_idx);
        // Input state.
        // Camera.
        // Persistent state 1.
        // Persistent state 2.
    }
    else if (mode == Scene_serialization_mode::SCENE_SERIAL_MODE_DESERIALIZE)
    {
        // @TODO
        assert(false);
    }
}

void BT::Pre_physics_script::script_player_character_movement(
    Physics_engine* phys_engine,
    vector<uint64_t> const& datas,
    size_t& in_out_read_data_idx)
{
    auto phys_obj_key{ Serial::pop_u64(datas, in_out_read_data_idx) };
    auto const& input_state{
        reinterpret_cast<Input_handler*>(Serial::pop_void_ptr(datas, in_out_read_data_idx))
            ->get_input_state() };
    auto camera{ reinterpret_cast<Camera*>(Serial::pop_void_ptr(datas, in_out_read_data_idx)) };
    auto& pers_state1{ Serial::pop_u64_persistent_state(datas, in_out_read_data_idx) };
    auto& pers_state2{ Serial::pop_u64_persistent_state(datas, in_out_read_data_idx) };

    Physics_object* phys_obj{ phys_engine->checkout_physics_object(phys_obj_key) };
    auto character_impl{ phys_obj->get_impl() };

    auto phys_system{ reinterpret_cast<JPH::PhysicsSystem*>(phys_engine->get_physics_system_ptr()) };

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
    bool prev_jump_pressed{ static_cast<bool>(pers_state1) };
    bool on_jump_press{ input_state.jump.val && !prev_jump_pressed };
    pers_state1 = input_state.jump.val;

    bool prev_crouch_pressed{ static_cast<bool>(pers_state2) };
    bool on_crouch_press{ input_state.crouch.val && !prev_crouch_pressed };
    pers_state2 = input_state.crouch.val;

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
    new_velocity += (up_rotation * phys_system->GetGravity()) * Physics_engine::k_simulation_delta_time;

    // Input velocity.
    new_velocity += desired_velocity;

    // Set input velocity.
    character_impl->set_cc_velocity(new_velocity);

    // Finish.
    phys_engine->return_physics_object(phys_obj);








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
