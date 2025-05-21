#pragma once

#include "../input_handler/input_handler.h"
#include "camera_read_ifc.h"
#include "render_object.h"
#include <memory>
#include <string>

using std::string;


namespace BT
{

class Renderer : public Camera_read_ifc
{
public:
    // Setup and teardown renderer.
    Renderer(Input_handler& input_handler, string const& title);
    ~Renderer();

    bool get_requesting_close();
    void poll_events();
    void render();

    // Camera read.
    void fetch_camera_matrices(mat4& out_projection,
                               mat4& out_view,
                               mat4& out_projection_view) override;

    // Create render graph bits.

    // Create render objects.
    using render_object_key_t = uint64_t;
    render_object_key_t emplace_render_object(Render_object&& rend_obj);
    void remove_render_object(render_object_key_t key);

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
