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

// @NOTE: For smart pimpl.
BT::Renderer::~Renderer() = default;

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
