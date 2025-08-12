#include "input_handler.h"

#include "input_codes.h"


void BT::Input_handler::report_keyboard_input_change(int32_t key_code, bool pressed)
{   // Update direct lookup table.
    if (m_direct_lookup_key_state_map.find(key_code)
        == m_direct_lookup_key_state_map.end())
        m_direct_lookup_key_state_map.emplace(key_code, pressed);
    else
        m_direct_lookup_key_state_map.at(key_code) = pressed;

    // @HARDCODE.
    bool update_move_wasd{ false };

    // @NOTE: Since all of these input changes happen one at a time, the int-based inputs
    //   will keep track of the exact input by adding or removing the input as a delta.
    constexpr int32_t k_pos_mod{ 1 };
    constexpr int32_t k_neg_mod{ -1 };

    switch (key_code)
    {
        case BT_KEY_W:
            m_state.ui_navigate.y.val += (pressed ? k_pos_mod : -k_pos_mod);
            update_move_wasd = true;
            break;

        case BT_KEY_A:
            m_state.ui_navigate.x.val += (pressed ? k_neg_mod : -k_neg_mod);
            update_move_wasd = true;
            break;

        case BT_KEY_S:
            m_state.ui_navigate.y.val += (pressed ? k_neg_mod : -k_neg_mod);
            update_move_wasd = true;
            break;

        case BT_KEY_D:
            m_state.ui_navigate.x.val += (pressed ? k_pos_mod : -k_pos_mod);
            update_move_wasd = true;
            break;

        case BT_KEY_E:
            m_state.interact.val = pressed;
            m_state.ui_confirm.val = pressed;
            m_state.le_move_world_y_axis.val += (pressed ? k_pos_mod : -k_pos_mod);
            break;

        case BT_KEY_LEFT_CONTROL:
            m_state.crouch.val = pressed;
            m_state.le_lctrl_mod.val = pressed;
            break;

        case BT_KEY_LEFT_SHIFT:
            m_state.sprint.val = pressed;
            m_state.le_lshift_mod.val = pressed;
            break;

        case BT_KEY_SPACE:
            m_state.jump.val = pressed;
            m_state.ui_confirm.val = pressed;
            break;

        case BT_KEY_ESCAPE:
            m_state.pause.val = pressed;
            m_state.ui_cancel.val = pressed;
            break;

        case BT_KEY_Q:
            m_state.ui_cancel.val = pressed;
            m_state.le_move_world_y_axis.val += (pressed ? k_neg_mod : -k_neg_mod);
            break;

        case BT_KEY_F1:
            m_state.le_f1.val = pressed;
            break;
    }

    if (update_move_wasd)
    {
        // Update move bc wasd change occurred.
        m_state.move.x.val = static_cast<float_t>(m_state.ui_navigate.x.val);
        m_state.move.y.val = static_cast<float_t>(m_state.ui_navigate.y.val);
    }
}

void BT::Input_handler::report_mouse_button_input_change(int32_t button_code, bool pressed)
{
    // @HARDCODE.
    switch (button_code)
    {
        case BT_MOUSE_BUTTON_LEFT:
            m_state.attack.val     = pressed;
            m_state.ui_confirm.val = pressed;
            m_state.le_select.val  = pressed;
            break;

        case BT_MOUSE_BUTTON_RIGHT:
            m_state.ui_cancel.val     = pressed;
            m_state.le_rclick_cam.val = pressed;
            break;
    }
}

void BT::Input_handler::report_mouse_position_offset(float_t x_offset, float_t y_offset)
{
    m_mouse_pos_offset_x = x_offset;
    m_mouse_pos_offset_y = y_offset;
}

void BT::Input_handler::report_mouse_position_change(float_t x, float_t y)
{
    x -= m_mouse_pos_offset_x;
    y -= m_mouse_pos_offset_y;

    // @HARDCODE.
    // Use ui_cursor_pos as prev state.
    m_state.look_delta.x.val += (x - m_state.ui_cursor_pos.x.val);
    m_state.look_delta.y.val += (y - m_state.ui_cursor_pos.y.val);

    m_state.ui_cursor_pos.x.val = x;
    m_state.ui_cursor_pos.y.val = y;
}

void BT::Input_handler::report_mouse_scroll_input_change(float_t dx, float_t dy)
{
    // @HARDCODE.
    (void)dx;
    m_state.ui_scroll_delta.val += dy;
}

void BT::Input_handler::clear_look_delta()
{
    m_state.look_delta = {};
}

void BT::Input_handler::clear_ui_scroll_delta()
{
    m_state.ui_scroll_delta = {};
}
