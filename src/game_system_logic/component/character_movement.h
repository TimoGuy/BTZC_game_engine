#pragma once

#include "btglm.h"
#include "btjson.h"
#include "uuid/uuid.h"

#include <array>


namespace BT
{
namespace component
{

/// Tag to mark the player character. `player_idx` marks which player this player character is.
struct Player_character
{
    size_t player_idx{ 0 };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Player_character,
        player_idx
    );
};

/// World space input for character controller movement.
/// NPC AI would output to this struct. Player input would output to this struct.
struct Character_world_space_input
{
    vec3s ws_flat_clamped_input{ 0, 0, 0 };

    bool jump_pressed{ false };
    bool prev_jump_pressed{ false };
    bool crouch_pressed{ false };
    bool prev_crouch_pressed{ false };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Character_world_space_input,
                                                ws_flat_clamped_input);
};

/// Holds movement state for characters. Also contains settings for movement parameters.
/// @NOTE: Only settings are serialized.
struct Character_mvt_state
{
    struct Grounded_state
    {
        bool allow_grounded_sliding{ false };
        float_t facing_angle{ 0.0f };
    } grounded_state;

    struct Airborne_state
    {
        float_t input_facing_angle{ 0.0f };
    } airborne_state;

    struct Settings
    {
        float_t jump_speed{ 30.0f };

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Settings,
                                                    jump_speed);
    } settings;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Character_mvt_state, settings);
};

/// Stores the reference to the entity with the transform of the display representation. This is
/// used in the case of a character, where the character controller physics object does not rotate
/// at all.
struct Display_repr_transform_ref
{
    UUID display_repr_uuid;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Display_repr_transform_ref,
        display_repr_uuid
    );
};

/// Communicates state of the animated model from the `input_controlled_character_movement` system.
struct Character_mvt_animated_state
{
    /// UUID that contains the animator to affect.
    UUID affecting_animator_uuid;

    struct Write_to_animator_data
    {
        bool is_moving{ false };
        bool on_turnaround{ false };
        bool is_grounded{ false };
        bool on_jump{ false };
        bool on_attack{ false };
        bool on_parry_hurt{ false };
        bool on_guard_hurt{ false };
        bool on_receive_hurt{ false };
        bool is_guarding{ false };
    } write_to_animator_data;

    struct State
    {
        bool prev_attack_pressed{ false };
    } state;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Character_mvt_animated_state,
        affecting_animator_uuid
    );
};

}  // namespace component
}  // namespace BT
