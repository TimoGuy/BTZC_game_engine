#pragma once

#include <string>
#include <map>


namespace BT
{

class Model;

namespace anim_editor
{

struct Editor_state
{
    Model const* working_model{ nullptr };
    std::map<std::string, size_t> anim_name_to_idx_map;
    size_t selected_anim_idx{ 0 };
    size_t selected_anim_num_frames{ 0 };
    size_t anim_current_frame{ 0 };
};

extern Editor_state s_editor_state;

void reset_editor_state();

}  // namespace anim_editor
}  // namespace BT
