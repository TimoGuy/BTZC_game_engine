#pragma once

#include "../input_handler/input_handler.h"
#include "camera_read_ifc.h"
#include "imgui_renderer.h"
#include "render_object.h"
#include <functional>
#include <memory>
#include <string>

using std::function;
using std::string;


namespace BT
{

class Camera;

class Renderer : public Camera_read_ifc
{
public:
    // Setup and teardown renderer.
    Renderer(Input_handler& input_handler, ImGui_renderer& imgui_renderer, string const& title);
    ~Renderer();

    bool get_requesting_close();
    void poll_events();
    void render(float_t delta_time, function<void()>&& debug_views_render_fn);

    // Camera read.
    void fetch_camera_matrices(mat4& out_projection,
                               mat4& out_view,
                               mat4& out_projection_view) override;
    Camera* get_camera_obj();

    // Create render graph bits.

    // Create render objects.
    Render_object_pool& get_render_object_pool();

    // Create render object render job for main view z-prepass.

    // Create render object render job for main view opaque pass.

    // Create render object render job for shadow view.

    // Create 3D model.

    // Create image texture.

    // App settings.
    void save_state_to_app_settings() const;

    // Imgui.
    void render_imgui_game_view();

    class Impl;

private:
    std::unique_ptr<Impl> m_pimpl;
};

}  // namespace BT
