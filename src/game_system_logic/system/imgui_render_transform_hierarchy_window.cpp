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
#include "renderer/camera.h"
#include "renderer/renderer.h"
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

/// Draws ImGuizmo's mat4 manipulate gizmo.
bool internal_imguizmo_manipulate(entt::registry& reg,
                                  Camera& camera,
                                  rvec3s& out_pos,
                                  versors& out_rot,
                                  vec3s& out_sca)
{   // Get selected entity transform.
    auto const ent_transform{ reg.try_get<component::Transform const>(s_state.selected_entity) };
    if (ent_transform == nullptr)
    {   // Exit since entity does not have a transform.
        return false;
    }

    // Calculate TRS into mat4 transform.
    // @COPYPASTA: This is the same operation as `write_render_transforms.cpp`
    mat4 transform;
    {
        glm_translate_make(transform,
                           vec3{ static_cast<float_t>(ent_transform->position.x),
                                 static_cast<float_t>(ent_transform->position.y),
                                 static_cast<float_t>(ent_transform->position.z) });
        glm_quat_rotate(transform, const_cast<float_t*>(ent_transform->rotation.raw), transform);
        glm_scale(transform, const_cast<float_t*>(ent_transform->scale.raw));
    }

    // Extract float translation.
    vec3 orig_flt_tra;
    glm_vec3(transform[3], orig_flt_tra);

    // Get camera matrices.
    mat4 proj;
    mat4 view;
    mat4 proj_view;
    camera.fetch_calculated_camera_matrices(proj, view, proj_view);
    proj[1][1] *= -1.0f;  // Fix neg-Y issue.

    // Draw Imguizmo gizmo.
    bool manipulated{ false };
    if (ImGuizmo::Manipulate(&view[0][0],
                             &proj[0][0],
                             ImGuizmo::UNIVERSAL,
                             true ? ImGuizmo::WORLD : ImGuizmo::LOCAL,
                             &transform[0][0]))
    {   // Copy result (@NOTE: This is reverse of the TRS->mat4 operation above).
        vec4 tra;
        mat4 rot;
        vec3 sca;
        glm_decompose(transform, tra, rot, sca);

        out_pos.x = ent_transform->position.x + (tra[0] - orig_flt_tra[0]);
        out_pos.y = ent_transform->position.y + (tra[1] - orig_flt_tra[1]);
        out_pos.z = ent_transform->position.z + (tra[2] - orig_flt_tra[2]);
        glm_mat4_quat(rot, out_rot.raw);
        glm_vec3_copy(sca, out_sca.raw);

        // Mark as manipulated.
        manipulated = true;
    }

    return manipulated;
}

/// Renders gizmo for transforms and updates entity transform if manipulated.
void internal_imguizmo_transform_gizmo()
{
    auto& renderer{ service_finder::find_service<Renderer>() };
    auto& camera{ *renderer.get_camera_obj() };

    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };

    ImGuizmo::Enable(!camera.is_mouse_captured());

    rvec3s  pos;
    versors rot;
    vec3s   sca;
    if (internal_imguizmo_manipulate(reg, camera, pos, rot, sca))
    {
        component::submit_transform_change_helper(reg, s_state.selected_entity, pos, rot, sca);
    }





    // @TODO: IMPLEMENT vv
#if 0
    ImGuizmo::Enable(!m_renderer.get_camera_obj()->is_mouse_captured());

    mat4 proj;
    mat4 view;
    mat4 proj_view;
    m_renderer.get_camera_obj()->fetch_calculated_camera_matrices(proj, view, proj_view);
    proj[1][1] *= -1.0f;  // Fix neg-Y issue.

    mat4 transform_mat;
    m_transform.get_transform_as_mat4(transform_mat);

    vec3 orig_tra;
    glm_vec3(transform_mat[3], orig_tra);

    static ImGuizmo::OPERATION s_current_gizmo_operation{ ImGuizmo::UNIVERSAL };
    ImGuizmo::MODE current_gizmo_mode{ s_imgui_gizmo_trans_space == 0 ?
                                       ImGuizmo::WORLD :
                                       ImGuizmo::LOCAL };

    if (ImGuizmo::Manipulate(&view[0][0],
                             &proj[0][0],
                             s_current_gizmo_operation,
                             current_gizmo_mode,
                             &transform_mat[0][0]))
    {
        vec4 tra;
        mat4 rot;
        vec3 sca;
        glm_decompose(transform_mat, tra, rot, sca);

        // @NOTE: The position vector is already changed in the mat4 matrix, so
        //   here is just manually calculating the world pos delta.
        vec3 delta_tra;
        glm_vec3_sub(tra, orig_tra, delta_tra);

        rvec3  global_pos;
        versor global_rot;
        vec3   global_sca;
        m_transform.get_global_transform_decomposed_data(global_pos, global_rot, global_sca);

        global_pos[0] += delta_tra[0];
        global_pos[1] += delta_tra[1];
        global_pos[2] += delta_tra[2];
        glm_mat4_quat(rot, global_rot);
        glm_vec3_copy(sca, global_sca);

        m_transform.set_global_pos_rot_sca(global_pos, global_rot, global_sca);

        propagate_transform_changes();

        s_imgui_trans_gizmo_used = true;
    }
#endif  // 0
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

        internal_imguizmo_transform_gizmo();
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
