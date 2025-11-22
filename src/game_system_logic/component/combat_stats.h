#pragma once

#include "btjson.h"

#include <cmath>
#include <cstdint>
#include <limits>


namespace BT
{
namespace component
{

/// Combat stats prior to any modifiers.
struct Base_combat_stats_data
{
    int32_t dmg_pts{ 0 };
    int32_t dmg_def_pts{ 0 };

    int32_t posture_dmg_pts{ 0 };
    int32_t posture_dmg_def_pts{ 0 };

    double_t atk_receive_debounce_time{ 0.1 };  // Min time between attacks in seconds.
    double_t prev_atk_received_time{ std::numeric_limits<double_t>::lowest() };  // DO NOT INCLUDE IN SERIALIZATION.

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Base_combat_stats_data,
        dmg_pts,
        dmg_def_pts,
        posture_dmg_pts,
        posture_dmg_def_pts,
        atk_receive_debounce_time
    );
};

}  // namespace component
}  // namespace BT
