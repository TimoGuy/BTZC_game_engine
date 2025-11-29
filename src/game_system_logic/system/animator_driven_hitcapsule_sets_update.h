#pragma once


namespace BT
{
namespace system
{

/// Ticks an update for all hitcapsule sets that have an animator attached.
/// @NOTE: This also updates the animator for the simulation loop right here!!!
void animator_driven_hitcapsule_sets_update();

}  // namespace system
}  // namespace BT
