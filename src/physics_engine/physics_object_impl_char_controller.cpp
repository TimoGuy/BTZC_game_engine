#include "physics_object_impl_char_controller.h"

#include "Jolt/Jolt.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/Collision/Shape/BoxShape.h"
#include "Jolt/Physics/Collision/Shape/RotatedTranslatedShape.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "physics_engine_impl_layers.h"


BT::Phys_obj_impl_char_controller::Phys_obj_impl_char_controller(Physics_engine& phys_engine,
                                                                 float_t radius,
                                                                 float_t height,
                                                                 float_t crouch_height,
                                                                 Physics_transform&& init_transform)
    : m_phys_engine{ phys_engine }
    , m_phys_system{ *reinterpret_cast<JPH::PhysicsSystem*>(m_phys_engine.get_physics_system_ptr()) }
    , m_phys_temp_allocator{ *reinterpret_cast<JPH::TempAllocator*>(m_phys_engine.get_physics_temp_allocator_ptr()) }
    , m_radius{ radius }
    , m_height{ height - 2.0f * radius }
    , m_crouch_height{ crouch_height }
    , m_is_crouched{ false }
{
    assert(m_height >= 0.0f);

    // @NOTE: Before the cylinder collider was used to get round sides and a flat
    //   bottom, however, the side collisions of the cylinder became so erratic that
    //   I had to switch to a box collider. It's a bit sad but the collision looks
    //   and feels great now!  -Thea 2025/05/29
    m_standing_shape = JPH::RotatedTranslatedShapeSettings(
        JPH::Vec3(0, 0.5f * m_height + m_radius, 0),
        JPH::Quat::sIdentity(),
        new JPH::BoxShape(JPH::Vec3(m_radius, 0.5f * m_height + m_radius, m_radius))).Create().Get();
    m_crouching_shape = JPH::RotatedTranslatedShapeSettings(
        JPH::Vec3(0, 0.5f * m_crouch_height + m_radius, 0),
        JPH::Quat::sIdentity(),
        new JPH::BoxShape(JPH::Vec3(m_radius, 0.5f * m_crouch_height + m_radius, m_radius))).Create().Get();

    JPH::Ref<JPH::CharacterVirtualSettings> settings = new JPH::CharacterVirtualSettings();
    settings->mMaxSlopeAngle = s_max_slope_angle;
    settings->mMaxStrength = s_max_strength;
    settings->mShape = (m_is_crouched ? m_crouching_shape : m_standing_shape);
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
                                            &m_phys_system);

    // @TODO: Add handling for char vs char collision!
    // m_character->SetCharacterVsCharacterCollision(&m_characterVsCharacterCollision);
    // m_characterVsCharacterCollision.Add(m_character);

    // Install contact listener.
    m_character->SetListener(this);
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

void BT::Phys_obj_impl_char_controller::tick_fetch_cc_status(JPH::Vec3& out_ground_velocity,
                                                             JPH::Vec3& out_linear_velocity,
                                                             JPH::Vec3& out_up_direction,
                                                             JPH::Quat& out_up_rotation,
                                                             bool& out_is_supported,
                                                             JPH::CharacterVirtual::EGroundState& out_ground_state,
                                                             JPH::Vec3& out_ground_normal,
                                                             bool& out_is_crouched)
{
    // Tick the ground velocity.
    m_character->UpdateGroundVelocity();

    // Fetch all wanted values.
    out_ground_velocity = m_character->GetGroundVelocity();
    out_linear_velocity = m_character->GetLinearVelocity();
    out_up_direction = m_character->GetUp();
    out_is_supported = m_character->IsSupported();
    out_ground_state = m_character->GetGroundState();
    out_ground_normal = m_character->GetGroundNormal();
    out_is_crouched = m_is_crouched;
}

bool BT::Phys_obj_impl_char_controller::is_cc_slope_too_steep(JPH::Vec3 normal)
{
    return m_character->IsSlopeTooSteep(normal);
}

void BT::Phys_obj_impl_char_controller::set_cc_allow_sliding(bool allow)
{
    m_allow_sliding = allow;
}

void BT::Phys_obj_impl_char_controller::set_cc_velocity(JPH::Vec3Arg velocity)
{
    m_character->SetLinearVelocity(velocity);
}

bool BT::Phys_obj_impl_char_controller::set_cc_stance(bool is_crouching)
{
    JPH::Shape const* shape{ is_crouching ? m_crouching_shape : m_standing_shape };
    bool success{ m_character->SetShape(shape,
                                        1.5f * m_phys_system.GetPhysicsSettings().mPenetrationSlop,
                                        m_phys_system.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                                        m_phys_system.GetDefaultLayerFilter(Layers::MOVING),
                                        { },
                                        { },
                                        m_phys_temp_allocator) };
    if (success)
    {
        // Update stance if switch succeeded.
        m_is_crouched = is_crouching;
    }

    return success;
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
    m_character->ExtendedUpdate(physics_delta_time,
                                -m_character->GetUp() * m_phys_system.GetGravity().Length(),
                                update_settings,
                                m_phys_system.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
                                m_phys_system.GetDefaultLayerFilter(Layers::MOVING),
                                { },
                                { },
                                m_phys_temp_allocator);
    // m_character->Update(physics_delta_time,
    //                     -m_character->GetUp() * m_phys_system.GetGravity().Length(),
    //                     m_phys_system.GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
    //                     m_phys_system.GetDefaultLayerFilter(Layers::MOVING),
    //                     { },
    //                     { },
    //                     m_phys_temp_allocator);
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
    // if (inCharacter == m_character
    //     && ioSettings.mCanPushCharacter
    //     && m_phys_system.GetBodyInterface().GetMotionType(inBodyID2) != EMotionType::Static)
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
    //     ioSettings.mCanPushCharacter = sOtherCharactersCanPushPlayer || inOtherCharacter == m_character;
    // else if (sOtherCharactersCanPushPlayer)
    //     ioSettings.mCanPushCharacter = inCharacter == m_character;
    // else
    //     ioSettings.mCanPushCharacter = false;

    // // If the player can be pushed by the other virtual character, we allow sliding
    // if (inCharacter == m_character && ioSettings.mCanPushCharacter)
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
