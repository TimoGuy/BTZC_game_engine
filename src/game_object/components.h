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

/// Forward declarations.
class Model_animator;
class Hitcapsule_group_set;

namespace component_system
{

struct Component_model_animator
{
    Model_animator* animator;
};

struct Component_hitcapsule_group_set
{
    Hitcapsule_group_set* hitcapsule_grp_set;
};

struct Character_controller_move_delta
{
    vec3 move_delta;
};

}  // namespace component_system
}  // namespace BT
