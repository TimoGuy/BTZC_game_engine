#pragma once

#include "btjson.h"


namespace BT
{
    
class Model;
class Model_animator;

namespace anim_frame_action
{
struct Runtime_data_controls;
}

namespace component
{

/// (DEV COMPONENT!!!) Editor agent for editing animation frame actions.
struct _Dev_animation_frame_action_editor_agent
{
    bool request_reset_editor_state{ true };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        _Dev_animation_frame_action_editor_agent,
        request_reset_editor_state
    );
};

}  // namespace component
}  // namespace BT
