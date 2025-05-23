#include "btzc_game_engine.h"
#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "input_handler/input_handler.h"
#include "renderer/material.h"  // @DEBUG
#include "renderer/material_impl_opaque_shaded.h"  // @DEBUG
#include "renderer/material_impl_post_process.h"  // @DEBUG
#include "renderer/mesh.h"  // @DEBUG
#include "renderer/render_object.h"  // @DEBUG
#include "renderer/renderer.h"
#include "renderer/shader.h"  // @DEBUG
#include <cstdint>
#include <fmt/base.h>
#include <memory>

using std::make_unique;


int32_t main()
{
    BT::Input_handler main_input_handler;
    BT::Renderer main_renderer{ main_input_handler,
                                "Untitled Zelda-like Collectathon Game" };

    BT::Material_bank::set_camera_read_ifc(&main_renderer);

    // Shaders.
    BT::Shader_bank::emplace_shader(
        "color_shaded",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_shaded.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_shaded.frag"));
    BT::Shader_bank::emplace_shader(
        "post_process",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "post_process.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "post_process.frag"));

    // Materials.
    BT::Material_bank::emplace_material(
        "default_material",
        std::unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_shaded(vec3{ 0.5f, 0.225f, 0.3f })));
    BT::Material_bank::emplace_material(
        "post_process",
        std::unique_ptr<BT::Material_ifc>(
            new BT::Material_impl_post_process(1.0f)));

    // Models.
    BT::Model_bank::emplace_model(
        "cylinder_0.5_2",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "cylinder_0.5_2.obj",
                               "default_material"));
    BT::Model_bank::emplace_model(
        "probuilder_example",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "probuilder_example.obj",
                               "default_material"));

    // Render objects.
    main_renderer.emplace_render_object(BT::Render_object{
        *BT::Model_bank::get_model("cylinder_0.5_2"),
        BT::Render_layer::RENDER_LAYER_DEFAULT,
        GLM_MAT4_IDENTITY });
    main_renderer.emplace_render_object(BT::Render_object{
        *BT::Model_bank::get_model("probuilder_example"),
        BT::Render_layer::RENDER_LAYER_DEFAULT,
        GLM_MAT4_IDENTITY });

    // Main loop.
    while (!main_renderer.get_requesting_close())
    {
        main_renderer.poll_events();

        // @TODO: @HERE: Physics fixed timestep logic
            // @TODO: @HERE: All game objects pre-physics scripts execution.

        // @TODO: @HERE: If gameobjects have both a physics and render object, then calc interpolated (decomposed) transform and make available to render object.
        // @TODO: @HERE: All game objects pre-render scripts execution.

        main_renderer.render();
    }

    return 0;
}
