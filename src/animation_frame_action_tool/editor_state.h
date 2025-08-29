#pragma once

#include <string>
#include <map>


namespace BT
{

class Model;

namespace anim_frame_action
{

struct Runtime_data;

struct Editor_state
{
    Runtime_data* working_timeline{ nullptr };
    Model const* working_model{ nullptr };
    std::map<std::string, size_t> anim_name_to_idx_map;
    size_t selected_anim_idx{ 0 };
    size_t selected_anim_num_frames{ 0 };
    size_t anim_current_frame{ 0 };
    bool is_working_timeline_dirty{ false };
};

extern Editor_state s_editor_state;

void reset_editor_state();

}  // namespace anim_frame_action
}  // namespace BT
