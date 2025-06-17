#include "imgui_renderer.h"

#include "../game_object/game_object.h"
#include "camera.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "imgui_internal.h"


void BT::ImGui_renderer::render_imgui()
{
    // @NOCHECKIN: @TEMP
    static bool s_show_game_view{ true };
    static bool s_show_scene_hierarchy{ true };
    static bool s_show_camera_props{ true };
    static bool s_show_console{ true };
    static bool s_show_gameobj_palette{ true };
    static bool show_demo_window = true;
    static ImGuiIO& io = ImGui::GetIO();

    // Main menu bar.
    if (ImGui::BeginMainMenuBar())
    {
        if (ImGui::BeginMenu("Menu"))
        {
            if (ImGui::MenuItem("New")) {}
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

                    static int32_t s_gizmo_trans_space_selection{ 1 };
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
            ImGui::Text("@TODO: Implement");
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

    // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
    if (show_demo_window)
        ImGui::ShowDemoWindow(&show_demo_window);

    // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.
    {
        static float_t f = 0.0f;
        static int counter = 0;

        ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

        ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

        if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
            counter++;
        ImGui::SameLine();
        ImGui::Text("counter = %d", counter);

        ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
    }
    ImGui::End();
}
