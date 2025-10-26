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


//     auto& entity_container{ service_finder::find_service<Entity_container>() };

//     auto& reg{ entity_container.get_ecs_registry() };
//     auto changed_trans_view = reg.view<component::Transform,
//                                        component::Transform_hierarchy,
//                                        component::Transform_changed>();
//     auto trans_view = reg.view<component::Transform,
//                                component::Transform_hierarchy>();

//     // Calculate delta transforms.
//     for (auto entity : changed_trans_view)
//     {
//         assert(false);  // @TODO: TEST THIS WHEN ITS WORKING.

//         auto const& transform{ changed_trans_view.get<component::Transform const>(entity) };
//         auto const& transform_hierarchy{
//             changed_trans_view.get<component::Transform_hierarchy const>(entity)
//         };
//         auto& transform_changed{ changed_trans_view.get<component::Transform_changed>(entity) };

//         // Invert `prev_transform`.
//         component::Transform inv_prev_transform =
//             inverse_transform(transform_changed.prev_transform);

//         // Append current transform to `inv_prev_transform` to get delta transform.
//         // @NOTE: Store the result inside of `prev_transform`, even though it's not technically the
//         //        right name for it.
//         transform_changed.prev_transform = append_transform(inv_prev_transform, transform);
//     }

//     // Propagate transforms with delta transforms.
//     for (auto entity : changed_trans_view)
//     {
//         auto const& transform_hierarchy{
//             changed_trans_view.get<component::Transform_hierarchy const>(entity)
//         };
//         auto const& transform_changed{ changed_trans_view.get<component::Transform_changed>(entity) };

//         // @NOTE: Prev step inserts delta transform into here teehee.
//         auto const& delta_transform{ transform_changed.prev_transform };

//         for (auto entity : transform_hierarchy.children_entities)
//         {
//             apply_delta_transform_recursive(trans_view,
//                                             entity_container,
//                                             entity_container.find_entity(entity),
//                                             delta_transform);
//         }
//     }

//     // Mark all transforms as propagated now (i.e. remove "changed" flag).
//     reg.clear<component::Transform_changed>();
}
