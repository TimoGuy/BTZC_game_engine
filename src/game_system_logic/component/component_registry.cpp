#include "component_registry.h"

#include "_dev_animation_frame_action_editor_agent.h"
#include "anim_frame_action_controller.h"
#include "animator_root_motion.h"
#include "btjson.h"
#include "character_movement.h"
#include "combat_stats.h"
#include "component_imgui_edit_functions.h"
#include "entity_metadata.h"
#include "entt/core/fwd.hpp"
#include "entt/core/type_info.hpp"
#include "entt/entity/registry.hpp"
#include "follow_camera.h"
#include "game_system_logic/component/animator_driven_hitcapsule_set.h"
#include "game_system_logic/entity_container.h"
#include "health_stats.h"
#include "physics_object_settings.h"
#include "render_object_settings.h"
#include "service_finder/service_finder.h"
#include "transform.h"

#include <cassert>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <typeindex>
#include <unordered_map>


namespace
{

static std::unordered_map<std::string, std::type_index> s_type_str_to_type_idx_map;

using Construct_comp_fn = std::function<void(entt::entity, json const&)>;
static std::unordered_map<std::type_index, Construct_comp_fn> s_type_idx_to_construct_comp_fn_map;

struct Component_metadata
{
    size_t display_order;
    std::string comp_name;
    std::function<void(entt::registry&, entt::entity)> imgui_edit_fn;
    std::function<json(entt::registry&, entt::entity)> serialize_component_fn;
};
static std::unordered_map<entt::id_type, Component_metadata> s_entt_type_id_to_comp_meta_map;

}  // namespace


void BT::component::register_all_components()
{   // Ensure this is only called once.
    assert(s_type_str_to_type_idx_map.empty());
    assert(s_type_idx_to_construct_comp_fn_map.empty());
    assert(s_entt_type_id_to_comp_meta_map.empty());

    /// Helpful macro for registering a component.
    #define REGISTER_COMPONENT__YES_SERIALIZE(_type, _imgui_edit_fn)                                \
        do                                                                                          \
        {                                                                                           \
            /* Register in serialization func map. */                                               \
            auto type_idx{ std::type_index(typeid(_type)) };                                        \
            s_type_str_to_type_idx_map.emplace(#_type, type_idx);                                   \
            s_type_idx_to_construct_comp_fn_map.emplace(type_idx,                                   \
                                                        [](entt::entity ecs_entity,                 \
                                                           json const& members_j) {                 \
                /* Add component to entity. */                                                      \
                auto& entity_container{ service_finder::find_service<Entity_container>() };         \
                                                                                                    \
                auto& new_comp{                                                                     \
                    entity_container.get_ecs_registry().emplace<_type>(ecs_entity) };               \
                new_comp = _type(members_j);                                                        \
            });                                                                                     \
                                                                                                    \
            /* Add component to hash-to-component-name map. */                                      \
            s_entt_type_id_to_comp_meta_map.emplace(                                                \
                entt::type_id<_type>().hash(),                                                      \
                Component_metadata{ s_entt_type_id_to_comp_meta_map.size(),                         \
                                    #_type,                                                         \
                                    _imgui_edit_fn,                                                 \
                                    [](entt::registry& reg, entt::entity ecs_entity) {              \
                                        return reg.get<_type>(ecs_entity);                          \
                                    } });                                                           \
        } while(0);

    #define REGISTER_COMPONENT___NO_SERIALIZE(_type, _imgui_edit_fn)                                \
        do                                                                                          \
        {                                                                                           \
            /* Add component to hash-to-component-name map. */                                      \
            s_entt_type_id_to_comp_meta_map.emplace(                                                \
                entt::type_id<_type>().hash(),                                                      \
                Component_metadata{ s_entt_type_id_to_comp_meta_map.size(),                         \
                                    #_type,                                                         \
                                    _imgui_edit_fn,                                                 \
                                    nullptr });                                                     \
        } while(0);

    //---- All Components -- @NOTE: Order matters here! --------------------------------------------
    //----------------                Component typename                                      ImGui edit func
 // REGISTER_COMPONENT__YES_SERIALIZE(component::Some_sample_component,                       edit::imgui_edit__sample);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Entity_metadata,                             edit::imgui_edit__entity_metadata);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Transform,                                   edit::imgui_edit__transform);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Transform_hierarchy,                         edit::imgui_edit__transform_hierarchy);
    REGISTER_COMPONENT___NO_SERIALIZE(component::Transform_changed,                           edit::imgui_edit__transform_changed);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Player_character,                            edit::imgui_edit__sample);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Character_world_space_input,                 edit::imgui_edit__character_world_space_input);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Character_mvt_state,                         edit::imgui_edit__sample);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Display_repr_transform_ref,                  edit::imgui_edit__sample);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Follow_camera_follow_ref,                    edit::imgui_edit__sample);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Render_object_settings,                      edit::imgui_edit__render_object_settings);
    REGISTER_COMPONENT___NO_SERIALIZE(component::Created_render_object_reference,             edit::imgui_edit__created_render_object_reference);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Character_mvt_animated_state,                edit::imgui_edit__sample);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Anim_frame_action_controller,                edit::imgui_edit__sample);
    REGISTER_COMPONENT___NO_SERIALIZE(component::Animator_driven_hitcapsule_set,              edit::imgui_edit__sample);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Animator_root_motion,                        edit::imgui_edit__sample);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Physics_object_settings,                     edit::imgui_edit__physics_object_settings);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Physics_obj_type_triangle_mesh_settings,     edit::imgui_edit__physics_obj_type_triangle_mesh_settings);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Physics_obj_type_char_con_settings,          edit::imgui_edit__physics_obj_type_char_con_settings);
    REGISTER_COMPONENT___NO_SERIALIZE(component::Created_physics_object_reference,            edit::imgui_edit__created_physics_object_reference);
    REGISTER_COMPONENT__YES_SERIALIZE(component::_Dev_animation_frame_action_editor_agent,    edit::imgui_edit__sample);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Health_stats_data,                           edit::imgui_edit__health_stats_data);
    REGISTER_COMPONENT__YES_SERIALIZE(component::Base_combat_stats_data,                      edit::imgui_edit__base_combat_stats_data);
    //----------------------------------------------------------------------------------------------

    #undef REGISTER_COMPONENT
}

void BT::component::construct_component(entt::entity ecs_entity,
                                        std::string const& type_name,
                                        json const& members_j)
{   // Run construct component func.
    auto type_idx{ s_type_str_to_type_idx_map.at(type_name) };
    auto& construct_comp_fn{ s_type_idx_to_construct_comp_fn_map.at(type_idx) };
    construct_comp_fn(ecs_entity, members_j);
}

std::string BT::component::find_component_name(entt::id_type comp_id)
{
    return s_entt_type_id_to_comp_meta_map.at(comp_id).comp_name;
}

std::optional<json> BT::component::serialize_component_of_entity(entt::entity ecs_entity,
                                                                 entt::id_type comp_id)
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto& serialize_component_fn{
        s_entt_type_id_to_comp_meta_map.at(comp_id).serialize_component_fn
    };

    if (serialize_component_fn == nullptr)
    {   // No serialization function found.
        return std::nullopt;
    }

    return serialize_component_fn(reg, ecs_entity);
}

void BT::component::imgui_render_components_edit_panes(entt::entity ecs_entity)
{   // Figure out what components appear.
    std::map<size_t, Component_metadata> found_comps_ordered;

    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };

    // Reference: https://github.com/skypjack/entt/issues/88#issuecomment-1276858022

    for (auto&& curr : reg.storage())
        if (curr.second.contains(ecs_entity))
        {   // Found a component.
            entt::id_type comp_id{ curr.first };

            auto comp_meta_copy{ s_entt_type_id_to_comp_meta_map.at(comp_id) };
            found_comps_ordered.emplace(comp_meta_copy.display_order, std::move(comp_meta_copy));
        }

    // Run ImGui edit funcs in desired order (i.e. the order listed in the .h file).
    for (auto&& it : found_comps_ordered)
    {
        auto& comp_meta{ it.second };
        if (edit::internal::imgui_open_component_editing_header(comp_meta.comp_name))
        {
            edit::internal::imgui_blank_space(8);

            comp_meta.imgui_edit_fn(reg, ecs_entity);

            edit::internal::imgui_blank_space(16);
            edit::internal::imgui_tree_pop();
        }
    }
}
