#include "input_handler/input_handler.h"
#include "renderer/mesh.h"  // @DEBUG
#include "renderer/renderer.h"

#include <cstdint>
#include <fmt/base.h>


int32_t main()
{
    fmt::println("Hello, {}", "Bootiful Warld!");

    BT::Input_handler main_input_handler;
    BT::Renderer main_renderer{ main_input_handler,
                                "Untitled Zelda-like Collectathon Game" };

    BT::Model asdfasdf{ "C:/Users/Timo/Desktop/asdfasdf.obj",
                        "default_material" };

    // Main loop.
    while (!main_renderer.get_requesting_close())
    {
        main_renderer.poll_events();

        // Logic here.

        main_renderer.render();
    }

    return 0;
}
