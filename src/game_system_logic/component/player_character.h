#pragma once

#include "btjson.h"


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

}  // namespace component
}  // namespace BT
