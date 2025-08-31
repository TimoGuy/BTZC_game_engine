#include "imgui_renderer.h"

#include "../animation_frame_action_tool/editor_state.h"
#include "../animation_frame_action_tool/runtime_data.h"
#include "../btzc_game_engine.h"
#include "../game_object/game_object.h"
#include "../input_handler/input_codes.h"
#include "../input_handler/input_handler.h"
#include "camera.h"
#include "cglm/util.h"
#include "debug_render_job.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "imgui_internal.h"
#include "logger.h"
#include "mesh.h"
#include "misc/cpp/imgui_stdlib.h"
#include "model_animator.h"  // @CHECK: @NOCHECKIN: Is this needed?
#include <array>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>


void BT::ImGui_renderer::set_selected_game_obj(Game_object* game_obj)
{
    m_game_obj_pool->set_selected_game_obj(game_obj);
}

void BT::ImGui_renderer::render_imgui()
{
    // @NOCHECKIN: @TEMP
    static bool s_show_demo_window{ false };
    static ImGuiIO& io = ImGui::GetIO();

    {   // Zoom keyboard shortcut.
        static bool s_prev_pressed_zoom_ks{ false };
        bool pressed_zoom_in_ks{ (m_input_handler->is_key_pressed(BT_KEY_LEFT_CONTROL) ||
                                  m_input_handler->is_key_pressed(BT_KEY_RIGHT_CONTROL)) &&
                                 m_input_handler->is_key_pressed(BT_KEY_EQUAL) };
        bool pressed_zoom_out_ks{ (m_input_handler->is_key_pressed(BT_KEY_LEFT_CONTROL) ||
                                   m_input_handler->is_key_pressed(BT_KEY_RIGHT_CONTROL)) &&
                                  m_input_handler->is_key_pressed(BT_KEY_MINUS) };
        if (!s_prev_pressed_zoom_ks && (pressed_zoom_in_ks || pressed_zoom_out_ks))
        {   // Do zoom in/out.
            io.FontGlobalScale += (pressed_zoom_in_ks ? 0.25f : -0.25f);
            logger::printef(logger::TRACE,
                            "ImGui font global scale to: %.3f",
                            io.FontGlobalScale);
        }

        // Update prev.
        s_prev_pressed_zoom_ks = (pressed_zoom_in_ks || pressed_zoom_out_ks);
    }

    // Context switching.
    static bool s_entering_new_context{ true };
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
    static std::array<void(ImGui_renderer::*)(bool), NUM_EDITOR_CONTEXTS> const k_editor_context_fns{
        &ImGui_renderer::render_imgui__level_editor_context,
        &ImGui_renderer::render_imgui__animation_frame_data_editor_context,
    };
    static Editor_context s_current_editor_context{ Editor_context(1) };

    static auto const s_window_name_w_context_fn = [](char const* const name) {
        return (std::string(name)
                + "##editor_context_"
                + std::to_string(s_current_editor_context));
    };

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
                s_entering_new_context = true;
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
    ImGui::SetNextWindowViewport(ImGui::GetMainViewport()->ID);  // Force game view to stay in main window.
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{ 0, 0 });
    ImGui::Begin(s_window_name_w_context_fn("Main viewport").c_str(),
                 nullptr,
                 ImGuiWindowFlags_NoScrollbar);
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

    // Console.
    ImGui::Begin(s_window_name_w_context_fn("Console").c_str());
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
        ImGui::BeginChild("console_scrolling_region",
                          ImVec2(0, 0), //-footer_height_to_reserve),
                          ImGuiChildFlags_NavFlattened,
                          ImGuiWindowFlags_HorizontalScrollbar);
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

    // Camera properties.
    m_camera->render_imgui(s_window_name_w_context_fn);

    // Demo window.
    if (s_show_demo_window)
        ImGui::ShowDemoWindow(&s_show_demo_window);

    // Context dependant gui.
    auto fn_ptr{ k_editor_context_fns[s_current_editor_context] };
    ((*this).*fn_ptr)(s_entering_new_context);
    s_entering_new_context = false;
}

void BT::ImGui_renderer::render_imgui__level_editor_context(bool enter)
{
    if (enter)
    {   // Load current loaded level.
        m_request_switch_scene_callback("_dev_sample_scene.btscene");
    }

    // Level select.
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

    // Scene hierarchy.
    m_game_obj_pool->render_imgui_scene_hierarchy();

    // Game obj palette.
    ImGui::Begin("Game obj palette");
    {
        ImGui::Text("@TODO: Implement");
    }
    ImGui::End();
}

void BT::ImGui_renderer::render_imgui__animation_frame_data_editor_context(bool enter)
{
    if (enter)
    {   // Load up editor-specific scene.
        m_request_switch_scene_callback("_dev_animation_editor_view.btscene");
    }

    static size_t s_selected_timeline_idx{ 0 };
    static int32_t s_current_animation_clip{ 0 };
    static auto s_all_timeline_names{ anim_frame_action::Bank::get_all_names() };

    // Timeline selection.
    ImGui::Begin("Timeline select");
    {
        static bool s_load_selected_timeline{ true };
        if (ImGui::Button("Refresh list"))
        {   // Reset timeline selection and load first one from refreshed list.
            s_selected_timeline_idx = 0;
            s_all_timeline_names = anim_frame_action::Bank::get_all_names();
            s_load_selected_timeline = true;
        }

        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::Text("Models:%llu Avg:6.9KB Mdn:6.9KB Max:69.4KB", s_all_timeline_names.size());

        ImGui::SeparatorText("List of Timelines");

        for (size_t i = 0; i < s_all_timeline_names.size(); i++)
        {
            auto& timeline_name{ s_all_timeline_names[i] };
            bool is_active_timeline{ s_selected_timeline_idx == i };
            bool is_active_timeline_dirty{ anim_frame_action::s_editor_state.is_working_timeline_dirty };

            ImGui::BeginDisabled(is_active_timeline_dirty || is_active_timeline);
            if (ImGui::Button((timeline_name
                               + (is_active_timeline && is_active_timeline_dirty
                                  ? "*"
                                  : "")).c_str()) &&
                !is_active_timeline)
            {   // Request loading new model.
                s_selected_timeline_idx = i;
                s_load_selected_timeline = true;
            }
            ImGui::EndDisabled();

            if (is_active_timeline && is_active_timeline_dirty)
            {   // Add save or discard buttons.
                ImGui::SameLine();
                if (ImGui::Button("Save changes"))
                {
                    auto const& timeline_name{ s_all_timeline_names[s_selected_timeline_idx] };
                    {   // Save changes.
                        json working_timeline_copy_as_json;
                        anim_frame_action::s_editor_state.working_timeline_copy
                            ->serialize(anim_frame_action::SERIAL_MODE_SERIALIZE,
                                        working_timeline_copy_as_json);

                        // Save to disk.
                        std::ofstream f{
                            BTZC_GAME_ENGINE_ASSET_ANIM_FRAME_ACTIONS_PATH
                            + timeline_name
                            + ".btafa" };
                        f << working_timeline_copy_as_json.dump(4);
                    }

                    anim_frame_action::Bank::replace(
                        timeline_name,
                        std::move(*anim_frame_action::s_editor_state.working_timeline_copy));

                    // Load the same timeline again.
                    s_load_selected_timeline = true;
                }
                ImGui::SameLine();
                if (ImGui::Button("Discard changes"))
                {   // Discard changes by loading the same timeline again.
                    s_load_selected_timeline = true;
                }
            }
        }

        if (s_load_selected_timeline)
        {   // Process load timeline (and model) request.
            if (anim_frame_action::s_editor_state.working_timeline_copy != nullptr)
                delete anim_frame_action::s_editor_state.working_timeline_copy;

            anim_frame_action::s_editor_state.working_timeline_copy =
                new anim_frame_action::Runtime_data(
                    anim_frame_action::Bank::get(s_all_timeline_names[s_selected_timeline_idx]));
            assert(anim_frame_action::s_editor_state.working_timeline_copy != nullptr);

            anim_frame_action::s_editor_state.working_model =
                anim_frame_action::s_editor_state.working_timeline_copy->model;
            assert(anim_frame_action::s_editor_state.working_model != nullptr);

            anim_frame_action::s_editor_state.is_working_timeline_dirty = false;  // Load from disk so not dirty.

            s_load_selected_timeline = false;
        }
    }
    ImGui::End();

    // Animation timeline.
    ImGui::Begin("Animation timeline", nullptr, (anim_frame_action::s_editor_state.is_working_timeline_dirty
                                                 ? ImGuiWindowFlags_UnsavedDocument
                                                 : 0));
    {   // Build anim clips \0 string set.
        std::string anim_names_0_delim{ "\0" };
        std::vector<std::string> anim_names_as_list;  // If new anim gets selected.
        {
            size_t alloc_size{ 0 };
            for (auto& [anim_name, idx] : anim_frame_action::s_editor_state.anim_name_to_idx_map)
            {
                alloc_size += (anim_name.size() + 1);  // +1 for delimiting \0.
            }

            // Allocate proper size in string.
            anim_names_0_delim.resize(alloc_size + 1, '\0');  // +1 for final \0.
            anim_names_as_list.reserve(
                anim_frame_action::s_editor_state.anim_name_to_idx_map.size());

            size_t current_str_pos{ 0 };
            for (auto& [anim_name, idx] : anim_frame_action::s_editor_state.anim_name_to_idx_map)
            {
                strncpy(const_cast<char*>(anim_names_0_delim.c_str() + current_str_pos),
                        anim_name.c_str(),
                        anim_name.size());
                anim_names_as_list.emplace_back(anim_name);
                current_str_pos += (anim_name.size() + 1);
            }
        }
        if (ImGui::Combo("Animation clip", &s_current_animation_clip, anim_names_0_delim.c_str()))
        {   // Change selected anim idx.
            anim_frame_action::s_editor_state.selected_anim_idx =
                anim_frame_action::s_editor_state.anim_name_to_idx_map.at(
                    anim_names_as_list[s_current_animation_clip]);
        }

        // Sequencer component.
        static int32_t s_current_frame = 0;
        static int32_t s_final_frame = 60;

        anim_frame_action::s_editor_state.anim_current_frame =  // A frame late but oh well.
            std::min(std::max(0, s_current_frame), s_final_frame);

        ImGui::PushItemWidth(130);
        ImGui::Text("Displaying frame %llu/%d. %.2f FPS",
                    anim_frame_action::s_editor_state.anim_current_frame,
                    s_final_frame,
                    Model_joint_animation::k_frames_per_second);
        ImGui::InputInt("Selected Frame", &s_current_frame);

        {   // Create new control item.
            static std::string s_new_ctrl_item_name_buffer{ "" };
            static bool s_new_ctrl_item_popup_first;
            if (ImGui::Button("Create new control item.."))
            {
                s_new_ctrl_item_name_buffer = "";
                s_new_ctrl_item_popup_first = true;
                ImGui::OpenPopup("create_new_control_item_popup");
            }
            if (ImGui::BeginPopup("create_new_control_item_popup"))
            {
                if (s_new_ctrl_item_popup_first)
                {
                    ImGui::SetKeyboardFocusHere();
                }
                ImGui::InputText("Name##new_ctrl_item_popup", &s_new_ctrl_item_name_buffer);

                if (ImGui::Button("Confirm##rename_popup") ||
                    m_input_handler->is_key_pressed(BT_KEY_ENTER))
                {   // Create new ctrl item.
                    auto& afa_ctrl_items{ anim_frame_action::s_editor_state
                                              .working_timeline_copy->control_items };
                    afa_ctrl_items.emplace_back(s_new_ctrl_item_name_buffer);

                    anim_frame_action::s_editor_state.is_working_timeline_dirty = true;
                    ImGui::CloseCurrentPopup();
                }

                ImGui::SameLine();

                if (ImGui::Button("Cancel##rename_popup") ||
                    m_input_handler->is_key_pressed(BT_KEY_ESCAPE))
                {   // Cancel!!!
                    ImGui::CloseCurrentPopup();
                }

                ImGui::Text("%s", "Press <Enter> to confirm rename or <Esc> to cancel.");

                s_new_ctrl_item_popup_first = false;
                ImGui::EndPopup();
            }
        }

        ImGui::PopItemWidth();

        // BT sequencer.
        ImGui::BeginChild("BT_sequencer");
        if (anim_frame_action::s_editor_state.anim_name_to_idx_map.empty())
        {
            ImGui::SetWindowFontScale(5.0f);

            // Just show empty message.
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(209/256.0, 186/256.0, 73/256.0, 1));
            ImGui::TextWrapped("Selected model \"%s\" does not contain any animations.",
                               s_all_timeline_names[s_selected_timeline_idx].c_str());
            ImGui::PopStyleColor();
        }
        else
        {
            ImGui::SetWindowFontScale(1.0f);

            auto& afa_ctrl_items{ anim_frame_action::s_editor_state
                                      .working_timeline_copy->control_items };
            auto& afa_regions{ anim_frame_action::s_editor_state
                                   .working_timeline_copy
                                   ->anim_frame_action_timelines[
                                       anim_frame_action::s_editor_state.selected_anim_idx]
                                   .regions };

            static vec2s s_timeline_cell_size{ 16.0f, 24.0f };

            ImDrawList* draw_list = ImGui::GetWindowDrawList();
            ImVec2 canvas_pos = ImGui::GetCursorScreenPos();      // ImDrawList API uses screen coordinates!
            ImVec2 canvas_size = ImGui::GetContentRegionAvail();  // Resize canvas to what's available.

            static float_t s_sequencer_x_offset{ 0.0f };
            static float_t s_sequencer_y_offset{ 0.0f };

            // Clip rects.
            constexpr float_t k_item_list_width_ratio{ 0.3f };
            ImVec2 cr_item_list_min{ canvas_pos };
            ImVec2 cr_item_list_max{ std::floorf(canvas_pos.x + canvas_size.x * k_item_list_width_ratio),
                                     canvas_pos.y + canvas_size.y };
            ImVec2 cr_timeline_min{ cr_item_list_max.x + 1, canvas_pos.y };
            ImVec2 cr_timeline_max{ canvas_pos.x + canvas_size.x,
                                    canvas_pos.y + canvas_size.y };

            constexpr int32_t k_top_measuring_region_height{ 20 };

            // Sequencer control item list.
            ImGui::PushClipRect(cr_item_list_min, cr_item_list_max, true);
            {   // Draw bg.
                draw_list->AddRectFilled(cr_item_list_min, cr_item_list_max, 0xFF362C2B);

                if (ImGui::IsWindowHovered() &&  // @NOTE: "Child" are windows.
                    ImGui::IsMouseHoveringRect(cr_item_list_min,
                                               cr_item_list_max))
                {   // Scrolling behavior.
                    auto& ins{ m_input_handler->get_input_state() };
                    s_sequencer_y_offset +=
                        m_input_handler->get_input_state().ui_scroll_delta.val
                        * 40.0f;
                }

                // RMB input for upcoming ui.
                static bool s_prev_rmb_pressed{ false };
                bool cur_rmb_pressed{ m_input_handler->get_input_state().le_rclick_cam.val };
                bool on_rmb_press{ cur_rmb_pressed && !s_prev_rmb_pressed };
                s_prev_rmb_pressed = cur_rmb_pressed;

                // Control item renaming vars.
                static size_t s_afa_ctrl_item_renaming_idx{ (size_t)-1 };
                static bool s_renaming_popup_first{ true };

                for (size_t i = 0; i < afa_ctrl_items.size(); i++)
                {
                    vec2s y_top_btm;
                    {
                        y_top_btm.s = (canvas_pos.y + s_sequencer_y_offset + k_top_measuring_region_height + 2 + (s_timeline_cell_size.y * i));
                        y_top_btm.t = (y_top_btm.s + s_timeline_cell_size.y);
                    }

                    if (i % 2 == 1)
                    {   // Draw label background for odd rows.
                        draw_list->AddRectFilled(ImVec2{ cr_item_list_min.x, y_top_btm.s },
                                                 ImVec2{ cr_item_list_max.x, y_top_btm.t },
                                                 0x11DDDDDD);
                    }

                    if (i == 0)
                    {   // Draw above line.
                        draw_list->AddLine(ImVec2{ cr_item_list_min.x, y_top_btm.s },
                                           ImVec2{ cr_item_list_max.x, y_top_btm.s },
                                           0x99DDDDDD);
                    }

                    if (i == afa_ctrl_items.size() - 1)
                    {   // Draw below line.
                        draw_list->AddLine(ImVec2{ cr_item_list_min.x, y_top_btm.t },
                                           ImVec2{ cr_item_list_max.x, y_top_btm.t },
                                           0x99DDDDDD);
                    }

                    // Draw control item labels.
                    draw_list->AddText(ImVec2{ canvas_pos.x + 4,
                                               y_top_btm.s + (s_timeline_cell_size.y * 0.5f) - (ImGui::GetFontSize() * 0.5f) },
                                       0xFFFFFFFF,
                                       afa_ctrl_items[i].name.c_str());
                    
                    if (ImGui::IsWindowHovered() &&
                        ImGui::IsMouseHoveringRect(ImVec2{ cr_item_list_min.x, y_top_btm.s },
                                                   ImVec2{ cr_item_list_max.x, y_top_btm.t }) &&
                        on_rmb_press)
                    {   // Right click to rename.
                        s_afa_ctrl_item_renaming_idx = i;
                        s_renaming_popup_first = true;
                        ImGui::OpenPopup("control_item_rename_popup");
                    }
                }

                if (ImGui::BeginPopup("control_item_rename_popup"))
                {   // Rename control item.
                    auto& renaming_afa_ctrl_item{ afa_ctrl_items[s_afa_ctrl_item_renaming_idx] };
                    ImGui::Text("Rename control item \"%s\"", renaming_afa_ctrl_item.name.c_str());

                    static std::string s_rename_buffer;
                    if (s_renaming_popup_first)
                    {   // Setup renaming input text.
                        s_rename_buffer = renaming_afa_ctrl_item.name;
                        ImGui::SetKeyboardFocusHere();
                    }
                    ImGui::InputText("New name##rename_popup", &s_rename_buffer);

                    if (ImGui::Button("Confirm##rename_popup") ||
                        m_input_handler->is_key_pressed(BT_KEY_ENTER))
                    {   // Submit rename.
                        renaming_afa_ctrl_item.name = std::move(s_rename_buffer);
                        anim_frame_action::s_editor_state.is_working_timeline_dirty = true;
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::SameLine();

                    if (ImGui::Button("Cancel##rename_popup") ||
                        m_input_handler->is_key_pressed(BT_KEY_ESCAPE))
                    {   // Cancel!!!
                        ImGui::CloseCurrentPopup();
                    }

                    ImGui::Text("%s", "Press <Enter> to confirm rename or <Esc> to cancel.");

                    s_renaming_popup_first = false;
                    ImGui::EndPopup();
                }
            }
            ImGui::PopClipRect();

            // Sequencer timeline.
            ImGui::PushClipRect(cr_timeline_min, cr_timeline_max, true);
            {   // Draw bg.
                draw_list->AddRectFilled(cr_timeline_min, cr_timeline_max, 0xFF2D2D2D);

                if (ImGui::IsWindowHovered() &&
                    ImGui::IsMouseHoveringRect(cr_timeline_min,
                                               cr_timeline_max))
                {   // Scrolling behavior.
                    auto& ins{ m_input_handler->get_input_state() };
                    if (ins.le_lctrl_mod.val)
                    {   // @TODO: Add focusing onto where the mouse cursor is instead of global cell size.
                        s_timeline_cell_size.x +=
                            m_input_handler->get_input_state().ui_scroll_delta.val
                            * 1.5f;
                    }
                    else
                    {
                        s_sequencer_x_offset +=
                            m_input_handler->get_input_state().ui_scroll_delta.val
                            * 40.0f;
                    }
                }

                for (size_t i = 0; i < afa_ctrl_items.size(); i++)
                {
                    vec2s y_top_btm;
                    {
                        y_top_btm.s = (cr_timeline_min.y + s_sequencer_y_offset + k_top_measuring_region_height + 2 + (s_timeline_cell_size.y * i));
                        y_top_btm.t = (y_top_btm.s + s_timeline_cell_size.y);
                    }

                    if (i % 2 == 1)
                    {   // Draw bg for odd rows.
                        draw_list->AddRectFilled(ImVec2{ cr_timeline_min.x, y_top_btm.s },
                                                 ImVec2{ cr_timeline_max.x, y_top_btm.t },
                                                 0x11DDDDDD);
                    }

                    if (i == 0)
                    {   // Draw above line.
                        draw_list->AddLine(ImVec2{ cr_timeline_min.x, y_top_btm.s },
                                           ImVec2{ cr_timeline_max.x, y_top_btm.s },
                                           0x99DDDDDD);
                    }

                    if (i == afa_ctrl_items.size() - 1)
                    {   // Draw below line.
                        draw_list->AddLine(ImVec2{ cr_timeline_min.x, y_top_btm.t },
                                           ImVec2{ cr_timeline_max.x, y_top_btm.t },
                                           0x99DDDDDD);
                    }
                }

                // Region selecting.
                struct Region_selecting
                {
                    enum Select_state
                    {
                        UNSELECTED,
                        SELECTED,
                        LEFT_DRAG,
                        WHOLE_DRAG,
                        RIGHT_DRAG,
                    };
                    Select_state sel_state{ Select_state::UNSELECTED };
                    using Region = anim_frame_action::Runtime_data::Animation_frame_action_timeline::Region;
                    Region* sel_reg{ nullptr };
                    float_t drag_x_amount{ 0.0f };
                    bool prev_lmb_pressed{ false };
                    bool prev_del_pressed{ false };
                };
                static Region_selecting s_reg_sel;

                // Selecting inputs.
                bool cur_lmb_pressed{ m_input_handler->get_input_state().le_select.val };
                bool on_lmb_press{ cur_lmb_pressed && !s_reg_sel.prev_lmb_pressed };
                bool on_lmb_release{ !cur_lmb_pressed && s_reg_sel.prev_lmb_pressed };
                s_reg_sel.prev_lmb_pressed = cur_lmb_pressed;

                bool cur_del_pressed{ m_input_handler->is_key_pressed(BT_KEY_DELETE) ||
                                      m_input_handler->is_key_pressed(BT_KEY_X) };
                bool on_del_press{ cur_del_pressed && !s_reg_sel.prev_del_pressed };
                s_reg_sel.prev_del_pressed = cur_del_pressed;

                if (s_reg_sel.sel_reg != nullptr)
                {   // Drag region.
                    s_reg_sel.drag_x_amount += m_input_handler->get_input_state()
                                               .look_delta.x.val;
                    while (abs(s_reg_sel.drag_x_amount) >= s_timeline_cell_size.x)
                    {   // Modulate dragged amount and apply to dragging region.
                        int32_t drag_sign{
                            static_cast<int32_t>(glm_signf(s_reg_sel.drag_x_amount)) };
                        s_reg_sel.drag_x_amount -= (s_timeline_cell_size.x
                                                    * drag_sign);

                        bool left_side_drag{ false };
                        if (s_reg_sel.sel_state == Region_selecting::LEFT_DRAG ||
                            s_reg_sel.sel_state == Region_selecting::WHOLE_DRAG)
                        {   // Left side drag.
                            s_reg_sel.sel_reg->start_frame += drag_sign;
                            left_side_drag = true;
                        }
                        if (s_reg_sel.sel_state == Region_selecting::RIGHT_DRAG ||
                            s_reg_sel.sel_state == Region_selecting::WHOLE_DRAG)
                        {   // Right side drag.
                            s_reg_sel.sel_reg->end_frame += drag_sign;
                            left_side_drag = false;
                        }

                        // Check for overlap issue/error after all drag operations.
                        if (left_side_drag)
                        {
                            s_reg_sel.sel_reg->start_frame =
                                glm_min(s_reg_sel.sel_reg->start_frame,
                                        s_reg_sel.sel_reg->end_frame - 1);
                        }
                        else
                        {
                            s_reg_sel.sel_reg->end_frame =
                                glm_max(s_reg_sel.sel_reg->start_frame + 1,
                                        s_reg_sel.sel_reg->end_frame);
                        }

                        // Mark working timeline as dirty.
                        anim_frame_action::s_editor_state.is_working_timeline_dirty = true;
                    }

                    if (on_lmb_release)
                    {   // Release drag.
                        s_reg_sel.sel_state = Region_selecting::SELECTED;
                    }

                    if (on_lmb_press)
                    {   // Deselect selected region.
                        s_reg_sel.sel_state = Region_selecting::UNSELECTED;
                        s_reg_sel.sel_reg = nullptr;
                    }
                }

                // Detect whether cursor is over empty area on timeline.
                bool is_hovering_over_timeline{
                    ImGui::IsWindowHovered() &&
                    ImGui::IsMouseHoveringRect(ImVec2(cr_timeline_min.x, cr_timeline_min.y + glm_max(0, s_sequencer_y_offset) + k_top_measuring_region_height + 2),
                                               ImVec2(cr_timeline_max.x, glm_min(cr_timeline_max.y, cr_timeline_min.y + s_sequencer_y_offset + k_top_measuring_region_height + 2 + (s_timeline_cell_size.y * afa_ctrl_items.size())))) };
                bool is_hovering_over_timeline_region{ false };  // Check in upcoming block.

                for (auto& region : afa_regions)
                {   // Draw bars for regions.
                    vec2s region_bar_top_bottom{
                        cr_timeline_min.y + s_sequencer_y_offset + k_top_measuring_region_height + 2 + (s_timeline_cell_size.y * region.ctrl_item_idx) + 1,
                        cr_timeline_min.y + s_sequencer_y_offset + k_top_measuring_region_height + 2 + (s_timeline_cell_size.y * (region.ctrl_item_idx + 1)) - 1 };
                    ImVec2 p_min{ cr_timeline_min.x + s_sequencer_x_offset + (region.start_frame * s_timeline_cell_size.x) + 1, region_bar_top_bottom.s };
                    ImVec2 p_max{ cr_timeline_min.x + s_sequencer_x_offset + (region.end_frame * s_timeline_cell_size.x) - 1, region_bar_top_bottom.t };
                    draw_list->AddRectFilled(p_min,
                                             p_max,
                                             0x5500FF00,
                                             4.0f);
                    bool is_selected_region{ &region == s_reg_sel.sel_reg };
                    draw_list->AddRect(p_min,
                                       p_max,
                                       (is_selected_region ? 0xFF3176F5 : 0x55FFFFFF),
                                       4.0f,
                                       NULL,
                                       (is_selected_region ? 4.0f : 2.0f));

                    // Adjustment handles.
                    constexpr int32_t k_side_handle_size{ 4 };
                    if (ImGui::IsWindowHovered() &&
                        ImGui::IsMouseHoveringRect(ImVec2(p_min),
                                                   ImVec2(p_min.x + k_side_handle_size, p_max.y)))
                    {   // Left side.
                        is_hovering_over_timeline_region = true;
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                        if (on_lmb_press)
                        {
                            s_reg_sel.sel_state = Region_selecting::LEFT_DRAG;
                            s_reg_sel.sel_reg = &region;
                            s_reg_sel.drag_x_amount = 0.0f;
                        }
                    }
                    else if (ImGui::IsWindowHovered() &&
                             ImGui::IsMouseHoveringRect(ImVec2(p_min.x + k_side_handle_size, p_min.y),
                                                        ImVec2(p_max.x - k_side_handle_size, p_max.y)))
                    {   // Move region.
                        is_hovering_over_timeline_region = true;
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
                        if (on_lmb_press)
                        {
                            s_reg_sel.sel_state = Region_selecting::WHOLE_DRAG;
                            s_reg_sel.sel_reg = &region;
                            s_reg_sel.drag_x_amount = 0.0f;
                        }
                    }
                    else if (ImGui::IsWindowHovered() &&
                             ImGui::IsMouseHoveringRect(ImVec2(p_max.x - k_side_handle_size, p_min.y),
                                                        ImVec2(p_max)))
                    {   // Right side.
                        is_hovering_over_timeline_region = true;
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                        if (on_lmb_press)
                        {
                            s_reg_sel.sel_state = Region_selecting::RIGHT_DRAG;
                            s_reg_sel.sel_reg = &region;
                            s_reg_sel.drag_x_amount = 0.0f;
                        }
                    }
                }

                // Draw measuring region bg.
                draw_list->AddRectFilled(cr_timeline_min, ImVec2(cr_timeline_max.x, cr_timeline_min.y + k_top_measuring_region_height + 2), 0x99000000);

                // Draw measuring lines and numbers.
                constexpr int32_t k_frame_start{ 0 };
                s_final_frame =
                    anim_frame_action::s_editor_state.selected_anim_num_frames;

                for (int32_t i = k_frame_start; i <= s_final_frame; i++)
                {
                    float_t line_x{ cr_timeline_min.x + s_sequencer_x_offset + (i * s_timeline_cell_size.x) };

                    // Draw top measuring line.
                    constexpr std::array<float_t, 10> k_baseline_heights{
                        0, 15, 15, 15, 15,
                        10, 15, 15, 15, 15,
                    };
                    draw_list->AddLine(ImVec2(line_x, cr_timeline_min.y + k_baseline_heights[i % 10]),
                                       ImVec2(line_x, cr_timeline_min.y + k_top_measuring_region_height),
                                       0xFFFFFFFF);

                    if (i == k_frame_start || i == s_final_frame)
                    {   // Draw full height start-end lines.
                        draw_list->AddLine(ImVec2(line_x, cr_timeline_min.y + k_top_measuring_region_height),
                                           ImVec2(line_x, cr_timeline_max.y),
                                           0x77DDDDDD);
                    }

                    if (i % 10 == 0)
                    {   // Draw 10s benchmark number.
                        auto number_label{ std::to_string(i) };
                        // @TODO: Fix the offset. Look into how it's done in `imgui_renderer.cpp` and stuff.
                        draw_list->AddText(ImVec2(cr_timeline_min.x
                                                  + s_sequencer_x_offset
                                                  + (i * s_timeline_cell_size.x)
                                                  + 4,
                                                  cr_timeline_min.y + 0),
                                           0xFFFFFFFF,
                                           number_label.c_str());
                    }
                }

                {   // Draw current frame line.
                    auto cur_frame_str{ std::to_string(s_current_frame) };
                    float_t cur_frame_line_x{ cr_timeline_min.x + s_sequencer_x_offset + (s_current_frame * s_timeline_cell_size.x) };

                    draw_list->AddLine(ImVec2(cur_frame_line_x, cr_timeline_min.y + 0),
                                       ImVec2(cur_frame_line_x, cr_timeline_max.y),
                                       0xFF7A50FA,
                                       2.0f);
                    draw_list->AddRectFilled(ImVec2(cur_frame_line_x, cr_timeline_min.y + 0),
                                             ImVec2(cur_frame_line_x + 4 + ImGui::CalcTextSize(cur_frame_str.c_str()).x + 4, cr_timeline_min.y + k_top_measuring_region_height),
                                             0xFF7A50FA);
                    draw_list->AddText(ImVec2(cur_frame_line_x + 4, cr_timeline_min.y + 2),
                                       0xFFFFFFFF,
                                       cur_frame_str.c_str());
                }

                static bool s_move_current_frame_to_mouse_active{ false };  // For click-n-drag.
                if (on_lmb_release)
                    s_move_current_frame_to_mouse_active = false;
                if ((s_move_current_frame_to_mouse_active && cur_lmb_pressed) ||
                    (on_lmb_press &&
                     ImGui::IsWindowHovered() &&
                     ImGui::IsMouseHoveringRect(ImVec2(cr_timeline_min),
                                                ImVec2(cr_timeline_max.x,
                                                       cr_timeline_min.y + k_top_measuring_region_height + 2))))
                {   // Move current frame to mouse.
                    s_move_current_frame_to_mouse_active = true;
                    float_t zoom_relative_mouse_x{ (ImGui::GetIO().MousePos.x
                                                    - (cr_timeline_min.x + s_sequencer_x_offset))
                                                   / s_timeline_cell_size.x };
                    s_current_frame = std::roundf(zoom_relative_mouse_x);
                }
                else if (is_hovering_over_timeline &&
                         !is_hovering_over_timeline_region &&
                         s_reg_sel.sel_state <= Region_selecting::SELECTED)  // Not doing a drag operation.
                {   // Prompt creating new region w/ tooltip.
                    ImGui::SetTooltip("Press Shift+A to create new region.");

                    static bool s_prev_is_key_a_pressed{ false };
                    bool cur_is_key_a_pressed{ m_input_handler->is_key_pressed(BT_KEY_A) };
                    if (cur_is_key_a_pressed &&
                        !s_prev_is_key_a_pressed &&
                        m_input_handler->is_key_pressed(BT_KEY_LEFT_SHIFT))
                    {   // Create new region since empty space selected.
                        ImVec2 mouse_pos{ ImGui::GetIO().MousePos };
                        float_t zoom_relative_mouse_x{ (mouse_pos.x
                                                        - (cr_timeline_min.x + s_sequencer_x_offset))
                                                       / s_timeline_cell_size.x };
                        uint32_t ctrl_item_idx{
                            static_cast<uint32_t>((mouse_pos.y
                                                   - (cr_timeline_min.y
                                                      + s_sequencer_y_offset
                                                      + k_top_measuring_region_height + 2))
                                                  / s_timeline_cell_size.y) };
                        int32_t start_frame{
                            static_cast<int32_t>(std::floorf(zoom_relative_mouse_x)) };

                        afa_regions.emplace_back(ctrl_item_idx,
                                               start_frame,
                                               start_frame + 4);
                        
                        // Immediately assign created region as selected.
                        // (Just in case there may be some kind of vector resizing
                        //  which makes the pointers stale. I hate this issue too)
                        s_reg_sel.sel_state = Region_selecting::SELECTED;
                        s_reg_sel.sel_reg = &afa_regions.back();

                        // Mark working timeline as dirty.
                        anim_frame_action::s_editor_state.is_working_timeline_dirty = true;
                    }

                    s_prev_is_key_a_pressed = cur_is_key_a_pressed;
                }
                else if (on_del_press && s_reg_sel.sel_state == Region_selecting::SELECTED)
                {   // Delete selected region.
                    assert(s_reg_sel.sel_reg != nullptr);
                    for (size_t i = afa_regions.size() - 1;; i--)
                    {
                        if (&afa_regions[i] == s_reg_sel.sel_reg)
                        {   // Found the one to delete.
                            afa_regions.erase(afa_regions.begin() + i);
                            break;
                        }
                        if (i == 0)
                        {   // Searching failed. Abort/exit.
                            logger::printe(logger::ERROR,
                                           "Delete selected region searching failed.");
                            assert(false);
                            break;
                        }
                    }

                    // Clear selection state.
                    // (Do this right after to prevent stale pointer issues)
                    s_reg_sel.sel_state = Region_selecting::UNSELECTED;
                    s_reg_sel.sel_reg = nullptr;

                    // Mark working timeline as dirty.
                    anim_frame_action::s_editor_state.is_working_timeline_dirty = true;
                }
            }
            ImGui::PopClipRect();
        }
        ImGui::EndChild();
    }
    ImGui::End();
}
