#pragma once

#include <memory>


namespace renderer
{

class Renderer
{
public:
    Renderer();
    ~Renderer();

private:
    class Impl;
    std::unique_ptr<Impl> m_pimpl;
};

}  // namespace renderer
