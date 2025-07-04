#include "raycast_helper.h"

#include "../renderer/debug_render_job.h"
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

BT::Raycast_helper::Raycast_result
BT::Raycast_helper::raycast(JPH::RVec3Arg origin, JPH::Vec3Arg direction_and_magnitude)
{
    Raycast_result return_result;

    auto& physics_system{
        *reinterpret_cast<JPH::PhysicsSystem*>(s_physics_engine->get_physics_system_ptr()) };

    // Draw debug line in raycast.
    JPH::RVec3 pos_2{ origin + direction_and_magnitude };
    get_main_debug_line_pool()
        .emplace_debug_line({ { origin.GetX(), origin.GetY(), origin.GetZ() },
                              { pos_2.GetX(), pos_2.GetY(), pos_2.GetZ() },
                              { 1.0f, 0.0f, 0.0f, 1.0f },
                              { 0.85f, 0.85f, 0.85f, 1.0f} });

    JPH::RRayCast ray{ origin, direction_and_magnitude };
    JPH::RayCastResult ray_result;
    if (physics_system.GetNarrowPhaseQuery().CastRay(
        ray,
        ray_result,
        JPH::SpecifiedBroadPhaseLayerFilter(Broad_phase_layers::NON_MOVING),
        JPH::SpecifiedObjectLayerFilter(Layers::NON_MOVING)))
    {
        return_result.success      = true;
        return_result.hit_distance = ray.mDirection.Length() * ray_result.GetEarlyOutFraction();
        return_result.hit_point    = ray.GetPointOnRay(ray_result.GetEarlyOutFraction());
        return_result.hit_normal   = physics_system.GetBodyInterface()
                                         .GetTransformedShape(ray_result.mBodyID)
                                         .GetWorldSpaceSurfaceNormal(ray_result.mSubShapeID2,
                                                                     return_result.hit_point);
    }

    return return_result;
}
