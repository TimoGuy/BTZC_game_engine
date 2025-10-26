#include "imgui_render_transform_hierarchy_window.h"

#include "entt/entity/entity.hpp"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "ImGuizmo.h"

#include <cassert>


namespace
{

struct State
{
    entt::entity selected_entity{ entt::null };
} s_state;

/// Renders entities not belonging in the transform hierarchy.
/// Returns `true` if rendered, `false` if not.
bool internal_imgui_render_floating_entities()
{
    return false;
}

/// Renders entities belonging to the transform hierarchy in a cascading node-like fashion.
/// Returns `true` if rendered, `false` if not.
bool internal_imgui_render_entity_transform_hierarchy()
{
    return false;
}

void internal_imgui_render_entities()
{
    ImGui::Begin("Entities");

    bool displayed_anything{ false };
    displayed_anything |= internal_imgui_render_floating_entities();
    displayed_anything |= internal_imgui_render_entity_transform_hierarchy();

    if (!displayed_anything)
    {
        ImGui::Text("No entities exist currently.");
    }

    ImGui::End();
}

void internal_imgui_render_item_properties_inspector()
{
    ImGui::Begin("Properties inspector");
    if (s_state.selected_entity == entt::null)
    {
        ImGui::Text("Select something to inspect its properties.");
    }
    else
    {
        assert(false);  // @TODO: Implement!!!
        // auto game_obj{ m_game_objects.at(m_selected_game_obj).get() };

        // auto name{ game_obj->get_name() };
        // if (ImGui::InputText("Name", &name))
        //     game_obj->set_name(std::move(name));

        // ImGui::Text("UUID: %s", UUID_helper::to_pretty_repr(game_obj->get_uuid()).c_str());

        // if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        // {
        //     game_obj->render_imgui_local_transform();
        // }

        // game_obj->render_imgui_transform_gizmo();
    }
    ImGui::End();
}

}  // namespace


void BT::system::imgui_render_transform_hierarchy_window(bool clear_state)
{
    if (clear_state)
    {
        s_state = {};
    }

    internal_imgui_render_entities();
    internal_imgui_render_item_properties_inspector();
}
