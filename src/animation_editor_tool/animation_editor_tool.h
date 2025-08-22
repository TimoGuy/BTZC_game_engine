#pragma once


namespace BT
{

class Model;

namespace anim_editor
{

struct Editor_state
{
    Model const* working_model{ nullptr };
    size_t num_animations{ 0 };
};

extern Editor_state s_editor_state;

void reset_editor_state();

}  // namespace anim_editor
}  // namespace BT
