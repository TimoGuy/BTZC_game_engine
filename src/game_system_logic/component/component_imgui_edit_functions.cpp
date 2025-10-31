#include "component_imgui_edit_functions.h"

#include "entity_metadata.h"
#include "game_system_logic/component/entity_metadata.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"
#include "service_finder/service_finder.h"
#include "transform.h"
#include "uuid/uuid.h"


bool BT::component::edit::internal::imgui_open_component_editing_header(std::string const& label)
{
    return ImGui::TreeNodeEx(label.c_str(),
                             ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                                 ImGuiTreeNodeFlags_NoAutoOpenOnLog);
}

void BT::component::edit::internal::imgui_tree_pop()
{
    ImGui::TreePop();
}

void BT::component::edit::internal::imgui_blank_space(float_t height)
{
    static size_t s_id_gen{ 676767 };

    ImGui::PushID(reinterpret_cast<void*>(s_id_gen));
    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, height * ImGui::GetIO().FontGlobalScale);
    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0);
    ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0x00000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0x00000000);

    ImGui::ButtonEx("", ImVec2(0.000001f, 0.000001f), ImGuiButtonFlags_NoNavFocus);

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar(2);
    ImGui::PopID();

    s_id_gen++;
}

void BT::component::edit::internal::imgui_separator()
{
    ImGui::Separator();
}


// ImGui edit functions.
void BT::component::edit::imgui_edit__sample(entt::registry&, entt::entity ecs_entity)
{
    ImGui::Text("Sample edit view! For entity %u", ecs_entity);
}

void BT::component::edit::imgui_edit__entity_metadata(entt::registry& reg, entt::entity ecs_entity)
{
    auto& meta{ reg.get<component::Entity_metadata>(ecs_entity) };

    ImGui::PushID(&meta);

    float_t padding{ 8 * ImGui::GetIO().FontGlobalScale };
    auto ecs_id{ service_finder::find_service<Entity_container>().find_entity(meta.uuid) };

    ImGui::InputText("Name", &meta.name);

    ImGui::Separator();

    ImGui::Text("UUID   : %s", UUID_helper::to_pretty_repr(meta.uuid).c_str());    
    ImGui::Text("ECS ID : %u", ecs_id);

    if (ImGui::Button("Copy UUID"))
    {
        ImGui::LogToClipboard();
        ImGui::LogText("%s", UUID_helper::to_pretty_repr(meta.uuid).c_str());
        ImGui::LogFinish();
    }

    ImGui::SameLine(0, padding);

    if (ImGui::Button("Copy ECS ID"))
    {
        ImGui::LogToClipboard();
        ImGui::LogText("%u", ecs_id);
        ImGui::LogFinish();
    }

    ImGui::PopID();
}

void BT::component::edit::imgui_edit__transform(entt::registry& reg, entt::entity ecs_entity)
{
    auto const& transform{ reg.get<component::Transform const>(ecs_entity) };

    ImGui::PushID(&transform);

    float_t padding{ 8 * ImGui::GetIO().FontGlobalScale };

    // Select transform editing space.
    static int32_t s_transform_type{ 0 };  // 0: Global
                                           // 1: Local
    ImGui::BeginGroup();
    {
        ImGui::SeparatorText("Transform Space");
        if (ImGui::RadioButton("Global", s_transform_type == 0)) s_transform_type = 0;
        if (ImGui::RadioButton("Local",  s_transform_type == 1)) s_transform_type = 1;
    }
    ImGui::EndGroup();
    ImGui::SameLine(0, padding);

    // Select rotation edit type.
    static int32_t s_rotation_type{ 0 };  // 0: Euler
                                          // 1: Quaternion
    ImGui::BeginGroup();
    {
        ImGui::SeparatorText("Rotation type");
        if (ImGui::RadioButton("Euler in degrees", s_rotation_type == 0)) s_rotation_type = 0;
        if (ImGui::RadioButton("Quaternion XYZW",  s_rotation_type == 1)) s_rotation_type = 1;
    }
    ImGui::EndGroup();

    // Display transform.
    ImGui::SeparatorText("Transform as TRS");

    bool changed{ false };
    auto transform_copy{ transform };

    changed |= ImGui::DragScalarN("Position", ImGuiDataType_Double, transform_copy.position.raw, 3, 0.1f);

    if (s_rotation_type == 0)
    {
        // @INCOMPLETE: @TODO: Make this actually use euler angles (in degrees).
        changed |= ImGui::DragFloat3("Rotation (Euler)", transform_copy.rotation.raw, 0.1f);
    }
    else if (s_rotation_type == 1)
    {   // Quaternion editing.
        changed |= ImGui::DragFloat4("Rotation (XYZW)", transform_copy.rotation.raw, 0.01f);
    }
    else
    {   // Ummm what?? How did you get here?
        assert(false);
    }

    changed |= ImGui::DragFloat3("Scale", transform_copy.scale.raw, 0.01f);

    if (changed)
    {   // Assign new transform as the form of a transform change submission.
        component::submit_transform_change_helper(reg,
                                                  ecs_entity,
                                                  transform_copy.position,
                                                  transform_copy.rotation,
                                                  transform_copy.scale);
    }

    ImGui::PopID();
}
