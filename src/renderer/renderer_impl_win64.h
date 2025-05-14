#pragma once

#include "renderer.h"
#include <cstdint>


namespace renderer
{

class Renderer::Impl
{
private:
    bool m_created;
    
    struct Window_dimensions
    {
        int32_t width;
        int32_t height;
    } m_window_dims;
};

}  // namespace renderer

