#include "renderer/renderer.h"

#include <cstdint>
#include <fmt/base.h>


int32_t main()
{
    fmt::println("Hello, {}", "Bootiful Warld!");

    renderer::Renderer main_renderer{ "Untitled Zelda-like Collectathon Game" };

    // Main loop.
    while (!main_renderer.get_requesting_close())
    {
        main_renderer.poll_events();

        // Logic here.

        main_renderer.render();
    }

    return 0;
}
