#pragma once

#include <cmath>


namespace BT
{

class Input_handler
{
public:
    void report_keyboard_input_change(int32_t key_code, bool pressed);
    void report_mouse_button_input_change(int32_t button_code, bool pressed);
    void report_mouse_position_change(float_t x, float_t y);
    void report_mouse_scroll_input_change(float_t dx, float_t dy);

    void clear_look_delta();
    void clear_ui_scroll_delta();

    struct State
    {
        struct Toggle { bool val{ false }; };
        struct Axis { float_t val{ 0.0f }; };
        struct Iaxis { int32_t val{ 0 }; };

        struct Axis2
        {
            Axis x;
            Axis y;
        };

        struct Iaxis2
        {
            Iaxis x;
            Iaxis y;
        };

        // Gameplay.
        Axis2  move;
        Axis2  look_delta;
        Toggle attack;
        Toggle interact;
        Toggle crouch;
        Toggle sprint;
        Toggle jump;
        Toggle pause;

        // UI.
        Iaxis2 ui_navigate;
        Axis2  ui_cursor_pos;
        Axis   ui_scroll_delta;
        Toggle ui_confirm;
        Toggle ui_cancel;

        // Level editor.
        Axis   le_move_world_y_axis;
        Toggle le_select;
        Toggle le_rclick_cam;
        Toggle le_lshift_mod;
        Toggle le_lctrl_mod;
        Toggle le_f1;
    };

    inline State const& get_input_state() const { return m_state; }

private:
    State m_state;
};

}  // namespace BT
