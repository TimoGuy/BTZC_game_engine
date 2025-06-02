#include "btzc_game_engine.h"
#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "renderer/camera.h"
#include "game_object/game_object.h"
#include "game_object/scripts/scripts.h"
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
#include "scene/scene_serialization_ifc.h"
#include "timer/timer.h"
#include "timer/watchdog_timer.h"
#include "uuid/uuid_ifc.h"
#include <cstdint>
#include <memory>
#include <fstream>  // @DEBUG

using std::make_unique;
using std::unique_ptr;


int32_t main()
{
    BT::Watchdog_timer main_watchdog;

    BT::Input_handler main_input_handler;
    BT::Renderer main_renderer{ main_input_handler,
                                "No Train No Game" };
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
    auto player_char_phys_obj{
        BT::Physics_object::create_character_controller(main_physics_engine,
                                                        true,
                                                        0.5f,
                                                        2.0f,
                                                        1.0f,
                                                        { JPH::RVec3(0.0f, 5.1f, 0.0f),
                                                          JPH::Quat::sIdentity() }) };
    player_char_phys_obj->generate_uuid();
    auto player_char_phys_obj_key =
        main_physics_engine.emplace_physics_object(std::move(player_char_phys_obj));

    auto static_level_terrain_phys_obj{
        BT::Physics_object::create_triangle_mesh(main_physics_engine,
                                                 false,
                                                 BT::Model_bank::get_model("probuilder_example"),
                                                 JPH::EMotionType::Static,
                                                 { JPH::RVec3(0.0f, 1.0f, 0.0f),
                                                   JPH::Quat::sIdentity() }) };
    static_level_terrain_phys_obj->generate_uuid();
    main_physics_engine.emplace_physics_object(std::move(static_level_terrain_phys_obj));

    // Render objects.
    auto& render_object_pool{ main_renderer.get_render_object_pool() };

    BT::Render_object player_char_rend_obj{
        *BT::Model_bank::get_model("box_0.5_2"),
        BT::Render_layer::RENDER_LAYER_DEFAULT,
        GLM_MAT4_IDENTITY,
        player_char_phys_obj_key };
    player_char_rend_obj.generate_uuid();
    auto player_char_rend_obj_key = render_object_pool.emplace(std::move(player_char_rend_obj));

    BT::Render_object static_level_terrain_rend_obj{
        *BT::Model_bank::get_model("probuilder_example"),
        BT::Render_layer::RENDER_LAYER_DEFAULT,
        GLM_MAT4_IDENTITY };
    static_level_terrain_rend_obj.generate_uuid();
    render_object_pool.emplace(std::move(static_level_terrain_rend_obj));

    // Game objects.
    BT::Game_object_pool game_object_pool;

    // vector<unique_ptr<BT::Scripts::Script_ifc>> scripts;

    // json scripts_as_json =
    //     R"([
    //         {
    //             "script_type": "script_player_character_movement",
    //             "script_datas": {
    //                 "phys_obj_key": ""
    //             }
    //         },
    //         {
    //             "script_type": "script_apply_physics_transform_to_render_object",
    //             "script_datas": {
    //                 "rend_obj_key": ""
    //             }
    //         }
    //     ])"_json;
    // scripts_as_json[0]["script_datas"]["phys_obj_key"] = BT::UUID_helper::to_pretty_repr(player_char_phys_obj_key);
    // scripts_as_json[1]["script_datas"]["rend_obj_key"] = BT::UUID_helper::to_pretty_repr(player_char_rend_obj_key);

    // for (auto& script_as_json : scripts_as_json)
    // {
    //     scripts.emplace_back(BT::Scripts::create_script_from_serialized_datas(
    //         &main_input_handler,
    //         &main_physics_engine,
    //         &main_renderer,
    //         script_as_json));
    // }

    unique_ptr<BT::Game_object> player_char_game_obj{
        new BT::Game_object(main_input_handler,
                            main_physics_engine,
                            main_renderer) };
    json game_obj_as_json =
        R"([
            {
                "children": [],
                "guid": "aae5ebfb-7c2b-49a8-bcb9-658fdea987af",
                "name": "My Gay object",
                "scripts": [
                    {
                        "script_datas": {
                            "phys_obj_key": "08743552-fff6-49d2-ad15-efbd121ab773"
                        },
                        "script_type": "script_player_character_movement"
                    },
                    {
                        "script_datas": {
                            "rend_obj_key": "fa2a0c0c-cfd4-47a7-b4e7-01dbbd3b3537"
                        },
                        "script_type": "script_apply_physics_transform_to_render_object"
                    }
                ],
                "render_obj": {
                    "guid": "fa2a0c0c-cfd4-47a7-b4e7-01dbbd3b3537",
                    "model_name": "box_0.5_2",
                    "render_layer": 1,
                    "transform": [[1.0, 0.0, 0.0, 0.0],
                                  [0.0, 1.0, 0.0, 0.0],
                                  [0.0, 0.0, 1.0, 0.0],
                                  [0.0, 0.0, 0.0, 1.0]],
                    "tethered_phys_obj": "08743552-fff6-49d2-ad15-efbd121ab773"
                },
                "physics_obj": {
                    "guid": "08743552-fff6-49d2-ad15-efbd121ab773",
                    "type": "character_controller",
                    "interpolate_transform": true,
                    "radius": 0.5,
                    "height": 2.0,
                    "crouch_height": 1.0,
                    "init_transform": [[0.0, 5.1, 0.0],
                                       [0.0, 0.0, 0.0, 1.0]]
                }
            }
        ])"_json;
    player_char_game_obj->scene_serialize(BT::SCENE_SERIAL_MODE_DESERIALIZE, game_obj_as_json);
    game_object_pool.emplace(std::move(player_char_game_obj));

    {   // @TODO: @NOCHECKIN: @DEBUG
        // Serialize scene.
        json root = {};
        size_t game_obj_idx{ 0 };
        auto const game_objs{ game_object_pool.checkout_all_as_list() };
        for (auto game_obj : game_objs)
        {
            game_obj->scene_serialize(BT::SCENE_SERIAL_MODE_SERIALIZE, root[game_obj_idx++]);
        }
        game_object_pool.return_list(std::move(game_objs));

        // Save to disk.
        std::ofstream f{ BTZC_GAME_ENGINE_ASSET_SCENE_PATH "sumthin_cumming_outta_me.btscene" };
        f << root.dump(4);
    }

    // Camera follow ref.
    main_renderer.get_camera_obj()->set_follow_object(player_char_rend_obj_key);

    // Timer.
    BT::Timer main_timer;
    main_timer.start_timer();

    // Main loop.
    while (!main_renderer.get_requesting_close())
    {
        BT::logger::notify_start_new_mainloop_iteration();
        main_watchdog.pet();
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

        game_object_pool.return_list(std::move(all_game_objs));

        // @TODO: @HERE: Tick level loading.
    }

    return 0;
}
