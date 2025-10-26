#include "imgui_render_transform_hierarchy_window.h"

#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "game_system_logic/component/entity_metadata.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "imgui.h"
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

/// Renders entities not belonging in the transform hierarchy.
/// Returns `true` if rendered, `false` if not.
bool internal_imgui_render_floating_entities()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
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
            ImGui::Text("%s", node.name.c_str());
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
    {   // Display all entities.
        for (auto const& node : entity_hierarchy_nodes)
        {
            ImGui::Text("%d %s", node.indentation, node.name.c_str());
        }
    }

    return displays;
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
