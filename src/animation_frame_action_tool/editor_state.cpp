#include "editor_state.h"


BT::anim_frame_action::Editor_state BT::anim_frame_action::s_editor_state;

void BT::anim_frame_action::reset_editor_state()
{
    s_editor_state = Editor_state{};
}
