#include "component_imgui_edit_functions.h"

#include "btglm.h"
#include "character_movement.h"
#include "combat_stats.h"
#include "entity_metadata.h"
#include "game_system_logic/component/animator_root_motion.h"
#include "game_system_logic/entity_container.h"
#include "health_stats.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "misc/cpp/imgui_stdlib.h"
#include "physics_engine/physics_object.h"
#include "physics_object_settings.h"
#include "render_object_settings.h"
#include "renderer/render_layer.h"
#include "renderer/renderer.h"
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


namespace
{

/// Color for warning yellow.
constexpr ImVec4 k_color_warning(0.75, 0.75, 0.22, 1);

/// Converts any number to its binary representation as a string.
std::string convert_number_to_binary_bit_string(uint32_t str_len, auto number)
{
    std::string bit_str(static_cast<size_t>(str_len), ' ');

    size_t current_bit_mask{ 0x01 };
    for (int64_t i = str_len - 1; i >= 0; i--)
    {
        bit_str[i] = (number & current_bit_mask ? '1' : '0');
        current_bit_mask = (current_bit_mask << 1);
    }

    return bit_str;
}

}  // namespace


// ImGui edit functions.
void BT::component::edit::imgui_edit__sample(entt::registry& reg, entt::entity ecs_entity)
{
    auto const& meta{ reg.get<component::Entity_metadata const>(ecs_entity) };

    ImGui::PushID(&meta);
    ImGui::PushItemWidth(ImGui::GetFontSize() * -10);

    ImGui::Text("Sample edit view! For entity %u", ecs_entity);

    ImGui::PopItemWidth();
    ImGui::PopID();
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

    float_t padding{ 16 * ImGui::GetIO().FontGlobalScale };

    ImGui::PushItemWidth((ImGui::GetContentRegionAvail().x * 0.5f) + (ImGui::GetFontSize() * -10));

    // Select transform editing space.
    static int32_t s_transform_type{ 0 };  // 0: Global
                                           // 1: Local
    ImGui::Combo("Transform Space", &s_transform_type, "Global\0Local\0");
    ImGui::SameLine(0, padding);

    // Select rotation edit type.
    static int32_t s_rotation_type{ 0 };  // 0: Euler
                                          // 1: Quaternion
    ImGui::Combo("Rotation type", &s_rotation_type, "Euler in degrees\0Quaternion XYZW\0");

    ImGui::PopItemWidth();

    // Display transform.
    ImGui::SeparatorText("Transform as TRS");

    bool changed{ false };
    auto transform_copy{ transform };

    ImGui::PushItemWidth(ImGui::GetFontSize() * -10);

    // Translation.
    changed |= ImGui::DragScalarN("Position", ImGuiDataType_Double, transform_copy.position.raw, 3, 0.1f);

    // Rotation.
    if (s_rotation_type == 0)
    {
        // @INCOMPLETE: @TODO: Make this actually use euler angles (in degrees).
        // See `game_object.cpp:522` for a good example (since euler angles are unstable when doing multiple calcs).
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

    // Scale.
    // @TODO: Ensure that scale does not get <0.001 abs val.
    changed |= ImGui::DragFloat3("Scale", transform_copy.scale.raw, 0.01f);

    ImGui::PopItemWidth();

    if (changed)
    {   // Assign new transform as the form of a transform change submission.
        component::submit_transform_change_helper(reg,
                                                  ecs_entity,
                                                  transform_copy.position,
                                                  transform_copy.rotation,
                                                  transform_copy.scale);

        // Try to assign the transform to physics object in case there is a physics object.
        component::try_set_physics_object_transform_helper(reg,
                                                           ecs_entity,
                                                           transform_copy.position,
                                                           transform_copy.rotation);
    }

    ImGui::PopID();
}

void BT::component::edit::imgui_edit__transform_hierarchy(entt::registry& reg,
                                                          entt::entity ecs_entity)
{
    auto const& hierarchy{ reg.get<component::Transform_hierarchy const>(ecs_entity) };

    ImGui::PushID(&hierarchy);

    // Parent.
    if (hierarchy.parent_entity.is_nil())
        ImGui::TextColored(k_color_warning,
                           "%s",
                           "No parent (i.e. this is a root transform).");
    else
        ImGui::Text("Parent: %s", UUID_helper::to_pretty_repr(hierarchy.parent_entity).c_str());

    // Children.
    ImGui::SeparatorText("Children");

    if (hierarchy.children_entities.empty())
    {
        ImGui::TextColored(k_color_warning, "%s", "No children.");
    }
    else
    {
        for (auto const& child_uuid : hierarchy.children_entities)
        {
            ImGui::BulletText("%s", UUID_helper::to_pretty_repr(child_uuid).c_str());
        }
    }

    ImGui::PopID();
}

void BT::component::edit::imgui_edit__transform_changed(entt::registry& reg,
                                                        entt::entity ecs_entity)
{
    auto const& trans_changed{ reg.get<component::Transform_changed const>(ecs_entity) };

    ImGui::PushID(&trans_changed);

    ImGui::Text("Next transform request:");

    auto& next_pos{ trans_changed.next_transform.position };
    auto& next_rot{ trans_changed.next_transform.rotation };
    auto& next_sca{ trans_changed.next_transform.scale };
    ImGui::Text("  Pos : (%0.6f, %0.6f, %0.6f)",        next_pos.x, next_pos.y, next_pos.z);
    ImGui::Text("  Rot : (%0.3f, %0.3f, %0.3f, %0.3f)", next_rot.x, next_rot.y, next_rot.z, next_rot.w);
    ImGui::Text("  Pos : (%0.3f, %0.3f, %0.3f)",        next_sca.x, next_sca.y, next_sca.z);

    ImGui::PopID();
}

void BT::component::edit::imgui_edit__character_world_space_input(entt::registry& reg,
                                                                  entt::entity ecs_entity)
{
    auto& char_ws_input{ reg.get<component::Character_world_space_input>(ecs_entity) };

    ImGui::PushID(&char_ws_input);

    if (ImGui::DragFloat3("Flat normalized input", char_ws_input.ws_flat_clamped_input.raw, 0.05f))
    {   // Flatten.
        char_ws_input.ws_flat_clamped_input.y = 0;

        // Clamp magnitude to <=1.0
        if (glm_vec3_norm2(char_ws_input.ws_flat_clamped_input.raw) > 1.0f * 1.0f)
            glm_vec3_normalize(char_ws_input.ws_flat_clamped_input.raw);
    }

    ImGui::Checkbox("jump_pressed",        &char_ws_input.jump_pressed);
    ImGui::Checkbox("prev_jump_pressed",   &char_ws_input.prev_jump_pressed);
    ImGui::Checkbox("crouch_pressed",      &char_ws_input.crouch_pressed);
    ImGui::Checkbox("prev_crouch_pressed", &char_ws_input.prev_crouch_pressed);

    ImGui::PopID();
}

void BT::component::edit::imgui_edit__render_object_settings(entt::registry& reg,
                                                             entt::entity ecs_entity)
{
    auto& rend_obj_settings{ reg.get<component::Render_object_settings>(ecs_entity) };

    ImGui::PushID(&rend_obj_settings);
    ImGui::PushItemWidth(ImGui::GetFontSize() * -10);

    bool is_disabled{ reg.any_of<component::Created_render_object_reference>(ecs_entity) };
    if (is_disabled)
        ImGui::TextColored(k_color_warning,
                           "Settings are disabled while a render object is created.");

    // Settings.
    ImGui::BeginDisabled(is_disabled);

    static std::vector<std::pair<std::string, Render_layer>> const s_layers{
        { "Default",      RENDER_LAYER_DEFAULT      },
        { "Invisible",    RENDER_LAYER_INVISIBLE    },
        { "Level editor", RENDER_LAYER_LEVEL_EDITOR },
    };

    // @NOTE: I just realized that this isn't applicable for the application (since I only want
    //        one render layer for the one render object).
    #if 0
    ImGui::Text("Render layer mask: %s",
                convert_number_to_binary_bit_string(8, rend_obj_settings.render_layer).c_str());
    ImGui::SameLine();
    if (ImGui::Button("Change.."))
        ImGui::OpenPopup("change_layer_mask");
    if (ImGui::BeginPopup("change_layer_mask"))
    {

        for (auto& [layer_str, layer_mask] : s_layers)
        {   // Checkbox for layer.
            bool layer_enabled{ (rend_obj_settings.render_layer & layer_mask) != 0 };
            if (ImGui::Checkbox(layer_str.c_str(), &layer_enabled))
            {
                if (layer_enabled)
                {
                    rend_obj_settings.render_layer =
                        Render_layer(rend_obj_settings.render_layer | layer_mask);
                }
                else
                {
                    rend_obj_settings.render_layer =
                        Render_layer(rend_obj_settings.render_layer & ~layer_mask);
                }
            }
        }

        ImGui::EndPopup();
    }
    #endif  // 0

    std::string current_layer_str{ "INVALID LAYER" };

    for (auto const& [layer_str, layer_mask] : s_layers)
        if (rend_obj_settings.render_layer == layer_mask)
            current_layer_str = layer_str;

    if (ImGui::BeginCombo("Render layer", current_layer_str.c_str()))
    {
        for (auto const& [layer_str, layer_mask] : s_layers)
        {   // Combo selectable item.
            bool const is_selected{ rend_obj_settings.render_layer == layer_mask };
            if (ImGui::Selectable(layer_str.c_str(), is_selected))
                rend_obj_settings.render_layer = layer_mask;

            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }

    ImGui::InputText("Model name", &rend_obj_settings.model_name);
    ImGui::Checkbox("Is deformed", &rend_obj_settings.is_deformed);

    ImGui::BeginDisabled(!rend_obj_settings.is_deformed);
    ImGui::InputText("Animator template name", &rend_obj_settings.animator_template_name);
    ImGui::EndDisabled();

    ImGui::EndDisabled();

    ImGui::PopItemWidth();
    ImGui::PopID();
}

void BT::component::edit::imgui_edit__created_render_object_reference(entt::registry& reg,
                                                                      entt::entity ecs_entity)
{
    auto const& rend_obj_ref{ reg.get<component::Created_render_object_reference const>(
        ecs_entity) };

    ImGui::PushID(&rend_obj_ref);

    ImGui::TextWrapped("A render object is created in the renderer.\n  UUID: %s",
                       UUID_helper::to_pretty_repr(rend_obj_ref.render_obj_uuid_ref).c_str());

    // Extras for if there's an animator.
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };
    auto& rend_obj{
        *rend_obj_pool.checkout_render_obj_by_key({ rend_obj_ref.render_obj_uuid_ref }).front()
    };

    if (auto animator{ rend_obj.get_model_animator() }; animator != nullptr)
    {   // EXTRAS!!
        // Control the state machine!!
        ImGui::SeparatorText("Extras: animator controls");

        ImGui::Text("is_using_root_motion: %s",
                    (animator->get_is_using_root_motion() ? "TRUE" : "FALSE"));

        for (size_t var_idx = 0; var_idx < animator->get_num_animator_variables(); var_idx++)
        {
            auto const& anim_var{ animator->get_animator_variable(var_idx) };
            switch (anim_var.type)
            {
            case anim_tmpl_types::Animator_variable::TYPE_BOOL:
            {
                bool var_val{ glm_eq(anim_var.var_value, anim_tmpl_types::k_bool_true) };
                if (ImGui::Checkbox(anim_var.var_name.c_str(), &var_val))
                {
                    animator->set_bool_variable(anim_var.var_name, var_val);
                }
                break;
            }

            case anim_tmpl_types::Animator_variable::TYPE_INT:
            {
                int32_t var_val{ static_cast<int32_t>(anim_var.var_value) };
                if (ImGui::InputInt(anim_var.var_name.c_str(), &var_val))
                {
                    animator->set_int_variable(anim_var.var_name, var_val);
                }
                break;
            }

            case anim_tmpl_types::Animator_variable::TYPE_FLOAT:
            {
                float_t var_val{ anim_var.var_value };
                if (ImGui::DragFloat(anim_var.var_name.c_str(), &var_val, 0.1f))
                {
                    animator->set_float_variable(anim_var.var_name, var_val);
                }
                break;
            }

            case anim_tmpl_types::Animator_variable::TYPE_TRIGGER:
                if (ImGui::Button(("Trigger \"" + anim_var.var_name + "\"").c_str()))
                {
                    animator->set_trigger_variable(anim_var.var_name);
                }
                break;

            default: assert(false); break;
            }
        }
    }

    rend_obj_pool.return_render_objs({ &rend_obj });

    ImGui::PopID();
}

void BT::component::edit::imgui_edit__animator_root_motion(entt::registry& reg,
                                                           entt::entity ecs_entity)
{
    auto& anim_root_motion{ reg.get<component::Animator_root_motion>(ecs_entity) };

    ImGui::PushID(&anim_root_motion);
    ImGui::PushItemWidth(ImGui::GetFontSize() * -10);

    // Show vals.
    ImGui::DragFloat("root_motion_multiplier", &anim_root_motion.root_motion_multiplier);
    ImGui::DragFloat3("delta_pos", anim_root_motion.delta_pos);
    ImGui::Text("%.9f", anim_root_motion.delta_pos[2]);

    ImGui::PopItemWidth();
    ImGui::PopID();
}

void BT::component::edit::imgui_edit__physics_object_settings(entt::registry& reg,
                                                              entt::entity ecs_entity)
{
    auto& phys_obj_settings{ reg.get<component::Physics_object_settings>(ecs_entity) };

    ImGui::PushID(&phys_obj_settings);
    ImGui::PushItemWidth(ImGui::GetFontSize() * -10);

    bool is_disabled{ reg.any_of<component::Created_physics_object_reference>(ecs_entity) };
    if (is_disabled)
        ImGui::TextColored(k_color_warning,
                           "Settings are disabled while a physics object is created.");

    static std::vector<std::pair<std::string, Physics_object_type>> const s_phys_obj_types{
        { "Triangle mesh",        PHYSICS_OBJECT_TYPE_TRIANGLE_MESH        },
        { "Character controller", PHYSICS_OBJECT_TYPE_CHARACTER_CONTROLLER },
    };

    int32_t selected_idx{ -1 };
    std::string combined_str;
    {
        size_t i{ 0 };
        for (auto& [type_str, phys_type] : s_phys_obj_types)
        {
            combined_str += (type_str + '\0');
            if (phys_obj_settings.phys_obj_type == phys_type)
                selected_idx = i;
            i++;
        }
    }

    ImGui::BeginDisabled(is_disabled);
    if (ImGui::Combo("Physics obj type", &selected_idx, combined_str.c_str()))
    {
        phys_obj_settings.phys_obj_type = s_phys_obj_types[selected_idx].second;
    }
    ImGui::EndDisabled();

    ImGui::PopItemWidth();
    ImGui::PopID();
}

void BT::component::edit::imgui_edit__physics_obj_type_triangle_mesh_settings(
    entt::registry& reg,
    entt::entity ecs_entity)
{
    auto& tri_mesh_settings{ reg.get<component::Physics_obj_type_triangle_mesh_settings>(
        ecs_entity) };

    ImGui::PushID(&tri_mesh_settings);
    ImGui::PushItemWidth(ImGui::GetFontSize() * -10);

    bool is_disabled{ reg.any_of<component::Created_physics_object_reference>(ecs_entity) };
    if (is_disabled)
        ImGui::TextColored(k_color_warning,
                           "Settings are disabled while a physics object is created.");

    ImGui::BeginDisabled(is_disabled);

    ImGui::InputText("Model name", &tri_mesh_settings.model_name);
    ImGui::Text("Sample edit view! For entity %u", ecs_entity);

    ImGui::EndDisabled();

    ImGui::PopItemWidth();
    ImGui::PopID();
}

void BT::component::edit::imgui_edit__physics_obj_type_char_con_settings(entt::registry& reg,
                                                                         entt::entity ecs_entity)
{
    auto& char_con_settings{ reg.get<component::Physics_obj_type_char_con_settings>(ecs_entity) };

    ImGui::PushID(&char_con_settings);
    ImGui::PushItemWidth(ImGui::GetFontSize() * -10);

    bool is_disabled{ reg.any_of<component::Created_physics_object_reference>(ecs_entity) };
    if (is_disabled)
        ImGui::TextColored(k_color_warning,
                           "Settings are disabled while a physics object is created.");

    ImGui::BeginDisabled(is_disabled);

    ImGui::DragFloat("radius", &char_con_settings.radius, 0.05f);
    ImGui::DragFloat("height", &char_con_settings.height, 0.05f);
    ImGui::DragFloat("crouch height", &char_con_settings.crouch_height, 0.05f);

    ImGui::EndDisabled();

    ImGui::PopItemWidth();
    ImGui::PopID();
}

void BT::component::edit::imgui_edit__created_physics_object_reference(entt::registry& reg,
                                                                       entt::entity ecs_entity)
{
    auto const& phys_obj_ref{ reg.get<component::Created_physics_object_reference const>(
        ecs_entity) };

    ImGui::PushID(&phys_obj_ref);

    ImGui::TextWrapped("A physics object is created in the physics engine.\n  UUID: %s",
                       UUID_helper::to_pretty_repr(phys_obj_ref.physics_obj_uuid_ref).c_str());

    ImGui::PopID();
}

void BT::component::edit::imgui_edit__health_stats_data(entt::registry& reg,
                                                        entt::entity ecs_entity)
{
    auto& health_stats_data{ reg.get<component::Health_stats_data>(ecs_entity) };

    ImGui::PushID(&health_stats_data);
    ImGui::PushItemWidth(ImGui::GetFontSize() * -20);

    ImGui::SeparatorText("Health");
    ImGui::InputInt("max_health_pts", &health_stats_data.max_health_pts);
    ImGui::InputInt("health_pts", &health_stats_data.health_pts);

    ImGui::SeparatorText("Posture");
    ImGui::InputInt("max_posture_pts", &health_stats_data.max_posture_pts);
    ImGui::InputInt("posture_pts", &health_stats_data.posture_pts);
    ImGui::DragFloat("posture_pts_regen_rate", &health_stats_data.posture_pts_regen_rate, 0.1f);

    ImGui::SeparatorText("Other");

    ImGui::Checkbox("is_invincible", &health_stats_data.is_invincible);

    // atk_receive_debounce_time.
    float_t atk_receive_debounce_time_f = health_stats_data.atk_receive_debounce_time;
    if (ImGui::DragFloat("atk_receive_debounce_time", &atk_receive_debounce_time_f, 0.01f))
        health_stats_data.atk_receive_debounce_time = atk_receive_debounce_time_f;

    // Grayed out prev attack received time.
    ImGui::BeginDisabled();
    float_t prev_atk_rece_time_f = health_stats_data.prev_atk_received_time;
    ImGui::InputFloat("prev_atk_received_time", &prev_atk_rece_time_f);
    ImGui::EndDisabled();

    ImGui::PopItemWidth();
    ImGui::PopID();
}

void BT::component::edit::imgui_edit__base_combat_stats_data(entt::registry& reg,
                                                             entt::entity ecs_entity)
{
    auto& combat_stats_data{ reg.get<component::Base_combat_stats_data>(ecs_entity) };

    ImGui::PushID(&combat_stats_data);
    ImGui::PushItemWidth(ImGui::GetFontSize() * -20);

    ImGui::SeparatorText("Health");
    ImGui::InputInt("dmg_pts", &combat_stats_data.dmg_pts);
    ImGui::InputInt("dmg_def_pts", &combat_stats_data.dmg_def_pts);

    ImGui::SeparatorText("Posture");
    ImGui::InputInt("posture_dmg_pts", &combat_stats_data.posture_dmg_pts);
    ImGui::InputInt("posture_dmg_def_pts", &combat_stats_data.posture_dmg_def_pts);

    ImGui::PopItemWidth();
    ImGui::PopID();
}
