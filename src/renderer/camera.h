#pragma once

#include "../input_handler/input_handler.h"
#include "cglm/cglm.h"
#include "render_object.h"
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

    // Camera frontend.
    void set_follow_object(Render_object const* render_object);
    void request_follow_orbit();
    bool is_follow_orbit();
    void update_frontend(Input_handler::State const& input_state, float_t delta_time);
    bool is_mouse_captured();

    // ImGui.
    void set_hovering_over_game_viewport(bool hovering);
    void render_imgui();

private:
    struct Data;
    unique_ptr<Data> m_data;

    // Frontend functions.
    void change_frontend_state(uint32_t to_state);
    void update_frontend_static(bool on_press_le_rclick_cam);
    void update_frontend_capture_fly(Input_handler::State const& input_state,
                                     float_t delta_time,
                                     bool on_release_le_rclick_cam);
    void update_frontend_follow_orbit();
};

}  // namespace BT
