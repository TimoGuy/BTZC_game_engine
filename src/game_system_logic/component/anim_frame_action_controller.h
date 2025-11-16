#pragma once

#include "btjson.h"

#include <string>


namespace BT
{
namespace component
{

/// Serializable data for AFA (animation frame action) controller setup. An animator takes this
/// config and gets an AFA and configures itself to the AFA.
struct Anim_frame_action_controller
{
    std::string anim_frame_action_controller_name{ "" };  // Name for the bank to pull.

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Anim_frame_action_controller,
        anim_frame_action_controller_name
    );
};

}  // namespace component
}  // namespace BT
