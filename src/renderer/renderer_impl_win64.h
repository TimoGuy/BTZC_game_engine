#pragma once

#include "renderer.h"
#include <cstdint>
#include <string>

using std::string;


namespace renderer
{

class Renderer::Impl
{
public:
    Impl(string const& title);
    ~Impl();

    bool get_requesting_close();
    void poll_events();
    void render();

private:
    bool m_created;

    void setup_glfw_and_opengl46_hints();
    
    struct Window_dimensions
    {
        int32_t width;
        int32_t height;
    } m_window_dims;

    void calc_ideal_standard_window_dim_and_apply_center_hints();

    void* m_window_handle{ nullptr };

    void create_window_with_gfx_context(string const& title);

    void setup_imgui();
    void render_imgui();
};

}  // namespace renderer

