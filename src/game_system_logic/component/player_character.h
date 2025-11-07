#pragma once

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

/// Holds movement state for characters. Also contains settings for movement parameters.
/// @NOTE: Only settings are serialized.
struct Character_mvt_state
{
    bool m_prev_jump_pressed{ false };
    bool m_prev_crouch_pressed{ false };

    struct Grounded_state
    {
        float_t speed{ 0.0f };
        float_t facing_angle{ 0.0f };
        bool turnaround_enabled{ false };
    } m_grounded_state;

    struct Airborne_state
    {
        float_t input_facing_angle{ 0.0f };
    } m_airborne_state;

    struct Settings
    {
        float_t crouched_speed{ 5.0f };
        float_t standing_speed{ 15.0f };

        float_t grounded_acceleration{ 80.0f };
        float_t grounded_deceleration{ 120.0f };

        struct Contextual_turn_speed
        {
            float_t turn_speed;
            float_t max_speed_of_context;

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(Contextual_turn_speed, turn_speed, max_speed_of_context);
        };
        std::array<Contextual_turn_speed, 3> grounded_turn_speeds{
            Contextual_turn_speed{ 1000000.0f, crouched_speed + 0.1f },
            Contextual_turn_speed{ 10.0f, standing_speed + 0.1f },
            Contextual_turn_speed{ 5.0f, 50.0f } };

        float_t airborne_acceleration{ 60.0f };
        float_t airborne_turn_speed{ 7.5f };
        float_t jump_speed{ 30.0f };

        NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Settings,
                                                    crouched_speed,
                                                    standing_speed,
                                                    grounded_acceleration,
                                                    grounded_deceleration,
                                                    grounded_turn_speeds,
                                                    airborne_acceleration,
                                                    airborne_turn_speed,
                                                    jump_speed);
    } m_settings;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(Character_mvt_state, m_settings);
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

}  // namespace component
}  // namespace BT
