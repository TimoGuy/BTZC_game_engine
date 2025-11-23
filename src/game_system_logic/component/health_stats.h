#pragma once

#include "btjson.h"

#include <cstdint>


namespace BT
{
namespace component
{

/// Data for health of an entity.
struct Health_stats_data
{
    int32_t max_health_pts{ 100 };
    int32_t health_pts{ max_health_pts };  // 0 causes death trigger.

    int32_t max_posture_pts{ 100 };
    int32_t posture_pts{ 0 };              // `max_posture_pts` causes posture break. Min is 0.
    float_t posture_pts_regen_rate{ 20 };  // N per sec to decrement `posture_pts`.

    bool is_invincible{ false };           // `true` prevents death trigger and decrement of `health_pts`.

    double_t atk_receive_debounce_time{ 0.1 };  // Min time between attacks in seconds.
    double_t prev_atk_received_time{ std::numeric_limits<double_t>::lowest() };  // DO NOT INCLUDE IN SERIALIZATION.

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Health_stats_data,
        max_health_pts,
        health_pts,
        max_posture_pts,
        posture_pts,
        posture_pts_regen_rate,
        is_invincible,
        atk_receive_debounce_time
    );
};

}  // namespace component
}  // namespace BT
