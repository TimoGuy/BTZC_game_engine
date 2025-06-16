#pragma once

#include "../input_handler/input_handler.h"
#include "camera.h"
#include "cglm/cglm.h"
#include "imgui_renderer.h"
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
    Impl(Renderer& renderer, ImGui_renderer& imgui_renderer, Input_handler& input_handler, string const& title);
    ~Impl();

    bool get_requesting_close();
    void poll_events();
    void render(float_t delta_time);

    inline Input_handler& get_input_handler() { return m_input_handler; }

    void submit_window_focused(bool focused);
    void submit_window_iconified(bool iconified);
    void submit_window_dims(int32_t width, int32_t height);

    void fetch_cached_camera_matrices(mat4& out_projection, mat4& out_view, mat4& out_projection_view);
    Camera* get_camera_obj();

    Render_object_pool& get_render_object_pool();

    void render_imgui_game_view();

private:
    Renderer& m_renderer;
    ImGui_renderer& m_imgui_renderer;

    void setup_glfw_and_opengl46_hints();
    
    struct Window_dimensions
    {
        int32_t width;
        int32_t height;
    } m_window_dims;
    bool m_window_focused{ true };
    bool m_window_iconified{ false };

    void calc_ideal_standard_window_dim_and_apply_center_hints();

    inline static void* m_window_handle{ nullptr };
    Input_handler& m_input_handler;

    void create_window_with_gfx_context(string const& title);

    Camera m_camera;

    // ImGui.
    void setup_imgui();
    void render_imgui();

    // Scene.
    Render_object_pool m_rend_obj_pool;
    Render_layer m_active_render_layers{ Render_layer::RENDER_LAYER_DEFAULT |
                                         Render_layer::RENDER_LAYER_LEVEL_EDITOR };

    // Display rendering.
    uint32_t m_ldr_fbo{ 0 };
    uint32_t m_ldr_color_texture{ 0 };
    uint32_t m_ldr_depth_rbo{ 0 };
    void create_ldr_fbo();

    Window_dimensions m_main_viewport_dims{ 256, 256 };
    Window_dimensions m_main_viewport_wanted_dims{ 256, 256 };
    bool m_render_to_ldr{ true };

    void begin_new_display_frame();
    void render_hdr_color_to_ldr_framebuffer();
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

