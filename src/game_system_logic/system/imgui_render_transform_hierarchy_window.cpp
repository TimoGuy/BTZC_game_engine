#include "imgui_render_transform_hierarchy_window.h"

#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "game_system_logic/component/entity_metadata.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"
#include "ImGuizmo.h"
#include "service_finder/service_finder.h"
#include "uuid/uuid.h"

#include <cassert>



namespace
{

using namespace BT;

struct State
{
    entt::entity selected_entity{ entt::null };
};
static State s_state;

/// Gets appropriate node flags depending on the modifiers.
constexpr ImGuiTreeNodeFlags get_tree_node_flags(bool is_selected, bool is_leaf_node)
{   // Base flags.
    ImGuiTreeNodeFlags flags{ ImGuiTreeNodeFlags_SpanAvailWidth |
                              ImGuiTreeNodeFlags_DrawLinesToNodes |
                              ImGuiTreeNodeFlags_OpenOnDoubleClick |
                              ImGuiTreeNodeFlags_OpenOnArrow |
                              ImGuiTreeNodeFlags_DefaultOpen };

    if (is_selected)
        flags |= ImGuiTreeNodeFlags_Selected;

    if (is_leaf_node)
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    return flags;
}

/// Renders entities not belonging in the transform hierarchy.
/// Returns `true` if rendered, `false` if not.
bool internal_imgui_render_floating_entities()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Entity_metadata const>(
        entt::exclude<component::Transform, component::Transform_hierarchy>) };

    // Fill in data structure of nodes from view.
    struct Flat_node
    {
        std::string name;
        UUID uuid;
    };
    std::vector<Flat_node> entity_flat_nodes;

    for (auto entity : view)
    {
        auto const& metadata{ view.get<component::Entity_metadata const>(entity) };
        entity_flat_nodes.emplace_back(metadata.name, metadata.uuid);
    }

    // Check if should display the nodes.
    bool displays{ !entity_flat_nodes.empty() };

    // Open header for displaying entities.
    if (displays && ImGui::CollapsingHeader("Floating entities##render_floating_entities",
                                            ImGuiTreeNodeFlags_DefaultOpen))
    {   // Display all entities.
        for (auto const& node : entity_flat_nodes)
        {
            auto ecs_entity{ entity_container.find_entity(node.uuid) };

            // Draw my node.
            ImGui::TreeNodeEx(reinterpret_cast<void*>(ecs_entity),
                              get_tree_node_flags(ecs_entity == s_state.selected_entity, true),
                              "%s",
                              node.name.c_str());

            if (ImGui::IsItemClicked())
                s_state.selected_entity = ecs_entity;
        }
    }

    return displays;
}

/// Struct for holding hierarchy nodes.
struct Hierarchy_node
{
    uint32_t indentation;
    std::string name;
    UUID uuid;
};

/// Recursively traverses hierarchy of transforms, filling in the struct for hierarchy nodes.
void internal_recursive_iterate_transform_hierarchy(
    uint32_t indentation,
    entt::entity entity,
    Entity_container const& entity_container,
    entt::registry const& view,
    std::vector<Hierarchy_node>& entity_hierarchy_nodes)
{
    auto const& metadata{ view.get<component::Entity_metadata const>(entity) };
    entity_hierarchy_nodes.emplace_back(indentation, metadata.name, metadata.uuid);

    // Process children.
    auto const& trans_hierarchy{ view.get<component::Transform_hierarchy const>(entity) };
    for (auto uuid : trans_hierarchy.children_entities)
    {
        internal_recursive_iterate_transform_hierarchy(indentation + 1,
                                                       entity_container.find_entity(uuid),
                                                       entity_container,
                                                       view,
                                                       entity_hierarchy_nodes);
    }
}

/// Renders entities belonging to the transform hierarchy in a cascading node-like fashion.
/// Returns `true` if rendered, `false` if not.
bool internal_imgui_render_entity_transform_hierarchy()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto view{ reg.view<component::Entity_metadata const,
                        component::Transform const,
                        component::Transform_hierarchy const>() };

    // Fill in data structure of nodes from view.
    std::vector<Hierarchy_node> entity_hierarchy_nodes;

    for (auto entity : view)
    {   // Ensure that this entity is at root level in the hierarchy.
        auto const& trans_hierarchy{ view.get<component::Transform_hierarchy const>(entity) };
        if (trans_hierarchy.parent_entity.is_nil())
        {   // Process as root node.
            internal_recursive_iterate_transform_hierarchy(0,
                                                           entity,
                                                           entity_container,
                                                           reg,
                                                           entity_hierarchy_nodes);
        }
    }

    // Check if should display the nodes.
    bool displays{ !entity_hierarchy_nodes.empty() };

    // Open header for displaying entities.
    if (displays &&
        ImGui::CollapsingHeader("Entity transform hierarchy##render_entity_transform_hierarchy",
                                ImGuiTreeNodeFlags_DefaultOpen))
    {
        uint32_t current_indentation{ 0 };

        // Display all entities.
        size_t node_idx{ 0 };
        for (auto const& node : entity_hierarchy_nodes)
        {   // Skip nodes that don't have their tree open (decided by indentation amount not
            // matching).
            if (node.indentation == current_indentation)
            {   // If next node is more indented than the current indentation, then it is not a leaf
                // node.
                bool has_next_node{ node_idx + 1 < entity_hierarchy_nodes.size() };
                uint32_t next_indentation{ has_next_node
                                               ? entity_hierarchy_nodes[node_idx + 1].indentation
                                               : 0 };
                bool is_leaf_node{ !(has_next_node && current_indentation < next_indentation) };

                // Get ECS entity version of UUID.
                auto ecs_entity{ entity_container.find_entity(node.uuid) };

                // Draw my node.
                bool tree_node_open = ImGui::TreeNodeEx(
                    reinterpret_cast<void*>(ecs_entity),
                    get_tree_node_flags(ecs_entity == s_state.selected_entity, is_leaf_node),
                    "%s",
                    node.name.c_str());

                if (ImGui::IsItemClicked())
                    s_state.selected_entity = ecs_entity;

                // Mess with indentation.
                if (!is_leaf_node && tree_node_open)
                {   // This is an indentation case. Increment the current indentation.
                    current_indentation++;
                    // If these are not the same now, then we have malformed node hierarchy data.
                    assert(current_indentation == next_indentation);
                }

                while (next_indentation < current_indentation)
                {   // Pop the tree until the next indentation is achieved.
                    ImGui::TreePop();
                    current_indentation--;
                }
            }

            node_idx++;
        }
    }

    return displays;
}

/// Renders a hidden button that deselects the selected entity when the user clicks on this "empty
/// space".
void internal_imgui_render_deselect_entity_field()
{
    ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0x00000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0x00000000);
    ImGui::ButtonEx("##deselect_entity_field_hidden_button",
                    ImVec2(ImGui::GetContentRegionAvail().x,
                        std::max(24.0f, ImGui::GetContentRegionAvail().y)),
                    ImGuiButtonFlags_NoNavFocus);
    ImGui::PopStyleColor(3);

    if (ImGui::IsItemClicked())
        s_state.selected_entity = entt::null;
}

/// "Entities" window.
void internal_imgui_render_entities()
{
    ImGui::Begin("Entities");

    bool displayed_anything{ false };
    displayed_anything |= internal_imgui_render_floating_entities();
    displayed_anything |= internal_imgui_render_entity_transform_hierarchy();
    internal_imgui_render_deselect_entity_field();

    if (!displayed_anything)
    {
        ImGui::Text("No entities exist currently.");
    }

    ImGui::End();
}

/// "Properties inspector" window.
void internal_imgui_render_item_properties_inspector()
{
    ImGui::Begin("Properties inspector");
    if (s_state.selected_entity == entt::null)
    {
        ImGui::Text("Select something to inspect its properties.");
    }
    else
    {
        ImGui::Text("@TODO: IMPLEMENT!!!!.");
        // assert(false);  // @TODO: Implement!!!





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
