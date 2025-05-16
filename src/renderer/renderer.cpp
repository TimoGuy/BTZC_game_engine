#include "renderer.h"

#include "renderer_impl_win64.h"
#include <memory>
#include <string>

using std::string;


renderer::Renderer::Renderer(string const& title)
    : m_pimpl{ std::make_unique<Impl>(title) }
{
}

// @NOTE: For smart pimpl.
renderer::Renderer::~Renderer() = default;

bool renderer::Renderer::get_requesting_close()
{
    return m_pimpl->get_requesting_close();
}

void renderer::Renderer::poll_events()
{
    m_pimpl->poll_events();
}

void renderer::Renderer::render()
{
    m_pimpl->render();
}
