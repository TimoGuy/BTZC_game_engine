////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief   Collection of components in a bare-bones ECS (entity component system) to be attached
///          to game objects (entities).
///
/// @details Steps for adding a component:
///            1. Here, create a struct in the `component` namespace.
///            2. Go to `component_registry.cpp`. There, add struct to ECS in
///               `register_all_components()` func using the macro.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "cglm/types.h"

#include <array>


namespace BT
{

/// Forward declarations.
class Model;
class Model_animator;
class Hitcapsule_group_set;
class Physics_object_type_impl_ifc;
class Render_object;
namespace anim_frame_action
{
struct Runtime_data_controls;
}

namespace component_system
{

struct Component_model_animator
{
    Model_animator* animator{ nullptr };
};

struct Component_hitcapsule_group_set
{
    Hitcapsule_group_set* hitcapsule_grp_set{ nullptr };
};

struct Component_char_con_movement_state
{
    Physics_object_type_impl_ifc* char_con_impl{ nullptr };

    // Input history.
    bool prev_jump_pressed{ false };
    bool prev_crouch_pressed{ false };

    // Settings.
    float_t crouched_speed{ 5.0f };
    float_t standing_speed{ 15.0f };

    float_t grounded_acceleration{ 80.0f };
    float_t grounded_deceleration{ 120.0f };

    struct Contextual_turn_speed
    {
        float_t turn_speed;
        float_t max_speed_of_context;
    };
    std::array<Contextual_turn_speed, 3> grounded_turn_speeds{
        Contextual_turn_speed{ 1000000.0f, crouched_speed + 0.1f },
        Contextual_turn_speed{ 10.0f, standing_speed + 0.1f },
        Contextual_turn_speed{ 5.0f, 50.0f }
    };

    float_t airborne_acceleration{ 60.0f };
    float_t airborne_turn_speed{ 7.5f };
    float_t jump_speed{ 30.0f };

    struct Grounded_state
    {
        float_t speed{ 0.0f };
        float_t facing_angle{ 0.0f };
        bool    turnaround_enabled{ false };
    } grounded_state;

    struct Airborne_state
    {
        float_t input_facing_angle{ 0.0f };
    } airborne_state;
};

struct Component_anim_editor_tool_communicator_state
{
    Render_object* rend_obj{ nullptr };

    Model const* prev_working_model{ nullptr };
    uint32_t working_anim_state_idx{ (uint32_t)-1 };
    size_t prev_anim_frame{ (size_t)-1 };

    anim_frame_action::Runtime_data_controls const* prev_working_timeline_copy{ nullptr };
};

}  // namespace component_system
}  // namespace BT
