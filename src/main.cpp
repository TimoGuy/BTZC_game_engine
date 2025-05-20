#include "input_handler/input_handler.h"
#include "renderer/material.h"  // @DEBUG
#include "renderer/material_impl_opaque_shaded.h"  // @DEBUG
#include "renderer/mesh.h"  // @DEBUG
#include "renderer/renderer.h"
#include "renderer/shader.h"  // @DEBUG

#include <cstdint>
#include <fmt/base.h>


int32_t main()
{
    fmt::println("Hello, {}", "Bootiful Warld!");

    BT::Input_handler main_input_handler;
    BT::Renderer main_renderer{ main_input_handler,
                                "Untitled Zelda-like Collectathon Game" };

    BT::Shader_bank::emplace_shader(
        "color_shaded",
        BT::Shader{ "color_shaded.vert", "color_shaded.frag" });

    BT::Material_bank::emplace_material(
        "default_material",
        std::unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_shaded(vec3{ 0.5f, 0.225f, 0.3f })));

    BT::Model_bank::emplace_model(
        "default_model",
        BT::Model{ "C:/Users/Timo/Desktop/asdfasdf.obj",
                   "default_material" });

    // Main loop.
    while (!main_renderer.get_requesting_close())
    {
        main_renderer.poll_events();

        // Logic here.

        main_renderer.render();
    }

    return 0;
}
