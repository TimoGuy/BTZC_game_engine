#include "raycast_helper.h"

#include "../renderer/debug_render_job.h"
#include "Jolt/Physics/Collision/BackFaceMode.h"
#include "physics_engine_impl_layers.h"
#include "Jolt/Math/Real.h"
#include "Jolt/Physics/Collision/CastResult.h"
#include "Jolt/Physics/Collision/RayCast.h"
#include "Jolt/Physics/PhysicsSystem.h"


namespace
{

static BT::Physics_engine* s_physics_engine{ nullptr };

}  // namespace


void BT::Raycast_helper::set_physics_engine(Physics_engine& physics_engine)
{
    s_physics_engine = &physics_engine;
}

namespace
{

class BTCollector : public JPH::CastRayCollector
{
public:
    BTCollector(JPH::PhysicsSystem& in_physics_system,
                JPH::RRayCast& in_ray)
        : m_physics_system{ in_physics_system }
        , m_ray{ in_ray }
    {
    }

    virtual void AddHit(JPH::RayCastResult const& in_result) override
    {
        // Test if this collision is closer than the previous one.
        if (in_result.mFraction < GetEarlyOutFraction())
        {
            // Lock the body.
            JPH::BodyLockRead lock(m_physics_system.GetBodyLockInterfaceNoLock(), in_result.mBodyID);
            JPH_ASSERT(lock.Succeeded()); // When this runs all bodies are locked so this should not fail
            JPH::Body const* body = &lock.GetBody();

            if (body->IsSensor())
                return;

            JPH::RVec3 contact_pos = m_ray.GetPointOnRay(in_result.mFraction);
            JPH::Vec3 normal = body->GetWorldSpaceSurfaceNormal(in_result.mSubShapeID2, contact_pos);

            // Update early out fraction to this hit.
            UpdateEarlyOutFraction(in_result.mFraction);

            // Get the contact properties.
            m_body = body;
            m_sub_shape_id2 = in_result.mSubShapeID2;
            m_contact_position = contact_pos;
            m_contact_normal = normal;
        }
    }

    // Configuration.
    JPH::PhysicsSystem& m_physics_system;
    JPH::RRayCast       m_ray;

    // Resulting closest collision.
    JPH::Body const* m_body{ nullptr };
    JPH::SubShapeID  m_sub_shape_id2;
    JPH::RVec3       m_contact_position;
    JPH::Vec3        m_contact_normal;
};

}  // namespace

BT::Raycast_helper::Raycast_result
BT::Raycast_helper::raycast(JPH::RVec3Arg origin, JPH::Vec3Arg direction_and_magnitude)
{
    Raycast_result return_result;

    auto& physics_system{
        *reinterpret_cast<JPH::PhysicsSystem*>(s_physics_engine->get_physics_system_ptr()) };

    JPH::RRayCast ray{ origin, direction_and_magnitude };

    JPH::RayCastSettings cast_settings;
    cast_settings.SetBackFaceMode(JPH::EBackFaceMode::IgnoreBackFaces);

    BTCollector ray_collector{ physics_system, ray };

    physics_system.GetNarrowPhaseQuery().CastRay(
        ray,
        cast_settings,
        ray_collector,
        JPH::SpecifiedBroadPhaseLayerFilter(Broad_phase_layers::NON_MOVING),
        JPH::SpecifiedObjectLayerFilter(Layers::NON_MOVING));

    if (ray_collector.GetEarlyOutFraction() <= 1.0f)
    {
        return_result.success      = true;
        return_result.hit_distance = ray.mDirection.Length() * ray_collector.GetEarlyOutFraction();
        return_result.hit_point    = ray_collector.m_contact_position;
        return_result.hit_normal   = ray_collector.m_contact_normal;
    }

    // Draw debug line in raycast.
    vec4 color_1{ 0.5f, 0.0f, 0.0f, 1.0f };
    if (return_result.success)
        glm_vec4_copy(vec4{ 1.0f, 0.0f, 0.0f, 1.0f }, color_1);
    JPH::RVec3 pos_2{ origin + direction_and_magnitude };
#if !BTZC_REFACTOR_TO_ENTT
    get_main_debug_line_pool()
        .emplace_debug_line({ { origin.GetX(), origin.GetY(), origin.GetZ() },
                              { pos_2.GetX(), pos_2.GetY(), pos_2.GetZ() },
                              { color_1[0], color_1[1], color_1[2], color_1[3] },
                              { 0.85f, 0.85f, 0.85f, 1.0f} });
#endif  // !BTZC_REFACTOR_TO_ENTT

    return return_result;
}
