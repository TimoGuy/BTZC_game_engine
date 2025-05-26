#include "physics_object_impl_char_controller.h"

#include "Jolt/Jolt.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/Collision/Shape/CylinderShape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "physics_engine_impl_layers.h"


BT::Phys_obj_impl_char_controller::Phys_obj_impl_char_controller(Physics_engine& phys_engine,
                                                                 float_t radius,
                                                                 float_t height,
                                                                 float_t crouch_height,
                                                                 Physics_transform&& init_transform)
    : m_phys_engine{ phys_engine }
    , m_radius{ radius }
    , m_height{ height }
    , m_crouch_height{ crouch_height }
{
    m_standing_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * m_height + m_radius, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * m_height + m_radius, m_radius)).Create().Get();
    m_crouching_shape = JPH::RotatedTranslatedShapeSettings(JPH::Vec3(0, 0.5f * m_crouch_height + m_radius, 0), JPH::Quat::sIdentity(), new JPH::CylinderShape(0.5f * m_crouch_height + m_radius, m_radius)).Create().Get();

    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mMaxSlopeAngle = s_max_slope_angle;
    settings->mMaxStrength = s_max_strength;
    settings->mShape = m_standing_shape;
    settings->mBackFaceMode = s_back_face_mode;
    settings->mCharacterPadding = s_character_padding;
    settings->mPenetrationRecoverySpeed = s_penetration_recovery_speed;
    settings->mPredictiveContactDistance = s_predictive_contact_distance;

    // Accept contacts that touch the lower sphere of the capsule.
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -m_radius);
    settings->mEnhancedInternalEdgeRemoval = s_enhanced_internal_edge_removal;
    // @HERE: Add inner shape if wanted.  vv
    settings->mInnerBodyShape = /*s_create_inner_body ? mInnerStandingShape :*/ nullptr; assert(!s_create_inner_body);
    settings->mInnerBodyLayer = Layers::MOVING;
    m_character = new JPH::CharacterVirtual(settings,
                                            init_transform.position,
                                            init_transform.rotation,
                                            0,
                                            reinterpret_cast<JPH::PhysicsSystem*>(
                                                m_phys_engine.get_physics_system_ptr()));

    // @TODO: Add handling for char vs char collision!
    // m_character->SetCharacterVsCharacterCollision(&mCharacterVsCharacterCollision);
    // mCharacterVsCharacterCollision.Add(m_character);

    // Install contact listener.
    m_character->SetListener(this);

    // @TEMP: @NOCHECKIN.
    m_character->SetLinearVelocity(JPH::Vec3(1.0f, 0.0f, 0.0f));
}

BT::Phys_obj_impl_char_controller::~Phys_obj_impl_char_controller()
{
}

// Phys obj impl ifc.
void BT::Phys_obj_impl_char_controller::move_kinematic(Physics_transform&& new_transform)
{
    m_character->SetPosition(new_transform.position);
    // @CHECK: Make sure that this works! UNTESTED!!!!!
    assert(false);
}

void BT::Phys_obj_impl_char_controller::set_linear_velocity(JPH::Vec3Arg velocity)
{
    m_character->SetLinearVelocity(velocity);
}

void BT::Phys_obj_impl_char_controller::on_pre_update(float_t physics_delta_time)
{
    // Settings for our update function.
    JPH::CharacterVirtual::ExtendedUpdateSettings update_settings;
    if (!s_enable_stick_to_floor)
    {
        update_settings.mStickToFloorStepDown = JPH::Vec3::sZero();
    }
    else
    {
        update_settings.mStickToFloorStepDown = -m_character->GetUp() * update_settings.mStickToFloorStepDown.Length();
    }

    if (!s_enable_walk_stairs)
    {
        update_settings.mWalkStairsStepUp = JPH::Vec3::sZero();
    }
    else
    {
        update_settings.mWalkStairsStepUp = m_character->GetUp() * update_settings.mWalkStairsStepUp.Length();
        update_settings.mWalkStairsMinStepForward = 0.1f;
    }

    // Update the character position.
    auto physics_system{ reinterpret_cast<JPH::PhysicsSystem*>(m_phys_engine.get_physics_system_ptr()) };
    auto temp_allocator{ reinterpret_cast<JPH::TempAllocator*>(m_phys_engine.get_physics_temp_allocator_ptr()) };
    m_character->ExtendedUpdate(physics_delta_time,
                                -m_character->GetUp() * physics_system->GetGravity().Length(),
                                update_settings,
                                physics_system->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                                physics_system->GetDefaultLayerFilter(Layers::MOVING),
                                { },
                                { },
                                *temp_allocator);
}

BT::Physics_transform BT::Phys_obj_impl_char_controller::read_transform()
{
    return { m_character->GetPosition(), m_character->GetRotation() };
}

// Character contact listener.
void BT::Phys_obj_impl_char_controller::OnAdjustBodyVelocity(JPH::CharacterVirtual const* inCharacter,
                                                             JPH::Body const& in_body2,
                                                             JPH::Vec3& io_linear_velocity,
                                                             JPH::Vec3& io_angular_velocity)
{
    // // Apply artificial velocity to the character when standing on the conveyor belt
    // if (inBody2.GetID() == mConveyorBeltBody)
    //     ioLinearVelocity += Vec3(0, 0, 2);
}

void BT::Phys_obj_impl_char_controller::OnContactAdded(JPH::CharacterVirtual const* in_character,
                                                       JPH::BodyID const& in_body_id2,
                                                       JPH::SubShapeID const& in_sub_shape_id2,
                                                       JPH::RVec3Arg in_contact_position,
                                                       JPH::Vec3Arg in_contact_normal,
                                                       JPH::CharacterContactSettings& io_settings)
{
    // // Draw a box around the character when it enters the sensor
    // if (inBodyID2 == mSensorBody)
    // {
    //     AABox box = inCharacter->GetShape()->GetWorldSpaceBounds(inCharacter->GetCenterOfMassTransform(), Vec3::sReplicate(1.0f));
    //     mDebugRenderer->DrawBox(box, Color::sGreen, DebugRenderer::ECastShadow::Off, DebugRenderer::EDrawMode::Wireframe);
    // }

    // // Dynamic boxes on the ramp go through all permutations
    // Array<BodyID>::const_iterator i = find(mRampBlocks.begin(), mRampBlocks.end(), inBodyID2);
    // if (i != mRampBlocks.end())
    // {
    //     size_t index = i - mRampBlocks.begin();
    //     ioSettings.mCanPushCharacter = (index & 1) != 0;
    //     ioSettings.mCanReceiveImpulses = (index & 2) != 0;
    // }

    // // If we encounter an object that can push the player, enable sliding
    // if (inCharacter == mCharacter
    //     && ioSettings.mCanPushCharacter
    //     && physics_system->GetBodyInterface().GetMotionType(inBodyID2) != EMotionType::Static)
    //     mAllowSliding = true;
}

void BT::Phys_obj_impl_char_controller::OnCharacterContactAdded(JPH::CharacterVirtual const* in_character,
                                                                JPH::CharacterVirtual const* in_other_character,
                                                                JPH::SubShapeID const& in_sub_shape_id2,
                                                                JPH::RVec3Arg in_contact_position,
                                                                JPH::Vec3Arg in_contact_normal,
                                                                JPH::CharacterContactSettings& io_settings)
{
    // // Characters can only be pushed in their own update
    // if (sPlayerCanPushOtherCharacters)
    //     ioSettings.mCanPushCharacter = sOtherCharactersCanPushPlayer || inOtherCharacter == mCharacter;
    // else if (sOtherCharactersCanPushPlayer)
    //     ioSettings.mCanPushCharacter = inCharacter == mCharacter;
    // else
    //     ioSettings.mCanPushCharacter = false;

    // // If the player can be pushed by the other virtual character, we allow sliding
    // if (inCharacter == mCharacter && ioSettings.mCanPushCharacter)
    //     mAllowSliding = true;
}

void BT::Phys_obj_impl_char_controller::OnContactSolve(JPH::CharacterVirtual const* in_character,
                                                       JPH::BodyID const& in_body_id2,
                                                       JPH::SubShapeID const& in_sub_shape_id2,
                                                       JPH::RVec3Arg in_contact_position,
                                                       JPH::Vec3Arg in_contact_normal,
                                                       JPH::Vec3Arg in_contact_velocity,
                                                       JPH::PhysicsMaterial const* in_contact_material,
                                                       JPH::Vec3Arg in_character_velocity,
                                                       JPH::Vec3& io_new_character_velocity)
{
    // Ignore callbacks for other characters than me.
    if (in_character != m_character)
        return;

    // Don't slide down static, not-too-steep surfaces when not actively moving and when not on a moving platform.
    if (!m_allow_sliding &&
        in_contact_velocity.IsNearZero()
        && !in_character->IsSlopeTooSteep(in_contact_normal))
        io_new_character_velocity = JPH::Vec3::sZero();
}
