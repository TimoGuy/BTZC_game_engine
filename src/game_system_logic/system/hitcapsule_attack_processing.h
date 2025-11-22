#pragma once

#include <cmath>


namespace BT
{
namespace system
{

/// Processes hitcapsule overlaps, then processes attacks from overlaps.
void hitcapsule_attack_processing(float_t delta_time);

}  // namespace system
}  // namespace BT
