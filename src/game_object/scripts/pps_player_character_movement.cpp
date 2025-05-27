#include "pre_physics_scripts.h"

#include "../physics_engine/physics_engine.h"


void BT::Pre_physics_script::script_player_character_movement(
    Physics_engine* phys_engine,
    vector<uint64_t> const& datas,
    size_t& in_out_read_data_idx)
{
    auto phys_obj_key{ datas[in_out_read_data_idx++] };








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
    if (mCharacter->GetGroundState() == CharacterVirtual::EGroundState::OnGround	// If on ground
        && (sEnableCharacterInertia?
            moving_towards_ground													// Inertia enabled: And not moving away from ground
            : !mCharacter->IsSlopeTooSteep(mCharacter->GetGroundNormal())))			// Inertia disabled: And not on a slope that is too steep
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







    Physics_object* phys_obj{ phys_engine->checkout_physics_object(phys_obj_key) };
    phys_obj->set_linear_velocity(JPH::Vec3Arg velocity)
}
