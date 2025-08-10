#include "imgui_renderer.h"

#include "../btzc_game_engine.h"
#include "../game_object/game_object.h"
#include "camera.h"
#include "debug_render_job.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "logger.h"
#include <array>
#include <fstream>


void BT::ImGui_renderer::set_selected_game_obj(Game_object* game_obj)
{
    m_game_obj_pool->set_selected_game_obj(game_obj);
}

void BT::ImGui_renderer::render_imgui()
{
    // @NOCHECKIN: @TEMP
    static bool s_show_game_view{ true };
    static bool s_show_camera_props{ true };
    static bool s_show_console{ true };
    static bool s_show_level_select{ true };
    static bool s_show_scene_hierarchy{ true };
    static bool s_show_gameobj_palette{ true };
    static bool s_show_demo_window{ false };
    static ImGuiIO& io = ImGui::GetIO();

    // Context switching.
    enum Editor_context
    {
        LEVEL_EDITOR,
        ANIMATION_FRAME_DATA_EDITOR,
        NUM_EDITOR_CONTEXTS
    };
    static std::array<std::string, NUM_EDITOR_CONTEXTS> const k_editor_context_strs{
        "Level Editor",
        "Animation Frame Data Editor",
    };
    static Editor_context s_current_editor_context{ Editor_context(0) };

    // Main menu bar.
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Menu##main_menu_bar_option"))
        {
            if (ImGui::MenuItem("New")) {}
            if (ImGui::MenuItem("Save"))
            {   // @TODO: @NOCHECKIN: @DEBUG
                // Serialize scene.
                json root = {};
                size_t game_obj_idx{ 0 };
                auto const game_objs{ m_game_obj_pool->get_all_as_list_no_lock() };
                for (auto game_obj : game_objs)
                {
                    game_obj->scene_serialize(BT::SCENE_SERIAL_MODE_SERIALIZE, root[game_obj_idx++]);
                }

                // Save to disk.
                std::ofstream f{ BTZC_GAME_ENGINE_ASSET_SCENE_PATH "sumthin_cumming_outta_me.btscene" };
                f << root.dump(4);
            }

            ImGui::Separator();

            ImGui::MenuItem("Show ImGui demo window", nullptr, &s_show_demo_window);

            if (ImGui::MenuItem("Show debug meshes", nullptr, get_main_debug_mesh_pool().get_visible()))
            {
                get_main_debug_mesh_pool().set_visible(!get_main_debug_mesh_pool().get_visible());
            }

            if (ImGui::MenuItem("Show debug lines", nullptr, get_main_debug_line_pool().get_visible()))
            {
                get_main_debug_line_pool().set_visible(!get_main_debug_line_pool().get_visible());
            }

            ImGui::EndMenu();
        }

        // Context switching menu.
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        for (size_t i = 0; i < NUM_EDITOR_CONTEXTS; i++)
        {
            auto color_btn_default{ ImGui::GetStyleColorVec4(ImGuiCol_Button) };
            auto color_btn_hover{ ImGui::GetStyleColorVec4(ImGuiCol_ButtonHovered) };
            auto color_btn_active{ ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive) };
            {   // Make very translucent for showing they're not the active button.
                color_btn_default.w *= (s_current_editor_context == i ? 1.0f : 0.25f);
                color_btn_hover.w *= (s_current_editor_context == i ? 1.0f : 0.25f);
                color_btn_active.w *= (s_current_editor_context == i ? 1.0f : 0.25f);
            }

            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 12.0f);
            ImGui::PushStyleColor(ImGuiCol_Button, color_btn_default);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, color_btn_hover);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, color_btn_active);
            // ImGui::BeginDisabled(s_current_editor_context == i);
            if (ImGui::Button((k_editor_context_strs[i]
                               + "##main_menu_bar_context_switch").c_str()))
            {   // New editor context.
                s_current_editor_context = Editor_context(i);
            }
            // ImGui::EndDisabled();
            ImGui::PopStyleColor(3);
            ImGui::PopStyleVar();
        }

        ImGui::EndMainMenuBar();
    }

    // Main dockspace.
    ImGui::DockSpaceOverViewport(0,
                                 ImGui::GetMainViewport(),
                                 ImGuiDockNodeFlags_PassthruCentralNode);

    // Game view.
    if (s_show_game_view)
    {
        ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);  // Force game view to stay in main window.
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
        ImGui::Begin("Main viewport", nullptr, ImGuiWindowFlags_NoScrollbar);
        {
            static bool s_on_play_switch_to_player_camera{ true };

            // Force padding.
            ImGui::NewLine();  // @TODO: Doesn't work for vert padding.
            ImGui::SameLine();

            // Game controls.
            ImGui::BeginDisabled();
                ImGui::Button("Play");

                ImGui::SameLine();
                ImGui::Button("Stop");
            ImGui::EndDisabled();

            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 0.5f), "Playing");

            ImGui::SameLine();
            ImGui::Text("%.1f FPS (%.3f ms)", io.Framerate, (1000.0f / io.Framerate));

            static float_t s_toggle_player_cam_btn_width{ 0 };
            static float_t s_player_cam_options_btn_width{ 0 };

            ImGui::SameLine(ImGui::GetWindowWidth()
                                - s_toggle_player_cam_btn_width - 2.0f
                                - s_player_cam_options_btn_width - 8.0f);
            if (ImGui::Button((!m_camera->is_follow_orbit() ?
                                   "Switch to player cam (F1)" :
                                   "Exit player cam (F1)")))
            {
                m_camera->request_follow_orbit();
            }
            s_toggle_player_cam_btn_width = ImGui::GetItemRectSize().x;

            ImGui::SameLine(ImGui::GetWindowWidth()
                                - s_player_cam_options_btn_width - 8.0f);
            if (ImGui::ArrowButton("Arrow_btn_for_switch_to_player_cam", ImGuiDir_Down))
                ImGui::OpenPopup("Popup_menu_for_switch_to_player_cam");
            s_player_cam_options_btn_width = ImGui::GetItemRectSize().x;

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 4, 4 });
                if (ImGui::BeginPopup("Popup_menu_for_switch_to_player_cam"))
                {
                    ImGui::Checkbox("Auto switch to player cam on play.", &s_on_play_switch_to_player_camera);

                    int32_t s_gizmo_trans_space_selection{
                        Game_object::get_imgui_gizmo_trans_space() };
                    if (ImGui::Combo("Gizmo transform space",
                                     &s_gizmo_trans_space_selection,
                                     "World space\0Local space\0"))
                    {
                        Game_object::set_imgui_gizmo_trans_space(s_gizmo_trans_space_selection);
                    }

                    ImGui::EndPopup();
                }
            ImGui::PopStyleVar();

            // Image of game view.
            m_renderer->render_imgui_game_view();

            // Allow ImGuizmo to accept inputs from this window.
            ImGuizmo::SetAlternativeWindow(ImGui::GetCurrentWindow());
        }
        ImGui::End();
        ImGui::PopStyleVar();
    }

    // Camera properties.
    if (s_show_camera_props)
    {
        m_camera->render_imgui();
    }

    // Console.
    if (s_show_console)
    {
        ImGui::Begin("Console");
        {
            bool copy_logs{ false };
            if (ImGui::Button("Copy##console"))
            {
                copy_logs = true;
            }

            ImGui::SameLine();

            if (ImGui::Button("Clear##console"))
            {
                logger::clear_log_entries();
            }

            ImGui::SameLine();

            if (ImGui::Button("DEBUG add test log##console"))
            {
                logger::printe(logger::TRACE, "TEST LOG ENTRY HERE");
            }

            ImGui::SameLine();

            static bool s_console_auto_scroll{ true };
            ImGui::Checkbox("Auto-scroll##console", &s_console_auto_scroll);

            ImGui::Separator();

            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0, 0, 0, 1));
            if (ImGui::BeginChild("console_scrolling_region",
                                  ImVec2(0, 0), //-footer_height_to_reserve),
                                  ImGuiChildFlags_NavFlattened,
                                  ImGuiWindowFlags_HorizontalScrollbar))
            {
                ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing.

                // Print all log entries recorded.
                auto [head_idx, tail_idx] = logger::get_head_and_tail();
                for (auto row_idx = head_idx; row_idx != tail_idx; row_idx++)
                {
                    auto [log_type, log_str] = logger::read_log_entry(row_idx);

                    static auto const s_get_text_color_fn = [](logger::Log_type type) {
                        switch (type)
                        {
                            case logger::TRACE: return ImVec4(1, 1, 1, 1);
                            case logger::WARN:  return ImVec4(0.647, 0.722, 0.180, 1);
                            case logger::ERROR: return ImVec4(0.719, 0.180, 0.180, 1);
                            default: return ImVec4();  // Ignore.
                        }
                    };

                    auto text_color{ s_get_text_color_fn(log_type) };
                    ImGui::PushStyleColor(ImGuiCol_Text, text_color);
                    ImGui::TextUnformatted(log_str);
                    ImGui::PopStyleColor();
                }

                // Scroll to bottom if auto scroll enabled.
                if (s_console_auto_scroll && ImGui::GetScrollY() >= ImGui::GetScrollMaxY())
                {
                    ImGui::SetScrollHereY(1.0f);
                }

                ImGui::PopStyleVar();
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }
        ImGui::End();
    }

    // Level select.
    if (s_show_level_select)
    {
        ImGui::Begin("Level select");
        {
            ImGui::Button("Create new level..");
            ImGui::SameLine();
            ImGui::Button("Refresh list");
            ImGui::SameLine();
            ImGui::Spacing();
            ImGui::Text("Levels:69 Avg:6.9KB Mdn:6.9KB Max:69.4KB");

            ImGui::SeparatorText("Open levels");

            // @TODO: Change to just be 1 level loaded at a time.
            std::vector<std::string> open_levels{ "choo_choo_torial_station.btscene",
                                                  "jump_jump_jungle.btscene" };
            static float_t s_close_btn_width{ 0.0f };
            for (auto& level : open_levels)
            {
                ImGui::PushID(("open_level" + level).c_str());
                ImGui::SetNextItemWidth(-(s_close_btn_width + ImGui::GetFontSize()));
                ImGui::Button("choo_choo_torial_station.btscene");  // Idk what this does. Maybe go to the hierarchy and highlight the level's root node?
                ImGui::SameLine();
                ImGui::Button("X");
                s_close_btn_width = ImGui::GetItemRectSize().x;
                ImGui::PopID();
            }

            ImGui::SeparatorText("Level list");

            // @TODO: Gray out loaded level in this list below.
            ImGui::PushItemWidth(0.0f);
            ImGui::Text("choo_choo_torial_station.btscene");
            ImGui::Text("jump_jump_jungle.btscene");
            ImGui::Text("give_temu_hitler_a_blowjob.btscene");
            ImGui::Text("cluster_lights_test.btscene");
            ImGui::Text("global_illumination_test.btscene");
            ImGui::Text("skeletal_animation_test.btscene");
            ImGui::PopItemWidth();
        }
        ImGui::End();
    }

    // Scene hierarchy.
    if (s_show_scene_hierarchy)
    {
        m_game_obj_pool->render_imgui_scene_hierarchy();
    }

    // Game obj palette.
    if (s_show_gameobj_palette)
    {
        ImGui::Begin("Game obj palette");
        {
            ImGui::Text("@TODO: Implement");
        }
        ImGui::End();
    }

    if (s_show_demo_window)
        ImGui::ShowDemoWindow(&s_show_demo_window);
}
