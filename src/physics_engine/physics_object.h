#pragma once

#include "cglm/cglm.h"
#include "Jolt/Jolt.h"
#include "Jolt/Math/Real.h"

using JPH::Real;


namespace BT
{

using rvec3 = JPH::Real[3];

enum Physics_object_type
{
    PHYSICS_OBJECT_TYPE_KINEMATIC_TRIANGLE_MESH = 0,
    PHYSICS_OBJECT_TYPE_CHARACTER_CONTROLLER,
    NUM_PHYSICS_OBJECT_TYPES
};

class Physics_object
{
public:
    void get_transform_for_rendering(bool as_interpolated, rvec3& position, versor rotation);
};

}  // namespace BT
