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
struct _Dev_animation_editor_agent
{
    anim_frame_action::Runtime_data_controls* working_timeline_copy{ nullptr };  // Idc if this is a memory leak (raw pointer).  -Thea 2025/08/30
    Model const* working_model{ nullptr };
    Model_animator* working_model_animator{ nullptr };
    std::map<std::string, size_t> anim_state_name_to_idx_map;
    size_t selected_anim_state_idx{ 0 };
    size_t selected_anim_num_frames{ 0 };
    size_t anim_current_frame{ 0 };
    bool is_working_timeline_dirty{ false };

    // @NOTE: In order to make this component be able to be serialized, one field has to be able to
    // be serialized, so I picked this one lol.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        _Dev_animation_editor_agent,
        anim_current_frame
    );
};

}  // namespace component
}  // namespace BT
