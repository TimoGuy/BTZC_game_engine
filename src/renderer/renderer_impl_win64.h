#pragma once

#include "../input_handler/input_handler.h"
#include "cglm/cglm.h"
#include "render_object.h"
#include "renderer.h"
#include <cstdint>
#include <string>

using std::string;


namespace BT
{

class Renderer::Impl
{
public:
    Impl(Input_handler& input_handler, string const& title);
    ~Impl();

    bool get_requesting_close();
    void poll_events();
    void render();

    inline Input_handler& get_input_handler() { return m_input_handler; }

    void submit_window_focused(bool focused);
    void submit_window_iconified(bool iconified);
    void submit_window_dims(int32_t width, int32_t height);

    void fetch_cached_camera_matrices(mat4& out_projection, mat4& out_view, mat4& out_projection_view);

    render_object_key_t emplace_render_object(Render_object&& rend_obj);
    void remove_render_object(render_object_key_t key);  // @INCOMPLETE.

private:
    void setup_glfw_and_opengl46_hints();
    
    struct Window_dimensions
    {
        int32_t width;
        int32_t height;
    } m_window_dims;
    bool m_window_dims_changed{ false };
    bool m_window_focused{ true };
    bool m_window_iconified{ false };

    void calc_ideal_standard_window_dim_and_apply_center_hints();

    inline static void* m_window_handle{ nullptr };
    Input_handler& m_input_handler;

    void create_window_with_gfx_context(string const& title);

    // ImGui.
    void setup_imgui();
    void render_imgui();

    // 3D camera.
    struct Camera
    {
        float_t fov;
        float_t aspect_ratio;
        float_t z_near;
        float_t z_far;
        vec3 position{ 0.0f, 0.0f, 0.0f };
        vec3 view_direction{ 0.0f, 0.0f, 1.0f };
    } m_camera;

    void setup_3d_camera();
    void calc_3d_aspect_ratio();

    struct Camera_matrices
    {
        mat4 projection;
        mat4 view;
        mat4 projection_view;
    } m_camera_matrices_cache;

    void update_camera_matrices();

    // Scene.
    vector<Render_object> m_render_objects;
    Render_layer m_active_render_layers{ Render_layer::RENDER_LAYER_DEFAULT |
                                         Render_layer::RENDER_LAYER_LEVEL_EDITOR };

    // Display rendering.
    uint32_t m_ldr_fbo{ 0 };
    uint32_t m_ldr_color_texture{ 0 };
    uint32_t m_ldr_depth_rbo{ 0 };
    void create_ldr_fbo();

    void begin_new_display_frame();
    void render_hdr_color_to_ldr_framebuffer(bool to_display_frame);
    void present_display_frame();

    // HDR rendering.
    uint32_t m_hdr_fbo{ 0 };
    uint32_t m_hdr_color_texture{ 0 };
    uint32_t m_hdr_depth_rbo{ 0 };
    void create_hdr_fbo();
    void render_scene_to_hdr_framebuffer();

    // Helper functions.
    void render_ndc_cube();
    void render_ndc_quad();
};

}  // namespace BT

