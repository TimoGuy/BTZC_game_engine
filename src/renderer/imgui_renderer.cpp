#include "imgui_renderer.h"

#include "../btzc_game_engine.h"
#include "../game_object/game_object.h"
#include "camera.h"
#include "debug_render_job.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "logger.h"
#include <fstream>


void BT::ImGui_renderer::set_selected_game_obj(Game_object* game_obj)
{
    m_game_obj_pool->set_selected_game_obj(game_obj);
}

void BT::ImGui_renderer::render_imgui()
{
    // @NOCHECKIN: @TEMP
    static bool s_show_game_view{ true };
    static bool s_show_scene_hierarchy{ true };
    static bool s_show_camera_props{ true };
    static bool s_show_console{ true };
    static bool s_show_gameobj_palette{ true };
    static bool s_show_demo_window{ false };
    static ImGuiIO& io = ImGui::GetIO();

    // Main menu bar.
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Menu"))
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

    // Scene hierarchy.
    if (s_show_scene_hierarchy)
    {
        m_game_obj_pool->render_imgui_scene_hierarchy();
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
            if (ImGui::Button("Clear##console"))
            {
                logger::clear_log_entries();
            }

            ImGui::SameLine();

            static bool s_console_auto_scroll{ true };
            ImGui::Checkbox("Auto-scroll##console", &s_console_auto_scroll);

            ImGui::Separator();

            // // Reserve enough left-over height for 1 separator + 1 input text.
            // float_t const footer_height_to_reserve{ ImGui::GetStyle().ItemSpacing.y
            //                                         + ImGui::GetFrameHeightWithSpacing() };
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
