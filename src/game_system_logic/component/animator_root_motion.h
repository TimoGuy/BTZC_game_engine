#pragma once

#include "btjson.h"
#include "btglm.h"

#include <cmath>


namespace BT
{
namespace component
{

/// Tag component to be used for root motion.
struct Animator_root_motion
{   // Settings.
    float_t root_motion_multiplier{ 1.0f };

    // Captured values.
    vec3 delta_pos;
    float_t turn_speed{ 0 };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Animator_root_motion, root_motion_multiplier);
};

}  // namespace component
}  // namespace BT
