#include "renderer.h"

#include "../input_handler/input_handler.h"
#include "renderer_impl_win64.h"
#include <memory>
#include <string>

using std::string;


BT::Renderer::Renderer(Input_handler& input_handler, ImGui_renderer& imgui_renderer, string const& title)
    : m_pimpl{ std::make_unique<Impl>(*this, imgui_renderer, input_handler, title) }
{
}

BT::Renderer::~Renderer() = default;  // @NOTE: For smart pimpl.

bool BT::Renderer::get_requesting_close()
{
    return m_pimpl->get_requesting_close();
}

void BT::Renderer::poll_events()
{
    m_pimpl->poll_events();
}

void BT::Renderer::render(float_t delta_time, function<void()>&& debug_views_render_fn)
{
    m_pimpl->render(delta_time, std::move(debug_views_render_fn));
}

// Camera read.
void BT::Renderer::fetch_camera_matrices(mat4& out_projection,
                                         mat4& out_view,
                                         mat4& out_projection_view)
{
    m_pimpl->fetch_cached_camera_matrices(out_projection, out_view, out_projection_view);
}

BT::Camera* BT::Renderer::get_camera_obj()
{
    return m_pimpl->get_camera_obj();
}

// Create render objects.
BT::Render_object_pool& BT::Renderer::get_render_object_pool()
{
    return m_pimpl->get_render_object_pool();
}

// Imgui.
void BT::Renderer::render_imgui_game_view()
{
    m_pimpl->render_imgui_game_view();
}
