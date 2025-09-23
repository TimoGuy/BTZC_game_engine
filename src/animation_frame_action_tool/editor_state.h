#pragma once

#include <string>
#include <map>


namespace BT
{

class Model;
class Model_animator;

namespace anim_frame_action
{

struct Runtime_data_controls;

struct Editor_state
{
    Runtime_data_controls* working_timeline_copy{ nullptr };  // Idc if this is a memory leak (raw pointer).  -Thea 2025/08/30
    Model const* working_model{ nullptr };
    Model_animator* working_model_animator{ nullptr };
    std::map<std::string, size_t> anim_state_name_to_idx_map;
    size_t selected_anim_state_idx{ 0 };
    size_t selected_anim_num_frames{ 0 };
    size_t anim_current_frame{ 0 };
    bool is_working_timeline_dirty{ false };
};

extern Editor_state s_editor_state;

void reset_editor_state();

}  // namespace anim_frame_action
}  // namespace BT
