#pragma once

#include "btjson.h"
#include "entt/entity/fwd.hpp"

#include <optional>
#include <string>


namespace BT
{
namespace component
{

/// Sets up all components to ensure that there are proper serialization/deserialization procedures
/// created.
void register_all_components();

/// Takes component name and serialized member data and constructs a component into the ECS entity.
void construct_component(entt::entity ecs_entity,
                         std::string const& type_name,
                         json const& members_j);

/// Finds component name via component ID type. Crashes if not found.
std::string find_component_name(entt::id_type comp_id);

/// Serializes component members of the provided entity.
std::optional<json> serialize_component_of_entity(entt::entity ecs_entity, entt::id_type comp_id);

/// Renders ImGui sections inside a window for property inspection/editing for all components.
void imgui_render_components_edit_panes(entt::entity ecs_entity);

}  // namespace component
}  // namespace BT
