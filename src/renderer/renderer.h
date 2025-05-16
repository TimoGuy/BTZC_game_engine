#pragma once

#include "../input_handler/input_handler.h"
#include <memory>
#include <string>

using std::string;


namespace BT
{

class Renderer
{
public:
    // Setup and teardown renderer.
    Renderer(Input_handler& input_handler, string const& title);
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

    class Impl;

private:
    std::unique_ptr<Impl> m_pimpl;
};

}  // namespace BT
