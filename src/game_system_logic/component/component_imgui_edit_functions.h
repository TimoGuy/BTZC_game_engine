#pragma once

#include "entt/entity/fwd.hpp"
#include <string>


namespace BT
{
namespace component
{
namespace edit
{
namespace internal
{

/// Creates a simple header for the start of the component editing view.
bool imgui_open_component_editing_header(std::string const& label);

/// For popping a tree.
void imgui_tree_pop();

/// Creates a simple piece of blank space.
void imgui_blank_space(float_t height);

/// Creates a simple separator.
void imgui_separator();

}  // namespace internal

// ImGui edit functions.
void imgui_edit__sample(entt::registry&, entt::entity);  // @NOTE: this is just a sample, placeholder one!
void imgui_edit__entity_metadata(entt::registry& reg, entt::entity ecs_entity);
void imgui_edit__transform(entt::registry& reg, entt::entity ecs_entity);

}  // namespace edit
}  // namespace component
}  // namespace BT