#pragma once

#include <memory>
#include <string>

using std::string;


namespace renderer
{

class Renderer
{
public:
    // Setup and teardown renderer.
    Renderer(string const& title);
    ~Renderer();

    bool get_requesting_close();
    void poll_events();
    void render();

    // Create render graph bits.

    // Create render objects.

    // Create render object render job for main view z-prepass.

    // Create render object render job for main view opaque pass.

    // Create render object render job for shadow view.

    // Create 3D model.

    // Create image texture.


private:
    class Impl;
    std::unique_ptr<Impl> m_pimpl;
};

}  // namespace renderer
