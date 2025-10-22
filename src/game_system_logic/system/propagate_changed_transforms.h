#pragma once

#include <entt/entity/fwd.hpp>


namespace BT
{
namespace system
{

/// Searches for all transforms that have a "changed" tag attached and propagate them thru the
/// transform hierarchy.
void propagate_changed_transforms(entt::registry& reg);

}  // namespace system
}  // namespace BT
