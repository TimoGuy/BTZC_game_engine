#include "btzc_game_engine.h"
#include "cglm/cglm.h"
#include "cglm/mat4.h"
#include "renderer/camera.h"
#include "game_object/game_object.h"
#include "game_object/scripts/scripts.h"
#include "input_handler/input_handler.h"
#include "Jolt/Jolt.h"  // @DEBUG
#include "Jolt/Math/Real.h"  // @DEBUG
#include "Jolt/Math/Quat.h"  // @DEBUG
#include "logger/logger.h"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "physics_engine/raycast_helper.h"
#include "renderer/imgui_renderer.h"  // @DEBUG
#include "renderer/material.h"  // @DEBUG
#include "renderer/material_impl_debug_lines.h"  // @DEBUG
#include "renderer/material_impl_debug_picking.h"  // @DEBUG
#include "renderer/material_impl_opaque_color_unlit.h"  // @DEBUG
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
#include "uuid/uuid.h"
#include <cstdint>
#include <memory>
#include <fstream>  // @DEBUG

using std::make_unique;
using std::unique_ptr;


int32_t main()
{
    BT::Watchdog_timer main_watchdog;

    BT::Input_handler main_input_handler;
    BT::ImGui_renderer main_renderer_imgui_renderer;
    BT::Renderer main_renderer{ main_input_handler,
                                main_renderer_imgui_renderer,
                                "No Train No Game" };
    BT::Physics_engine main_physics_engine;

    BT::Raycast_helper::set_physics_engine(main_physics_engine);
    BT::Material_bank::set_camera_read_ifc(&main_renderer);

    // Shaders.
    BT::Shader_bank::emplace_shader(
        "color_unlit",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_unlit.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_unlit.frag"));
    BT::Shader_bank::emplace_shader(
        "color_unlit_lines",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_unlit_lines.vert",
                                BTZC_GAME_ENGINE_ASSET_SHADER_PATH "color_unlit_lines.frag"));
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
    BT::Shader_bank::emplace_shader(
        "skinned_mesh_compute",
        make_unique<BT::Shader>(BTZC_GAME_ENGINE_ASSET_SHADER_PATH "skinned_mesh.comp"));

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
        "debug_physics_wireframe_fore_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_color_unlit(vec3{ 0.2f, 0.95f, 0.3f },
                                                BT::k_depth_test_mode_front)));
    BT::Material_bank::emplace_material(
        "debug_physics_wireframe_back_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_color_unlit(vec3{ 0.1f, 0.475f, 0.15f },
                                                BT::k_depth_test_mode_back)));
    BT::Material_bank::emplace_material(
        "debug_selected_wireframe_fore_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_color_unlit(vec3{ 0.95f, 0.3f, 0.95f },
                                                BT::k_depth_test_mode_front)));
    BT::Material_bank::emplace_material(
        "debug_selected_wireframe_back_material",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_opaque_color_unlit(vec3{ 0.475f, 0.15f, 0.475f },
                                                BT::k_depth_test_mode_back)));
    BT::Material_bank::emplace_material(  // @INCOMPLETE @TODO
        "debug_lines_fore_material",
        unique_ptr<BT::Material_ifc>(new BT::Material_debug_lines(true)));
    BT::Material_bank::emplace_material(
        "debug_lines_back_material",
        unique_ptr<BT::Material_ifc>(new BT::Material_debug_lines(false)));
    BT::Material_bank::emplace_material(
        "debug_picking_material",
        unique_ptr<BT::Material_ifc>(new BT::Material_debug_picking()));
    BT::Material_bank::emplace_material(
        "post_process",
        unique_ptr<BT::Material_ifc>(
            new BT::Material_impl_post_process(1.0f)));

    // Models.
    BT::Model_bank::emplace_model(
        "unit_box",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "unit_box.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "box_0.5_2",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "box_0.5_2.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "player_model_0.5_2",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "player_model_0.5_2.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "probuilder_example",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "probuilder_example.obj",
                               "textured_material"));
    BT::Model_bank::emplace_model(  // @NOCHECKIN: REMOVE THIS MODEL ONCE DONE TESTING SKINNING (and remove from `s_scene_as_json`).
        "test_gltf",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "SlimeGirl.glb",
                               "textured_material"));

    // POPULATE TEST LEVEL (@TODO: Once level loading is implemented, replace this with it)
    // Physics objects.
    // auto player_char_phys_obj{
    //     BT::Physics_object::create_character_controller(main_physics_engine,
    //                                                     true,
    //                                                     0.5f,
    //                                                     2.0f,
    //                                                     1.0f,
    //                                                     { JPH::RVec3(0.0f, 5.1f, 0.0f),
    //                                                       JPH::Quat::sIdentity() }) };
    // player_char_phys_obj->assign_generated_uuid();
    // auto player_char_phys_obj_key =
    //     main_physics_engine.emplace_physics_object(std::move(player_char_phys_obj));

    // auto static_level_terrain_phys_obj{
    //     BT::Physics_object::create_triangle_mesh(main_physics_engine,
    //                                              false,
    //                                              BT::Model_bank::get_model("probuilder_example"),
    //                                              JPH::EMotionType::Static,
    //                                              { JPH::RVec3(0.0f, 1.0f, 0.0f),
    //                                                JPH::Quat::sIdentity() }) };
    // static_level_terrain_phys_obj->assign_generated_uuid();
    // main_physics_engine.emplace_physics_object(std::move(static_level_terrain_phys_obj));

    // Render objects.
    auto& render_object_pool{ main_renderer.get_render_object_pool() };

    // BT::Render_object player_char_rend_obj{
    //     BT::Model_bank::get_model("box_0.5_2"),
    //     BT::Render_layer::RENDER_LAYER_DEFAULT,
    //     GLM_MAT4_IDENTITY,
    //     player_char_phys_obj_key };
    // player_char_rend_obj.assign_generated_uuid();
    // auto player_char_rend_obj_key = render_object_pool.emplace(std::move(player_char_rend_obj));

    // BT::Render_object static_level_terrain_rend_obj{
    //     BT::Model_bank::get_model("probuilder_example"),
    //     BT::Render_layer::RENDER_LAYER_DEFAULT,
    //     GLM_MAT4_IDENTITY };
    // static_level_terrain_rend_obj.assign_generated_uuid();
    // render_object_pool.emplace(std::move(static_level_terrain_rend_obj));

    // Game objects.
    BT::Game_object_pool game_object_pool;
    game_object_pool.set_callback_fn([&]() {
        auto new_game_obj = unique_ptr<BT::Game_object>{
            new BT::Game_object(main_input_handler,
                                main_physics_engine,
                                main_renderer,
                                game_object_pool) };
        new_game_obj->set_name("New Game Object");
        new_game_obj->assign_generated_uuid();
        return new_game_obj;
    });

    main_renderer.get_camera_obj()->set_game_object_pool(game_object_pool);

    // Setup imgui renderer.
    main_renderer_imgui_renderer.set_game_obj_pool_ref(&game_object_pool);
    main_renderer_imgui_renderer.set_camera_ref(main_renderer.get_camera_obj());
    main_renderer_imgui_renderer.set_renderer_ref(&main_renderer);

    static json s_scene_as_json =
        R"({
            "cam_following_game_obj": "aae5ebfb-7c2b-49a8-bcb9-658fdea987af",
            "game_objects": [
                {
                    "children": ["d33f01dc-b0b5-4954-9771-71a1c42e5209"],
                    "guid": "aae5ebfb-7c2b-49a8-bcb9-658fdea987af",
                    "name": "My Gay object",
                    "scripts": [
                        {
                            "script_datas": {
                                "phys_obj_key": "08743552-fff6-49d2-ad15-efbd121ab773",
                                "apply_facing_angle_game_obj_key": "d33f01dc-b0b5-4954-9771-71a1c42e5209"
                            },
                            "script_type": "script_player_character_movement"
                        },
                        {
                            "script_datas": {
                                "game_obj_key": "aae5ebfb-7c2b-49a8-bcb9-658fdea987af"
                            },
                            "script_type": "script_apply_physics_transform_to_render_object"
                        }
                    ],
                    "physics_obj": {
                        "guid": "08743552-fff6-49d2-ad15-efbd121ab773",
                        "type": "character_controller",
                        "interpolate_transform": true,
                        "radius": 0.5,
                        "height": 2.0,
                        "crouch_height": 1.0,
                        "init_transform": [[0.0, 5.1, 0.0],
                                           [0.0, 0.0, 0.0, 1.0]]
                    },
                    "transform": {
                        "global": {
                            "position": [0.0, 0.0, 0.0],
                            "rotation": [0.0, 0.0, 0.0, 1.0],
                            "scale": [1.0, 1.0, 1.0]
                        },
                        "local": {
                            "position": [0.0, 0.0, 0.0],
                            "rotation": [0.0, 0.0, 0.0, 1.0],
                            "scale": [1.0, 1.0, 1.0]
                        }
                    }
                },
                {
                    "children": [],
                    "guid": "aae5ebfb-7c2b-49a8-bcb9-658fdea987ae",
                    "name": "Static ground geometry",
                    "render_obj": {
                        "guid": "fa2a0c0c-cfd4-47a7-b4e7-01dbbd3b3538",
                        "renderable": {
                            "model_name": "probuilder_example",
                            "type": "Model"
                        },
                        "render_layer": 1,
                        "transform": [[1.0, 0.0, 0.0, 0.0],
                                    [0.0, 1.0, 0.0, 0.0],
                                    [0.0, 0.0, 1.0, 0.0],
                                    [0.0, 0.0, 0.0, 1.0]]
                    },
                    "physics_obj": {
                        "guid": "08743552-fff6-49d2-ad15-efbd121ab772",
                        "type": "triangle_mesh",
                        "interpolate_transform": false,
                        "model_name": "probuilder_example",
                        "motion_type": 0,
                        "init_transform": [[0.0, 0.0, 0.0],
                                        [0.0, 0.0, 0.0, 1.0]]
                    },
                    "transform": {
                        "global": {
                            "position": [0.0, 0.0, 0.0],
                            "rotation": [0.0, 0.0, 0.0, 1.0],
                            "scale":    [1.0, 1.0, 1.0]
                        },
                        "local": {
                            "position": [0.0, 0.0, 0.0],
                            "rotation": [0.0, 0.0, 0.0, 1.0],
                            "scale":    [1.0, 1.0, 1.0]
                        }
                    }
                },
                {
                    "children": [],
                    "guid": "d33f01dc-b0b5-4954-9771-71a1c42e5209",
                    "name": "Character repr",
                    "parent": "aae5ebfb-7c2b-49a8-bcb9-658fdea987af",
                    "physics_obj": null,
                    "render_obj": {
                        "guid": "fa2a0c0c-cfd4-47a7-b4e7-01dbbd3b3537",
                        "renderable": {
                            "model_name": "test_gltf",
                            "type": "Deformed_model"
                        },
                        "render_layer": 1
                    },
                    "scripts": [],
                    "transform": {
                        "global": {
                            "position": [0.0, 0.0, 0.0],
                            "rotation": [0.0, 0.0, 0.0, 1.0],
                            "scale":    [1.0, 1.0, 1.0]
                        },
                        "local": {
                            "position": [0.0, -2.980232238769531e-07, 0.0],
                            "rotation": [0.0, 0.0, 0.0, 1.0],
                            "scale":    [1.0, 1.0, 1.0]
                        }
                    }
                }
            ]
        })"_json;

    assert(s_scene_as_json["game_objects"].is_array());
    for (auto& game_obj_node : s_scene_as_json["game_objects"])
    {
        unique_ptr<BT::Game_object> new_game_obj{
            new BT::Game_object(main_input_handler,
                                main_physics_engine,
                                main_renderer,
                                game_object_pool) };
        new_game_obj->scene_serialize(BT::SCENE_SERIAL_MODE_DESERIALIZE, game_obj_node);
        game_object_pool.emplace(std::move(new_game_obj));
    }

    assert(s_scene_as_json["cam_following_game_obj"].is_string());
    main_renderer.get_camera_obj()->set_follow_object(
        BT::UUID_helper::to_UUID(s_scene_as_json["cam_following_game_obj"]));

    // {   // @TODO: @NOCHECKIN: @DEBUG
    //     // Serialize scene.
    //     json root = {};
    //     size_t game_obj_idx{ 0 };
    //     auto const game_objs{ game_object_pool.checkout_all_as_list() };
    //     for (auto game_obj : game_objs)
    //     {
    //         game_obj->scene_serialize(BT::SCENE_SERIAL_MODE_SERIALIZE, root[game_obj_idx++]);
    //     }
    //     game_object_pool.return_list(std::move(game_objs));

    //     // Save to disk.
    //     std::ofstream f{ BTZC_GAME_ENGINE_ASSET_SCENE_PATH "sumthin_cumming_outta_me.btscene" };
    //     f << root.dump(4);
    // }

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

            main_renderer.render(delta_time, [&]() {
                // Render selected game obj.
                auto selected_game_obj{ game_object_pool.get_selected_game_obj() };
                if (!selected_game_obj.is_nil())
                {
                    auto game_obj = game_object_pool.get_one_no_lock(selected_game_obj);
                    auto rend_obj_key{ game_obj->get_rend_obj_key() };
                    if (!rend_obj_key.is_nil())
                    {
                        auto rend_obj =
                            main_renderer.get_render_object_pool()
                                .checkout_render_obj_by_key({ rend_obj_key })[0];

                        static auto s_material_fore{
                            BT::Material_bank::get_material("debug_selected_wireframe_fore_material") };
                        static auto s_material_back{
                            BT::Material_bank::get_material("debug_selected_wireframe_back_material") };
                        rend_obj->render(BT::Render_layer::RENDER_LAYER_ALL, s_material_fore);
                        rend_obj->render(BT::Render_layer::RENDER_LAYER_ALL, s_material_back);

                        main_renderer.get_render_object_pool().return_render_objs({ rend_obj });
                    }
                }
            });
        }

        game_object_pool.return_list(std::move(all_game_objs));

        // @TODO: @HERE: Tick level loading.
    }

    return 0;
}
