////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief   Collection of components in a bare-bones ECS (entity component system) to be attached
///          to game objects (entities).
///
/// @details Steps for adding a component:
///            1. Here, create a struct in the `component` namespace.
///            2. Go to `component_registry.cpp`.
///            3. There, add struct to ECS in `register_all_components()` func using the macro.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "cglm/types.h"


namespace BT
{
namespace component
{

struct Character_controller_move_delta
{
    vec3 move_delta;
};

}  // namespace component
}  // namespace BT
