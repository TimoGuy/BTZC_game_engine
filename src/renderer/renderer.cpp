#include "renderer.h"

#include "renderer_impl_win64.h"
#include <memory>


renderer::Renderer::Renderer()
    : m_pimpl{ std::make_unique<Impl>() }
{
}

// @NOTE: For smart pimpl.
renderer::Renderer::~Renderer() = default;
