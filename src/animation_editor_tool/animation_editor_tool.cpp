#include "animation_editor_tool.h"


BT::anim_editor::Editor_state BT::anim_editor::s_editor_state;

void BT::anim_editor::reset_editor_state()
{
    s_editor_state = Editor_state{};
}
