#include "raycast_helper.h"

#include "physics_engine_impl_layers.h"
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
