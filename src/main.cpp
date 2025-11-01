#include "animation_frame_action_tool/runtime_data.h"
#include "btzc_game_engine.h"
#include "btglm.h"
#include "game_system_logic/system/imgui_render_transform_hierarchy_window.h"
#include "renderer/camera.h"
#include "game_object/component_registry.h"
#include "game_object/system/concrete_systems.h"
#include "game_object/game_object.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/component/component_registry.h"
#include "game_system_logic/system/process_physics_object_lifetime.h"
#include "game_system_logic/system/process_render_object_lifetime.h"
#include "game_system_logic/system/propagate_changed_transforms.h"
#include "game_system_logic/system/write_render_transforms.h"
#include "game_system_logic/world/scene_loader.h"
#include "hitbox_interactor/hitcapsule.h"
#include "input_handler/input_handler.h"
#include "Jolt/Jolt.h"  // @DEBUG
#include "Jolt/Math/Real.h"  // @DEBUG
#include "Jolt/Math/Quat.h"  // @DEBUG
#include "btlogger.h"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "physics_engine/raycast_helper.h"
#include "refactor_to_entt.h"
#include "renderer/animator_template.h"
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
#include "service_finder/service_finder.h"
#include "timer/timer.h"
#include "timer/watchdog_timer.h"
#include "uuid/uuid.h"
#include <cstdint>
#include <memory>

using std::make_unique;
using std::unique_ptr;


int32_t main()
{
    BT::Watchdog_timer main_watchdog;

#if BTZC_REFACTOR_TO_ENTT
    /// Super duper dummy erasure thing @INCOMPLETE
    #define INVOKE_SYSTEM(_)
#else
    // ---- Initialization of entity component system --------------------------------------------------------------
    BT::component_system::Registry main_component_registry;
    main_component_registry.register_all_components();

    BT::component_system::system::System__dev__anim_editor_tool_state_agent main_system_sdaetsa;
    BT::component_system::system::System_animator_driven_hitcapsule_set main_system_sadhs;
    BT::component_system::system::System_apply_phys_xform_to_rend_obj main_system_sapxtro;
    BT::component_system::system::System_player_character_movement main_system_spcm;

    #define INVOKE_SYSTEM(_sys_type)                                                               \
        BT::service_finder::find_service<BT::component_system::system::_sys_type>().invoke_system()
    // ---------------------------------------------------------------------------------------------
#endif  // !BTZC_REFACTOR_TO_ENTT

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
    // "models": [
    //     "SlimeGirl": {
    //         "type": "obj",
    //         "material": "color_material"
    //     },
    //     ...
    // ]
    BT::Model_bank::emplace_model(
        "unit_box",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "unit_box.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "box_0.5_2",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "box_0.5_2.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "xz_grid",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "stupid_polygon_grid.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "player_model_0.5_2",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "player_model_0.5_2.obj",
                               "color_material"));
    BT::Model_bank::emplace_model(
        "probuilder_example",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "probuilder_example.obj",
                               "textured_material"));
    BT::Model_bank::emplace_model(
        "simple_combat_char",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "simple_combat_char.glb",
                               "textured_material"));
    BT::Model_bank::emplace_model(  // @NOCHECKIN: REMOVE THIS MODEL ONCE DONE TESTING SKINNING (and remove from `s_scene_as_json`).
        "SlimeGirl",
        make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "SlimeGirl.glb",
                               "textured_material"));
    // BT::Model_bank::emplace_model(  // @NOCHECKIN: REMOVE THIS MODEL ONCE DONE TESTING SKINNING (and remove from `s_scene_as_json`).
    //     "test_gltf",
    //     make_unique<BT::Model>(BTZC_GAME_ENGINE_ASSET_MODEL_PATH "Leever.glb",
    //                            "textured_material"));

    // Animator templates.
    BT::Animator_template_bank main_anim_template_bank;

    // Animation frame action runtime data.
    // "anim_frame_action_runtime_datas": [
    //     "SlimeGirl",
    //     ...
    // ]
    std::vector<std::string> const afa_names{
        "simple_combat_char",
        "SlimeGirl",
    };
    for (auto const& afa_name : afa_names)
    {
        BT::anim_frame_action::Bank::emplace(
            afa_name + ".btafa",  // @NOTE: Key must match the file name!!!!! This is very very important.  -Thea 2025/08/30
            BT::anim_frame_action::Runtime_data_controls(
                BTZC_GAME_ENGINE_ASSET_ANIM_FRAME_ACTIONS_PATH + afa_name + ".btafa"));
    }

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

    // Hitcapsule solver.
    BT::Hitcapsule_group_overlap_solver hitcapsule_solver;

#if BTZC_REFACTOR_TO_ENTT
    // Entity container.
    BT::component::register_all_components();
    BT::Entity_container entity_container;
#else
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
#endif  // !BTZC_REFACTOR_TO_ENTT

#if !BTZC_REFACTOR_TO_ENTT
    // Setup scene serialization IO helper.
    BT::scene_serialization_io_helper::set_load_scene_callbacks(
        [&]() {
            return new BT::Game_object(main_input_handler,
                                       main_physics_engine,
                                       main_renderer,
                                       game_object_pool);
        },
        [&](BT::Game_object* game_obj) {
            game_object_pool.emplace(unique_ptr<BT::Game_object>{ game_obj });
        },
        [&](BT::UUID follow_obj) {
            main_renderer.get_camera_obj()->set_follow_object(follow_obj);
        });
#endif  // !BTZC_REFACTOR_TO_ENTT

    // Load default scene.
    BT::world::Scene_loader main_scene_loader;

#if 0
    class Scene_switching_cache
    {   // Simple class for holding scene switching logic.
    public:     Scene_switching_cache(BT::Game_object_pool& game_object_pool)
                    : m_game_object_pool{ game_object_pool }
                {
                }

                void request_new_scene(std::string const& scene_name)
                {
                    m_scene_name = scene_name;
                    m_new_request = true;
                }

                void process_scene_load_request()
                {
                    if (!m_new_request)
                        return;

                    BT::Timer perf_timer;
                    perf_timer.start_timer();

#if BTZC_REFACTOR_TO_ENTT
                    assert(false);  // @TODO implement.
#else
                    // Unload whole scene.
                    auto const all_game_objs{ m_game_object_pool.get_all_as_list_no_lock() };
                    for (auto game_obj : all_game_objs)
                    {
                        m_game_object_pool.remove(game_obj->get_uuid());
                    }

                    // Load new scene.
                    BT::scene_serialization_io_helper::load_scene_from_disk(m_scene_name);
#endif  // !BTZC_REFACTOR_TO_ENTT

                    BT::logger::printef(BT::logger::TRACE,
                                        "Switch to scene \"%s\" from disk finished in %.3fms.",
                                        m_scene_name.c_str(),
                                        perf_timer.calc_delta_time() * 1000.0f);

                    m_new_request = false;
                }

    private:    std::string m_scene_name;
                bool m_new_request;

                BT::Game_object_pool& m_game_object_pool;
    };
#if !BTZC_REFACTOR_TO_ENTT
    Scene_switching_cache main_scene_switcher{ game_object_pool };
    main_scene_switcher.request_new_scene("_dev_sample_scene.btscene");
    main_scene_switcher.process_scene_load_request();
#endif  // !BTZC_REFACTOR_TO_ENTT
#endif  // 0

    // Setup imgui renderer.
#if !BTZC_REFACTOR_TO_ENTT
    main_renderer_imgui_renderer.set_game_obj_pool_ref(&game_object_pool);
#endif  // !BTZC_REFACTOR_TO_ENTT
    main_renderer_imgui_renderer.set_camera_ref(main_renderer.get_camera_obj());
    main_renderer_imgui_renderer.set_renderer_ref(&main_renderer);
    main_renderer_imgui_renderer.set_input_handler_ref(&main_input_handler);
#if !BTZC_REFACTOR_TO_ENTT
    main_renderer_imgui_renderer.set_switch_scene_callback(
        [&main_scene_switcher](std::string const& new_scene_name) {
            main_scene_switcher.request_new_scene(new_scene_name);
        });
#endif  // !BTZC_REFACTOR_TO_ENTT

    // Timer.
    BT::Timer main_timer;
    main_timer.start_timer();

    // Main loop.
    bool first_iter{ true };
    while (!main_renderer.get_requesting_close())
    {
        BT::logger::notify_start_new_mainloop_iteration();
        main_watchdog.pet();
        main_renderer.poll_events();

        float_t delta_time =
            main_physics_engine.limit_delta_time(
                main_timer.calc_delta_time());

        #if !BTZC_REFACTOR_TO_ENTT
        auto const all_game_objs{ game_object_pool.checkout_all_as_list() };
        #endif  // !BTZC_REFACTOR_TO_ENTT

        main_physics_engine.accumulate_delta_time(delta_time);
        while (main_physics_engine.calc_wants_to_tick())  // @TODO: Change the `wants_to_tick()` to something that's not the physics engine. Perhaps a simulation manager or something???  -Thea 2025/10/31
        {   // Run all pre-physics systems.
            #if !BTZC_REFACTOR_TO_ENTT
            for (auto game_obj : all_game_objs)
            {
                game_obj->run_pre_physics_scripts(main_physics_engine.k_simulation_delta_time);
            }
            #endif  // !BTZC_REFACTOR_TO_ENTT

            BT::system::process_physics_object_lifetime();

            INVOKE_SYSTEM(System_player_character_movement);
            INVOKE_SYSTEM(System_animator_driven_hitcapsule_set);
            BT::system::propagate_changed_transforms();  // Does there need to be 2 of these system invocations???  -Thea 2025/10/22
            // @REPLY: @TODO: @FIXME: I kinda don't think that there needs to be 2
            // `propagate_changed_transforms()`, since ideally there would be just a conversion of
            // the entity transform to the render object transform. Maybe?  -Thea 2025/10/31

            // Update physics.
            main_physics_engine.update_physics();

            // Update overlap solver.
            hitcapsule_solver.update_overlaps();
        }

        main_physics_engine.calc_interpolation_alpha();
        {   // Run all pre-render systems.
            #if !BTZC_REFACTOR_TO_ENTT
            for (auto game_obj : all_game_objs)
            {
                game_obj->run_pre_render_scripts(delta_time);
            }
            #endif  // !BTZC_REFACTOR_TO_ENTT

            BT::system::process_render_object_lifetime();
            BT::system::write_render_transforms();
            BT::system::update_selected_entity_debug_render_transform();

            INVOKE_SYSTEM(System__dev__anim_editor_tool_state_agent);
            INVOKE_SYSTEM(System_apply_phys_xform_to_rend_obj);
            BT::system::propagate_changed_transforms();  // Does there need to be 2 of these system invocations???  -Thea 2025/10/22

            main_renderer.render(delta_time, [&]() {
                // Render selected game obj.
                #if !BTZC_REFACTOR_TO_ENTT
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
                #endif  // !BTZC_REFACTOR_TO_ENTT
            });
        }

        #if !BTZC_REFACTOR_TO_ENTT
        for (auto game_obj : all_game_objs)
        {   // Updates game obj and inner parts if any changes are detected.
            game_obj->process_change_requests();
        }

        game_object_pool.return_list(std::move(all_game_objs));
        #endif  // !BTZC_REFACTOR_TO_ENTT

#if BTZC_REFACTOR_TO_ENTT
        // Tick scene loader.
        main_scene_loader.process_scene_loading_requests();
#else
        // Tick level loading.
        main_scene_switcher.process_scene_load_request();
#endif  // !BTZC_REFACTOR_TO_ENTT

        if (first_iter)
        {   // Turn off logging to the console.
            BT::logger::printe(BT::logger::TRACE, "Set logger to not print to console.");
            BT::logger::set_logging_print_mask(BT::logger::NONE);

            first_iter = false;
        }
    }

    // Start showing console logs again for cleanup now that window is gonna disappear.
    BT::logger::set_logging_print_mask(BT::logger::ALL);
    BT::logger::printe(BT::logger::TRACE, "Set logger to print to console.");

    return 0;
}
