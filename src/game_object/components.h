////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief   Collection of components in a bare-bones ECS (entity component system) to be attached
///          to game objects (entities).
///
/// @details Steps for adding a component:
///            1. Here, create a struct in the `component` namespace.
///            2. Go to `component_registry.cpp`. There, add struct to ECS in
///               `register_all_components()` func using the macro.
///            3. (OPTIONAL) if declared any methods inside the struct, define them in
///               `components.cpp`
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "refactor_to_entt.h"
#if !BTZC_REFACTOR_TO_ENTT

#include "../uuid/uuid.h"
#include "btjson.h"
#include "btglm.h"

#include <array>
#include <string>
#include <vector>


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

struct Component_transform
{   // Parent-child relationships for transform hierarchy.
    UUID parent_entity;
    std::vector<UUID> children_entities;

    // Global transform.
    rvec3s  position{ 0, 0, 0 };
    versors rotation{ 0, 0, 0, 1 };
    vec3s   scale{ 1, 1, 1 };

    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Component_transform,
        parent_entity,
        children_entities,
        position,
        rotation,
        scale
    );
};

struct Component_model_animator
{
    ~Component_model_animator();

    std::string animatable_model_name;
    std::string animator_template_name;
    std::string anim_frame_action_ctrls;

    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Component_model_animator,
        animatable_model_name,
        animator_template_name
    );

    /// Builds and returns `product`.
    Model_animator& get_product();

private:
    /// Fully built component.
    Model_animator* product{ nullptr };  // Use owning raw pointer for serialization compatibility.
};

// struct Component_physics_object
// {
//     Physics_object* phys_obj{ nullptr };

//     /// Serialization/deserialization.
//     NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
//         Component_physics_object,
//         phys_obj
//     );
// };

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
        working_anim_state_idx,
        prev_anim_frame
    );
};

}  // namespace component_system
}  // namespace BT

#endif  // !BTZC_REFACTOR_TO_ENTT
