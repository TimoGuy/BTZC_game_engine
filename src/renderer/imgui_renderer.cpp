#include "imgui_renderer.h"

#include "../btzc_game_engine.h"
#include "../game_object/game_object.h"
#include "../input_handler/input_codes.h"
#include "../input_handler/input_handler.h"
#include "camera.h"
#include "debug_render_job.h"
#include "imgui.h"
#include "ImGuizmo.h"
#include "ImSequencer.h"
#include "ImCurveEdit.h"  // @TEMP @NOCHECKIN
#include "imgui_internal.h"
#include "logger.h"
#include <array>
#include <fstream>
#include <string>


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
    static std::array<void(ImGui_renderer::*)(), NUM_EDITOR_CONTEXTS> const k_editor_context_fns{
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

    // Camera properties.
    m_camera->render_imgui(s_window_name_w_context_fn);

    // Demo window.
    if (s_show_demo_window)
        ImGui::ShowDemoWindow(&s_show_demo_window);

    // Context dependant gui.
    auto fn_ptr{ k_editor_context_fns[s_current_editor_context] };
    ((*this).*fn_ptr)();
}

void BT::ImGui_renderer::render_imgui__level_editor_context()
{   // Level select.
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

namespace
{

class Animation_frame_data_sequencer
    : public ImSequencer::SequenceInterface
{
public:
    int32_t mFrameMin = 0;
    int32_t mFrameMax = 60;

    int32_t GetFrameMin() const override
    {
        return mFrameMin;
    }

    int32_t GetFrameMax() const override
    {
        return mFrameMax;
    }

    int32_t GetItemCount() const override
    {
        return 5;
    }

    void BeginEdit(int32_t index) override
    {
        BT::logger::printef(BT::logger::TRACE, "Fn: BeginEdit(%i);", index);
    }

    void EndEdit() override
    {
        BT::logger::printe(BT::logger::TRACE, "Fn: EndEdit();");
    }

    int32_t GetItemTypeCount() const override
    {
        return 1;
    }

    char const* GetItemTypeName(int32_t type_index) const override
    {
        return "Something type name";
    }

    char const* GetItemLabel(int32_t index) const override
    {
        switch (index)
        {
            case 0: return "squeegee";
            case 1: return "hip hop";
            case 2: return "dementia";
            case 3: return "solo leveling";
            case 4: return "transgenders";
            default: assert(false); return "";
        }
    }

    char const* GetCollapseFmt() const override
    {
        return "%d Frames / %d entries";
    }

    struct MySequenceItem
    {
        int mType = 0;
        int mFrameStart = 4, mFrameEnd = 40;
        bool mExpanded = false;
    };
    std::array<MySequenceItem, 5> myItems;

    void Get(int32_t index, int32_t** start, int32_t** end, int32_t* type, uint32_t* color) override
    {
        MySequenceItem& item = myItems[index];
        if (color)
            *color = 0xFFAA8080; // same color for everyone, return color based on type
        if (start)
            *start = &item.mFrameStart;
        if (end)
            *end = &item.mFrameEnd;
        if (type)
            *type = item.mType;
    }

    void Add(int32_t type) override
    {
        BT::logger::printef(BT::logger::TRACE, "Fn: Add(%i);", type);
    }

    void Del(int32_t index) override
    {
        BT::logger::printef(BT::logger::TRACE, "Fn: Del(%i);", index);
    }

    void Duplicate(int32_t index) override
    {
        BT::logger::printef(BT::logger::TRACE, "Fn: Duplicate(%i);", index);
    }

    void Copy() override
    {
        BT::logger::printe(BT::logger::TRACE, "Fn: Copy();");
    }

    void Paste() override
    {
        BT::logger::printe(BT::logger::TRACE, "Fn: Paste();");
    }

    size_t GetCustomHeight(int32_t index) override
    {
        return 0;
    }

    void DoubleClick(int32_t index) override
    {
        BT::logger::printef(BT::logger::TRACE, "Fn: DoubleClick(%i);", index);
    }

    void CustomDraw(int32_t index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect) override
    {
        static const char* labels[] = { "Translation", "Rotation" , "Scale" };

        // rampEdit.mMax = ImVec2(float(mFrameMax), 1.f);
        // rampEdit.mMin = ImVec2(float(mFrameMin), 0.f);
        draw_list->PushClipRect(legendClippingRect.Min, legendClippingRect.Max, true);
        for (int i = 0; i < 3; i++)
        {
            // ImVec2 pta(legendRect.Min.x + 30, legendRect.Min.y + i * 14.f);
            // ImVec2 ptb(legendRect.Max.x, legendRect.Min.y + (i + 1) * 14.f);
            // draw_list->AddText(pta, rampEdit.mbVisible[i] ? 0xFFFFFFFF : 0x80FFFFFF, labels[i]);
            // if (ImRect(pta, ptb).Contains(ImGui::GetMousePos()) && ImGui::IsMouseClicked(0))
            //     rampEdit.mbVisible[i] = !rampEdit.mbVisible[i];
        }
        draw_list->PopClipRect();

        ImGui::SetCursorScreenPos(rc.Min);
        // ImCurveEdit::Edit(rampEdit, rc.Max - rc.Min, 137 + index, &clippingRect);
    }

    void CustomDrawCompact(int32_t index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect) override
    {
        // rampEdit.mMax = ImVec2(float(mFrameMax), 1.f);
        // rampEdit.mMin = ImVec2(float(mFrameMin), 0.f);
        draw_list->PushClipRect(clippingRect.Min, clippingRect.Max, true);
        for (int i = 0; i < 3; i++)
        {
            // for (unsigned int j = 0; j < rampEdit.mPointCount[i]; j++)
            // {
            //     float p = rampEdit.mPts[i][j].x;
            //     if (p < myItems[index].mFrameStart || p > myItems[index].mFrameEnd)
            //         continue;
            //     float r = (p - mFrameMin) / float(mFrameMax - mFrameMin);
            //     float x = ImLerp(rc.Min.x, rc.Max.x, r);
            //     draw_list->AddLine(ImVec2(x, rc.Min.y + 6), ImVec2(x, rc.Max.y - 4), 0xAA000000, 4.f);
            // }
        }
        draw_list->PopClipRect();
    }
};

}  // namespace

void BT::ImGui_renderer::render_imgui__animation_frame_data_editor_context()
{
    static int32_t s_current_animation_clip{ 0 };

    // Model selection.
    ImGui::Begin("Model select");
    {
        ImGui::Button("Refresh list");
        ImGui::SameLine();
        ImGui::Spacing();
        ImGui::SameLine();
        ImGui::Text("Models:69 Avg:6.9KB Mdn:6.9KB Max:69.4KB");

        ImGui::SeparatorText("List of Models");

        ImGui::Text("@TODO: Implement");
    }
    ImGui::End();

    // Animation timeline.
    ImGui::Begin("Animation timeline");
    {
        ImGui::Combo("Animation clip", &s_current_animation_clip, "Idle\0Running\0Jump\0Fall\0");
        ImGui::Text("Frame: 4/20");

        ImGui::Separator();

        static Animation_frame_data_sequencer s_sequencer;
        // let's create the sequencer
        static int selectedEntry = -1;
        static int firstFrame = 0;
        static bool expanded = true;
        static int currentFrame = 30;

        ImGui::PushItemWidth(130);
        ImGui::InputInt("Frame Min", &s_sequencer.mFrameMin);
        ImGui::SameLine();
        ImGui::InputInt("Frame ", &currentFrame);
        ImGui::SameLine();
        ImGui::InputInt("Frame Max", &s_sequencer.mFrameMax);
        ImGui::PopItemWidth();

        // BT sequencer.
        if (ImGui::BeginChild("BT_sequencer"))
        {
            struct Control_item
            {
                std::string name;
            };
            static std::vector<Control_item> s_ctrl_items{
                { "JOJO" },
                { "BF6" },
                { "Lerp to sweet happiness" },
                { "Enable hurtbox 1" },
            };
            struct Region
            {
                uint32_t ctrl_item_idx;
                int32_t  start_frame;
                int32_t  end_frame;
            };
            static std::vector<Region> s_regions{
                { 0, 0, 5 },
                { 0, 5, 6 },
                { 0, 30, 35 },
                { 1, 30, 35 },
                { 2, 30, 35 },
                { 3, 30, 35 },
            };

            // static float_t s_timeline_zoom_x{ 1.0f };  IGNORE FOR NOW.
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

                if (ImGui::IsMouseHoveringRect(cr_item_list_min,
                                               cr_item_list_max))
                {   // Scrolling behavior.
                    auto& ins{ m_input_handler->get_input_state() };
                    s_sequencer_y_offset +=
                        m_input_handler->get_input_state().ui_scroll_delta.val
                        * 40.0f;
                }

                for (size_t i = 0; i < s_ctrl_items.size(); i++)
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

                    if (i == s_ctrl_items.size() - 1)
                    {   // Draw below line.
                        draw_list->AddLine(ImVec2{ cr_item_list_min.x, y_top_btm.t },
                                           ImVec2{ cr_item_list_max.x, y_top_btm.t },
                                           0x99DDDDDD);
                    }

                    // Draw control item labels.
                    draw_list->AddText(ImVec2{ canvas_pos.x + 4,
                                               y_top_btm.s + (s_timeline_cell_size.y * 0.5f) - (ImGui::GetFontSize() * 0.5f) },
                                       0xFFFFFFFF,
                                       s_ctrl_items[i].name.c_str());
                }
            }
            ImGui::PopClipRect();

            // Sequencer timeline.
            ImGui::PushClipRect(cr_timeline_min, cr_timeline_max, true);
            {   // Draw bg.
                draw_list->AddRectFilled(cr_timeline_min, cr_timeline_max, 0xFF2D2D2D);

                if (ImGui::IsMouseHoveringRect(cr_timeline_min,
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

                for (size_t i = 0; i < s_ctrl_items.size(); i++)
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

                    if (i == s_ctrl_items.size() - 1)
                    {   // Draw below line.
                        draw_list->AddLine(ImVec2{ cr_timeline_min.x, y_top_btm.t },
                                           ImVec2{ cr_timeline_max.x, y_top_btm.t },
                                           0x99DDDDDD);
                    }
                }

                for (auto& region : s_regions)
                {   // Draw bars for regions.
                    vec2s region_bar_top_bottom{
                        cr_timeline_min.y + s_sequencer_y_offset + k_top_measuring_region_height + 2 + (s_timeline_cell_size.y * region.ctrl_item_idx) + 1,
                        cr_timeline_min.y + s_sequencer_y_offset + k_top_measuring_region_height + 2 + (s_timeline_cell_size.y * (region.ctrl_item_idx + 1)) - 1 };
                    ImVec2 p_min{ cr_timeline_min.x + s_sequencer_x_offset + (region.start_frame * s_timeline_cell_size.x) + 1, region_bar_top_bottom.s };
                    ImVec2 p_max{ cr_timeline_min.x + s_sequencer_x_offset + (region.end_frame * s_timeline_cell_size.x) - 1, region_bar_top_bottom.t };
                    draw_list->AddRectFilled(p_min,
                                             p_max,
                                             0xFF005500,
                                             2.0f);

                    // Adjustment handles.
                    constexpr int32_t k_side_handle_size{ 4 };
                    if (ImGui::IsMouseHoveringRect(ImVec2(p_min),
                                                   ImVec2(p_min.x + k_side_handle_size, p_max.y)))
                    {   // Left side.
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                        // @TODO: Add the clicking features.
                    }
                    else if (ImGui::IsMouseHoveringRect(ImVec2(p_min.x + k_side_handle_size, p_min.y),
                                                        ImVec2(p_max.x - k_side_handle_size, p_max.y)))
                    {   // Move region.
                        ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
                        // @TODO: Add the clicking features.
                    }
                    else if (ImGui::IsMouseHoveringRect(ImVec2(p_max.x - k_side_handle_size, p_min.y),
                                                        ImVec2(p_max)))
                    {   // Right side.
                        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);
                        // @TODO: Add the clicking features.
                    }
                }

                // Draw measuring region bg.
                draw_list->AddRectFilled(cr_timeline_min, ImVec2(cr_timeline_max.x, cr_timeline_min.y + k_top_measuring_region_height + 2), 0x99000000);

                // Draw measuring lines and numbers.
                int32_t frame_start{ 0 };
                int32_t frame_end{ 100 };
                for (int32_t i = frame_start; i <= frame_end; i++)
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

                    if (i == frame_start || i == frame_end)
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
                    auto cur_frame_str{ std::to_string(currentFrame) };
                    float_t cur_frame_line_x{ cr_timeline_min.x + s_sequencer_x_offset + (currentFrame * s_timeline_cell_size.x) };

                    draw_list->AddLine(ImVec2(cur_frame_line_x, cr_timeline_min.y + 0),
                                       ImVec2(cur_frame_line_x, cr_timeline_max.y),
                                       0xFF992200,
                                       2.0f);
                    draw_list->AddRectFilled(ImVec2(cur_frame_line_x, cr_timeline_min.y + 0),
                                             ImVec2(cur_frame_line_x + 4 + (ImGui::GetFontSize() * cur_frame_str.length() * 0.5f) + 4, cr_timeline_min.y + k_top_measuring_region_height),
                                             0xFF992200);
                    draw_list->AddText(ImVec2(cur_frame_line_x + 4, cr_timeline_min.y + 0),
                                       0xFFFFFFFF,
                                       cur_frame_str.c_str());
                }
            }
            ImGui::PopClipRect();

            ImGui::EndChild();
        }

        #if 0
        ImSequencer::Sequencer(&s_sequencer, &currentFrame, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_ADD | ImSequencer::SEQUENCER_DEL | ImSequencer::SEQUENCER_COPYPASTE | ImSequencer::SEQUENCER_CHANGE_FRAME);
        // @TODO vvvv
        // // add a UI to edit that particular item
        // if (selectedEntry != -1)
        // {
        //     const MySequence::MySequenceItem &item = mySequence.myItems[selectedEntry];
        //     ImGui::Text("I am a %s, please edit me", SequencerItemTypeNames[item.mType]);
        //     // switch (type) ....
        // }
        #endif  //0
    }
    ImGui::End();
}
