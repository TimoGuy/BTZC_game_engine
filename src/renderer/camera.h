#pragma once

#include "../uuid/uuid.h"
#include "../input_handler/input_handler.h"
#include "btglm.h"
#include <cmath>
#include <functional>
#include <memory>

using std::function;
using std::unique_ptr;


namespace BT
{

class Camera
{
public:
    Camera();
    ~Camera();

    // Callbacks.
    void set_callbacks(function<void(bool)>&& cursor_lock_fn);

    // GPU camera.
    void set_fov(float_t fov);  // `fov` in radians.
    void set_aspect_ratio(float_t width, float_t height);
    void set_z_clip_plane(float_t z_near, float_t z_far);

    void update_camera_matrices();
    void fetch_calculated_camera_matrices(mat4& out_projection,
                                          mat4& out_view,
                                          mat4& out_projection_view);
    void get_view_direction(vec3& out_view_direction);

    // Camera frontend.
    bool is_static_cam();
    bool is_capture_fly();
    bool is_follow_orbit();
    void update_frontend(Input_handler::State const& input_state, float_t delta_time);
    bool is_mouse_captured();
    bool is_ortho_cam();
    bool is_ortho_cam_dragging();
    std::string get_ortho_cam_dragging_tooltip_text() const;

    void request_cam_state_static();
    void request_cam_state_follow_orbit();
    void request_cam_state_ortho(vec3 focus_position, vec3 look_direction);

    // ImGui.
    void set_hovering_over_game_viewport(bool hovering);
    void render_imgui(std::function<std::string(char const* const)> const& window_name_w_context_fn);

private:
    struct Data;
    unique_ptr<Data> m_data;

    // Frontend functions.
    void change_frontend_state(uint32_t to_state);
    void update_frontend_static(Input_handler::State const& input_state,
                                bool on_press_le_rclick_cam,
                                bool on_press_le_f1,
                                bool first);
    void update_frontend_capture_fly(Input_handler::State const& input_state,
                                     float_t delta_time,
                                     bool on_release_le_rclick_cam);
    void update_frontend_follow_orbit(Input_handler::State const& input_state,
                                      float_t delta_time,
                                      bool on_press_le_f1,
                                      bool first);
    void update_frontend_orthographic(Input_handler::State const& input_state,
                                      float_t delta_time,
                                      bool first);
};

}  // namespace BT
