#pragma once

#include "Jolt/Jolt.h"
#include "Jolt/Math/MathTypes.h"
#include "Jolt/Math/Real.h"
#include "Jolt/Math/Vec3.h"
#include "physics_engine.h"


namespace BT::Raycast_helper
{

void set_physics_engine(Physics_engine& physics_engine);

struct Raycast_result
{
    bool success{ false };
    float_t hit_distance;
    JPH::RVec3 hit_point;
    JPH::Vec3 hit_normal;
};

Raycast_result raycast(JPH::RVec3Arg origin, JPH::Vec3Arg direction_and_magnitude);

}  // namespace BT::Raycast_helper