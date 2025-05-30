#include "btzc_game_engine.h"
#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "renderer/camera.h"
#include "game_object/game_object.h"
#include "game_object/scripts/pre_physics_scripts.h"
#include "game_object/scripts/pre_render_scripts.h"
#include "game_object/scripts/serialization.h"
#include "input_handler/input_handler.h"
#include "Jolt/Jolt.h"  // @DEBUG
#include "Jolt/Math/Real.h"  // @DEBUG
#include "Jolt/Math/Quat.h"
#include "logger/logger.h"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "renderer/material.h"  // @DEBUG
#include "renderer/material_impl_opaque_shaded.h"  // @DEBUG
#include "renderer/material_impl_opaque_texture_shaded.h"  // @DEBUG
#include "renderer/material_impl_post_process.h"  // @DEBUG
#include "renderer/mesh.h"  // @DEBUG
#include "renderer/render_object.h"  // @DEBUG
#include "renderer/renderer.h"
#include "renderer/shader.h"  // @DEBUG
#include "renderer/texture.h"  // @DEBUG
#include "timer/timer.h"
#include <cstdint>
#include <memory>

using std::make_unique;
using std::unique_ptr;


int32_t main()
{
    BT::Input_handler main_input_handler;
    BT::Renderer main_renderer{ main_input_handler,
                                "Untitled Zelda-like Collectathon Game" };
    BT::Physics_engine main_physics_engine;

    BT::Material_bank::set_camera_read_ifc(&main_renderer);

    // Shaders.
    BT::Shader_bank::emplace_shader(
        "color_shaded",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_shaded.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_shaded.frag"));
    BT::Shader_bank::emplace_shader(
        "textured_shaded",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "textured_shaded.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "textured_shaded.frag"));
    BT::Shader_bank::emplace_shader(
        "post_process",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "post_process.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "post_process.frag"));

    // Textures.
    BT::Texture_bank::emplace_texture_2d(
        "default_texture",
        BT::Texture_bank::load_texture_2d_from_file(BTZC_GAME_ENGINE_ASSET_TEXTURE_PATH "grids_1m.jpg",
                                                    3));

    // Materials.
    BT::Material_bank::emplace_material(
        "color_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_shaded(vec3{ 0.5f, 0.225f, 0.3f })));
    BT::Material_bank::emplace_material(
        "textured_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_texture_shaded(BT::Texture_bank::get_texture_2d("default_texture"),
                                                   vec3{ 0.0f, 0.2f, 0.5f },
                                                   vec3{ 0.5f, 0.2f, 0.1f },
                                                   45.0f)));
    BT::Material_bank::emplace_material(
        "post_process",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_impl_post_process(1.0f)));

    // Models.
    BT::Model_bank::emplace_model(
        "box_0.5_2",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "box_0.5_2.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "probuilder_example",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "probuilder_example.obj",
                               "textured_material"));

    // POPULATE TEST LEVEL (@TODO: Once level loading is implemented, replace this with it)
    // Physics objects.
    auto player_char_phys_obj_key = main_physics_engine.emplace_physics_object(
        BT::Physics_object::create_character_controller(main_physics_engine,
                                                        true,
                                                        0.5f,
                                                        2.0f,
                                                        1.0f,
                                                        { JPH::RVec3(0.0f, 5.1f, 0.0f),
                                                          JPH::Quat::sIdentity() }));
    main_physics_engine.emplace_physics_object(
        BT::Physics_object::create_triangle_mesh(main_physics_engine,
                                                           false,
                                                           BT::Model_bank::get_model("probuilder_example"),
                                                           JPH::EMotionType::Static,
                                                           { JPH::RVec3(0.0f, 1.0f, 0.0f),
                                                             JPH::Quat::sIdentity() }));

    // Render objects.
    auto& render_object_pool{ main_renderer.get_render_object_pool() };
    auto player_char_rend_obj_key = render_object_pool.emplace(BT::Render_object{
        *BT::Model_bank::get_model("box_0.5_2"),
        BT::Render_layer::RENDER_LAYER_DEFAULT,
        GLM_MAT4_IDENTITY,
        player_char_phys_obj_key });
    render_object_pool.emplace(BT::Render_object{
        *BT::Model_bank::get_model("probuilder_example"),
        BT::Render_layer::RENDER_LAYER_DEFAULT,
        GLM_MAT4_IDENTITY });

    // Game objects.
    BT::Game_object_pool game_object_pool;

    vector<BT::Pre_physics_script::Script_type> phys_scripts;
    vector<uint64_t> phys_scripts_datas;

    phys_scripts.emplace_back(BT::Pre_physics_script::SCRIPT_TYPE_player_character_movement);
    BT::Serial::push_u64(phys_scripts_datas, player_char_phys_obj_key);
    BT::Serial::push_void_ptr(phys_scripts_datas, &main_input_handler);
    BT::Serial::push_void_ptr(phys_scripts_datas, main_renderer.get_camera_obj());

    game_object_pool.emplace(unique_ptr<BT::Game_object>(
        new BT::Game_object(main_physics_engine,
                            main_renderer,
                            std::move(phys_scripts),
                            std::move(phys_scripts_datas),
                            { BT::Pre_render_script::SCRIPT_TYPE_apply_physics_transform_to_render_object },
                            { player_char_rend_obj_key, reinterpret_cast<uint64_t>(&main_physics_engine) } )));

    // Camera follow ref.
    main_renderer.get_camera_obj()->set_follow_object(player_char_phys_obj_key);

    // Timer.
    BT::Timer main_timer;
    main_timer.start_timer();

    // Main loop.
    while (!main_renderer.get_requesting_close())
    {
        BT::logger::notify_start_new_mainloop_iteration();
        main_renderer.poll_events();

        float_t delta_time =
            main_physics_engine.limit_delta_time(
                main_timer.calc_delta_time());

        auto const all_game_objs{ game_object_pool.checkout_all_as_list() };

        main_physics_engine.accumulate_delta_time(delta_time);
        while (main_physics_engine.calc_wants_to_tick())
        {
            // Run all pre-physics scripts.
            for (auto game_obj : all_game_objs)
            {
                game_obj->run_pre_physics_scripts(main_physics_engine.k_simulation_delta_time);
            }

            main_physics_engine.update_physics();
        }

        main_physics_engine.calc_interpolation_alpha();
        {
            // Run all pre-render scripts.
            for (auto game_obj : all_game_objs)
            {
                game_obj->run_pre_render_scripts(delta_time);
            }

            main_renderer.render(delta_time);
        }

        game_object_pool.return_all_as_list(std::move(all_game_objs));

        // @TODO: @HERE: Tick level loading.
    }

    return 0;
}
