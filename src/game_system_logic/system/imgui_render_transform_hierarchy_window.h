#pragma once

#include "entt/entity/fwd.hpp"


namespace BT
{
namespace system
{

/// Renders the whole transform hierarchy window using the IMGUI platform.
void imgui_render_transform_hierarchy_window(bool clear_state);

/// Sets selected entity so that it will be rendered in debug views.
void set_selected_entity(entt::entity entity);

/// Updates debug view's selected entity to keep it as the same transform as the render object it
/// belongs to.
void update_selected_entity_debug_render_transform();

}  // namespace system
}  // namespace BT
