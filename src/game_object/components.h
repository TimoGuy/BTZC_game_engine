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
#include "nlohmann/json.hpp"

#include <array>

using json = nlohmann::json;


namespace BT
{

/// Forward declarations ---------------------------------------------------------------------------
class Model;
class Model_animator;
class Hitcapsule_group_set;
class Physics_object;
class Physics_object_type_impl_ifc;
class Render_object;

namespace anim_frame_action
{
struct Runtime_data_controls;
}
///-------------------------------------------------------------------------------------------------

namespace component_system
{

// @NOCHECKIN: @THEA: DELETE ME!!!!
struct Jojoweeooweeoo
{
    int32_t asdf{ 0 };

    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Jojoweeooweeoo,
        asdf
    );
};

struct Component_model_animator
{
    Model_animator* animator{ nullptr };

    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Component_model_animator,
        animator
    );
};

struct Component_hitcapsule_group_set
{
    Hitcapsule_group_set* hitcapsule_grp_set{ nullptr };

    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Component_hitcapsule_group_set,
        hitcapsule_grp_set
    );
};

struct Component_physics_object
{
    Physics_object* phys_obj{ nullptr };

    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Component_physics_object,
        phys_obj
    );
};

struct Component_char_con_movement_state
{
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

        /// Serialization/deserialization.
        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            Contextual_turn_speed,
            turn_speed,
            max_speed_of_context
        );
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

        /// Serialization/deserialization.
        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            Grounded_state,
            speed,
            facing_angle,
            turnaround_enabled
        );
    } grounded_state;

    struct Airborne_state
    {
        float_t input_facing_angle{ 0.0f };

        /// Serialization/deserialization.
        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
            Airborne_state,
            input_facing_angle
        );
    } airborne_state;

    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Component_char_con_movement_state,
        prev_jump_pressed,
        prev_crouch_pressed,
        crouched_speed,
        standing_speed,
        grounded_acceleration,
        grounded_deceleration,
        grounded_turn_speeds,
        airborne_acceleration,
        airborne_turn_speed,
        jump_speed,
        grounded_state,
        airborne_state
    );
};

struct Component_anim_editor_tool_communicator_state
{
    Render_object* rend_obj{ nullptr };

    Model const* prev_working_model{ nullptr };
    uint32_t working_anim_state_idx{ (uint32_t)-1 };
    size_t prev_anim_frame{ (size_t)-1 };

    anim_frame_action::Runtime_data_controls const* prev_working_timeline_copy{ nullptr };

    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Component_anim_editor_tool_communicator_state,
        rend_obj,
        prev_working_model,
        working_anim_state_idx,
        prev_anim_frame,
        prev_working_timeline_copy
    );
};

}  // namespace component_system
}  // namespace BT
