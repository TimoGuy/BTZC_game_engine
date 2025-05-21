#include "renderer.h"

#include "../input_handler/input_handler.h"
#include "renderer_impl_win64.h"
#include <memory>
#include <string>

using std::string;


BT::Renderer::Renderer(Input_handler& input_handler, string const& title)
    : m_pimpl{ std::make_unique<Impl>(input_handler, title) }
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

void BT::Renderer::render()
{
    m_pimpl->render();
}

// Camera read.
void BT::Renderer::fetch_camera_matrices(mat4& out_projection,
                                         mat4& out_view,
                                         mat4& out_projection_view)
{
    m_pimpl->fetch_cached_camera_matrices(out_projection, out_view, out_projection_view);
}
