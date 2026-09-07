#include "audio_engine/audio_engine.h"
#include "btdatecheck.h"
#include "btuuid.h"
#include "btzc_game_engine.h"
#include "btglm.h"
#include "game_system_logic/component/component_registry.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/system/animator_driven_hitcapsule_sets_update.h"
#include "game_system_logic/system/character_broadcast_attack_msg_to_enemies.h"
#include "game_system_logic/system/cpu_character_enemy_detection.h"
#include "game_system_logic/system/cpu_character_world_space_input.h"
#include "game_system_logic/system/follow_camera_position_update.h"
#include "game_system_logic/system/hitcapsule_attack_processing.h"
#include "game_system_logic/system/imgui_render_transform_hierarchy_window.h"
#include "game_system_logic/system/input_controlled_character_movement.h"
#include "game_system_logic/system/player_character_lock_onto_target.h"
#include "game_system_logic/system/player_character_world_space_input.h"
#include "game_system_logic/system/process_physics_object_lifetime.h"
#include "game_system_logic/system/propagate_changed_transforms.h"
#include "game_system_logic/system/rail_line_editor_update.h"
#include "game_system_logic/system/rail_line_rider_update.h"
#include "game_system_logic/system/tick_sim_char_mvt_animator.h"
#include "game_system_logic/system/write_entity_transforms_from_physics.h"
#include "game_system_logic/system/write_render_transforms.h"
#include "game_system_logic/world/scene_loader.h"
#include "game_system_logic/world/world_properties.h"
#include "btlogger.h"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "physics_engine/raycast_helper.h"
#include "settings/settings.h"
#include "timer/timer.h"
#include "timer/watchdog_timer.h"
#include "txp_renderer_public.h"

#include <cstdint>



#define IMPLEMENT_THIS 0


int32_t main()
{
    BT::initialize_app_settings_from_file_or_fallback_to_defaults();
    BT::App_settings const& app_settings{ BT::get_app_settings_read_handle() };

    BT::Watchdog_timer main_watchdog;

    // Setup ECS system.
    BT::component::register_all_components();
    BT::Entity_container entity_container;

    // Setup world properties.
    BT::world::World_properties_container world_properties;
    {
        auto& wprops{ world_properties.get_data_handle() };
        wprops.is_simulation_running = false;
    }
    bool dev_is_afa_editor_open{ false };

    // Load default scene.
    std::string current_scene{ "_dev_sample_scene.btscene" };
    BT::world::Scene_loader main_scene_loader;
    main_scene_loader.load_scene_additive(current_scene);

    // Setup renderer.
    TXP::Input::Input_handler input_handler;
    TXP::UI_state ui_state;

    TXP::Renderer main_renderer{
        entity_container.get_ecs_registry(),
        "No Train No Game",
        BTZC_GAME_ENGINE_ASSET_TEXTURE_PATH,
        BTZC_GAME_ENGINE_ASSET_SHADER_PATH,
        BTZC_GAME_ENGINE_ASSET_MODEL_PATH,
        BTZC_GAME_ENGINE_ASSET_UI_PATH,
        BTZC_GAME_ENGINE_ASSET_ANIM_FRAME_ACTIONS_PATH,
        BTZC_GAME_ENGINE_ASSET_ANIMATOR_TEMPLATES_PATH,
        [&world_properties, &main_scene_loader, &current_scene](bool flag) {
            world_properties.get_data_handle().is_simulation_running = flag;

            if (!flag)
            {
                main_scene_loader.unload_all_scenes();
                main_scene_loader.load_scene_additive(current_scene);
            }
        },
        [&world_properties]() {
            auto& wph{ world_properties.get_data_handle() };
            return wph.is_simulation_running;
        },
        [&world_properties, &main_scene_loader, &current_scene, &dev_is_afa_editor_open](
            bool flag) {
            assert(!world_properties.get_data_handle().is_simulation_running);

            main_scene_loader.unload_all_scenes();
            main_scene_loader.load_scene_additive(flag ? "_dev_animation_editor_view.btscene"
                                                       : current_scene);

            dev_is_afa_editor_open = flag;
        }
    };

    TXP::debug::set_callbacks_and_references(
        entity_container.get_ecs_registry(),
        [&entity_container]() {
            return entity_container.create_entity(BT::UUID_helper::generate_uuid());
        },
        [&entity_container](entt::entity ent) {
            entity_container.destroy_entity(entity_container.find_entity_uuid(ent));
        });

    main_renderer.add_texture("test_ktx_tex", ".ktx2");
    main_renderer.add_material("default_mat", "basic_diffuse", { { "texture0", "test_ktx_tex" } });
    main_renderer.add_material("ProBuilderDefault",
                               "basic_diffuse",
                               { { "texture0", "test_ktx_tex" } });
    main_renderer.add_material("__gradient_mat",
                               "gradient",
                               { { "image", "__hdr_draw_image_color" } });
    main_renderer.add_material_palette("default_material_palette", { "default_mat" });
    main_renderer.add_model("unit_box", ".wobj", false, false);
    main_renderer.add_model("probuilder_example", ".wobj", false, false);
    main_renderer.add_model("simple_combat_char", ".glb", true, true);
    main_renderer.add_model("rail_line_editor_gizmo", ".wobj", false, false);
    main_renderer.add_model("rails", ".wobj", false, false);

    main_renderer.build();

    BT::get_app_settings_read_handle().send_settings_to_renderer(main_renderer);

    main_renderer.set_imgui_build_contents_callback([]() {
        BT::system::imgui_render_transform_hierarchy_window(false);
    });

    // Setup physics engine.
    BT::Physics_engine main_physics_engine;

    BT::Raycast_helper::set_physics_engine(main_physics_engine);

    // Setup audio engine.
    BT::audio::initialize();

    // Timer.
    BT::Timer main_timer;
    main_timer.start_timer();
    float_t time_scale{ 1 };

    // Iteration types for main loop.
    enum class Iteration_type
    {
        FIRST_RUNNING_ITERATION,
        RUNNING_ITERATION,
        TEARDOWN_ITERATION,
        EXIT_LOOP,
    };
    BT_TRACE("==== ENTERING MAIN LOOP (FIRST RUNNING ITERATION) ==============");
    Iteration_type iter_type{ Iteration_type::FIRST_RUNNING_ITERATION };

    // Main loop.
    while (iter_type != Iteration_type::EXIT_LOOP)
    {
        BT::logger::notify_start_new_mainloop_iteration();
        main_watchdog.pet();
        main_renderer.poll_input_events();

        {   // Change time scale.
            static bool s_prev_ts_decr_pressed{ false };
            static bool s_prev_ts_incr_pressed{ false };

            bool ts_decr_pressed{
                input_handler.get_keyboard_key_state(BT_KEY_LEFT_BRACKET).pressed
            };
            bool ts_incr_pressed{
                input_handler.get_keyboard_key_state(BT_KEY_RIGHT_BRACKET).pressed
            };

            bool changed{ false };
            if (s_prev_ts_decr_pressed != ts_decr_pressed)
            {
                time_scale *= 0.5f;
                changed = true;
            }
            if (s_prev_ts_incr_pressed != ts_incr_pressed)
            {
                time_scale *= 2;
                changed = true;
            }

            if (changed)
                BT_TRACEF("Timescale changed to: %.3f", time_scale);

            s_prev_ts_decr_pressed = ts_decr_pressed;
            s_prev_ts_incr_pressed = ts_incr_pressed;
        }

        float_t delta_time =
            main_physics_engine.limit_delta_time(main_timer.calc_delta_time() * time_scale);

        // @NOCHECKIN: @DEBUG: Fun little sfx for audio engine.
        if (iter_type == Iteration_type::FIRST_RUNNING_ITERATION)
        {
            auto snd_key{ BT::audio::mark_snd_required("test_sfx_0.ogg", false, false, false) };
            BT::audio::play_sound(snd_key, BT::audio::volume_to_db(0.25f));
        }

        // Simulation loop.
        main_physics_engine.accumulate_delta_time(delta_time);
        while (main_physics_engine.calc_wants_to_tick() ||  // @TODO: Change the `wants_to_tick()` to something that's not the physics engine. Perhaps a simulation manager or something???  -Thea 2025/10/31
               iter_type == Iteration_type::TEARDOWN_ITERATION)  // Force one iteration if teardown.
        {   // Performance measure.
            BT::Timer perf_timer;
            perf_timer.start_timer();

            // Pre-physics.
            BT::system::process_physics_object_lifetime();

            TXP::Renderer::advance_afa_sim_timer(main_physics_engine.k_simulation_delta_time);
            BT::system::tick_sim_char_mvt_animator();

            BT::system::rail_line_rider_update();

            BT::system::character_broadcast_attack_msg_to_enemies();
            BT::system::cpu_character_enemy_detection();
            BT::system::cpu_character_world_space_input();
            BT::system::player_character_world_space_input();
            BT::system::input_controlled_character_movement();

            // Physics calculations.
            main_physics_engine.update_physics();

            // Post-physics.
            BT::system::write_entity_transforms_from_physics();
            BT::system::propagate_changed_transforms();

            BT::system::player_character_lock_onto_target();

            BT::system::animator_driven_hitcapsule_sets_update();
            BT::system::hitcapsule_attack_processing(BT::Physics_engine::k_simulation_delta_time);

            // Audio tick.
            BT::audio::update();

            // Performance measure.
            main_renderer.report_performance_time(TXP::PERF_TIME_TYPE_SIMULATION_LOOP,
                                                  perf_timer.calc_delta_time());

            // Only run once if teardown iteration.
            if (iter_type == Iteration_type::TEARDOWN_ITERATION)
                break;
        }

        // Render loop.
        main_physics_engine.calc_interpolation_alpha();
        {   // Performance measure.
            BT::Timer perf_timer;
            perf_timer.start_timer();

            // Run all pre-render systems.
            BT::system::rail_line_editor_update();

            BT::system::write_render_transforms();
            BT::system::follow_camera_position_update();
#if IMPLEMENT_THIS
            BT::system::update_selected_entity_debug_render_transform();
#endif // IMPLEMENT_THIS

            if (iter_type < Iteration_type::TEARDOWN_ITERATION)
            {
                main_renderer.set_allow_deformed_render_models(
                    world_properties.get_data_handle().is_simulation_running ||
                    dev_is_afa_editor_open);

                main_renderer.render_one_frame(delta_time, &ui_state);
            }

            // Performance measure.
            main_renderer.report_performance_time(TXP::PERF_TIME_TYPE_RENDERER_LOOP,
                                                  perf_timer.calc_delta_time());
        }

        // Switch iteration type.
        switch (iter_type)
        {
        case Iteration_type::FIRST_RUNNING_ITERATION:
            // Turn off logging to the console (except for errors and warnings).
            BT_TRACE("Set logger to not print to console (except for errors and warnings).");
            BT::logger::set_logging_print_mask(  // @TODO: @FIXME: Make bitmask support better. This sucks ass.  -Thea 2025/11/23
                (BT::logger::Log_type)((uint32_t)BT::logger::ERROR | (uint32_t)BT::logger::WARN));

            BT_TRACE("==== ENTERING RUNNING ==========================================");
            iter_type = Iteration_type::RUNNING_ITERATION;
            break;

        case Iteration_type::RUNNING_ITERATION:
            if (main_renderer.is_requesting_shutdown())
            {   // Enter teardown.
                main_scene_loader.unload_all_scenes();

                BT::logger::set_logging_print_mask(BT::logger::ALL);

                BT_TRACE("==== ENTERING TEARDOWN =========================================");
                iter_type = Iteration_type::TEARDOWN_ITERATION;
            }
            break;

        case Iteration_type::TEARDOWN_ITERATION:
            BT_TRACE("==== EXITING MAIN LOOP =========================================");
            iter_type = Iteration_type::EXIT_LOOP;
            break;

        case Iteration_type::EXIT_LOOP:
            // How did you get here? The loop should've exited.
            assert(false);
            break;
        }

        // Tick scene loader.
        main_scene_loader.process_scene_loading_requests();
    }

    // Write final state of settings file.
    BT::get_app_settings_write_handle().load_settings_from_renderer(main_renderer);
    BT::save_app_settings_to_disk();

    // Show stats prior to cleanup.
    BT_TRACEF("Post-teardown statistics:\n"
              "  Num scenes                        : %i\n"
              "  Num entities                      : %i\n"
              "  Num ECS entities                  : %i\n"
              "  Num physics objects               : %i\n"
#if IMPLEMENT_THIS
              "  Num render objects                : %i\n"
              "  Num hitcapsule grp sets in solver : %i\n"
#endif // IMPLEMENT_THIS
              ,
              main_scene_loader.get_num_loaded_scenes(),
              entity_container.get_num_entities(),
              entity_container.get_ecs_registry().view<entt::entity>().size(),
              main_physics_engine.get_num_physics_objects()
#if IMPLEMENT_THIS
              ,
              main_renderer.get_render_object_pool().get_num_render_objects(),
              hitcapsule_solver.get_num_group_sets()
#endif // IMPLEMENT_THIS
    );

    return 0;
}
