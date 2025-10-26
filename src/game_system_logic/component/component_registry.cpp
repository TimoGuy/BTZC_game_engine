#include "component_registry.h"

#include "service_finder/service_finder.h"
#include "game_system_logic/entity_container.h"
#include "entity_metadata.h"
#include "transform.h"

#include <cassert>
#include <typeindex>
#include <unordered_map>
#include <string>
#include <functional>


namespace
{

static std::unordered_map<std::string, std::type_index> s_type_str_to_type_idx_map;

using Construct_comp_fn = std::function<void(entt::entity, json const&)>;
static std::unordered_map<std::type_index, Construct_comp_fn> s_type_idx_to_construct_comp_fn_map;

}  // namespace


void BT::component::register_all_components()
{   // Ensure this is only called once.
    assert(s_type_str_to_type_idx_map.empty());
    assert(s_type_idx_to_construct_comp_fn_map.empty());

    /// Helpful macro for registering a component.
    #define REGISTER_COMPONENT(_type)                                                               \
        do                                                                                          \
        {                                                                                           \
            auto type_idx{ std::type_index(typeid(_type)) };                                        \
            s_type_str_to_type_idx_map.emplace("Transform", type_idx);                              \
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
        } while(0);

    //---- All Components --------------------------------------------------------------------------
    REGISTER_COMPONENT(component::Entity_metadata);
    REGISTER_COMPONENT(component::Transform);
    REGISTER_COMPONENT(component::Transform_hierarchy);
    //----------------------------------------------------------------------------------------------

    #undef REGISTER_COMPONENT
}

void BT::component::construct_component(entt::entity ecs_entity, std::string const& type_name, json const& members_j)
{   // Run construct component func.
    auto type_idx{ s_type_str_to_type_idx_map.at(type_name) };
    auto& construct_comp_fn{ s_type_idx_to_construct_comp_fn_map.at(type_idx) };
    construct_comp_fn(ecs_entity, members_j);
}
