#include "component_registry.h"

#include "../service_finder/service_finder.h"
#include "components.h"
#include "game_object.h"
#include "system/system_ifc.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>


BT::component_system::Component_list::Component_list(Game_object& attached_game_obj)
    : m_attached_game_obj(attached_game_obj)
{
    service_finder::find_service<Registry>().add_component_list(this);
}

BT::component_system::Component_list::~Component_list()
{
    service_finder::find_service<Registry>().remove_component_list(this);
}

void BT::component_system::Component_list::commit_change_requests()
{
    // @TODO
    assert(false);
}

bool BT::component_system::Component_list::check_component_exists(
    std::type_index comp_typename_idx) const
{
    return (m_type_to_data_offset_map.find(comp_typename_idx) != m_type_to_data_offset_map.end());
}

void BT::component_system::Component_list::scene_serialize(Scene_serialization_mode mode,
                                                           json& node_ref) /*override*/
{
    auto& registry{ service_finder::find_service<Registry>() };

    if (mode == SCENE_SERIAL_MODE_SERIALIZE)
    {
        node_ref = json::array();
        size_t idx{ 0 };
        for (auto it : m_type_to_data_offset_map)
        {   // Find string of component type id.
            auto comp_typename{ registry.find_typename_str_by_component_typeid(it.first) };

            node_ref[idx]["typename"] = comp_typename;

            idx++;
        }
    }
    else if (mode == SCENE_SERIAL_MODE_DESERIALIZE)
    {
        bool is_valid_array{ !node_ref.is_null() && node_ref.is_array() };
        if (!is_valid_array)
        {   // Error: list of components is not a list.
            assert(false);
            return;
        }
        for (auto component_json : node_ref)
        {   // Run default add component func.
            registry.find_component_metadata_by_typename_str(component_json["typename"])
                .default_add_component_fn(*this);
        }
    }

    // @TODO
    assert(false);
}


BT::component_system::Registry::Registry()
{
    BT_SERVICE_FINDER_ADD_SERVICE(Registry, this);
}

void BT::component_system::Registry::register_all_components()
{   // Make sure that this function only runs once.
    assert(m_components.empty());

    #define REGISTER_COMPONENT(_typename)                                                           \
        do                                                                                          \
        {                                                                                           \
            size_t next_component_idx{ m_components.size() };                                       \
            m_component_name_to_list_idx_map.emplace(#_typename, next_component_idx);               \
                                                                                                    \
            m_components.emplace_back(next_component_idx,                                           \
                                      std::type_index(typeid(_typename)),                           \
                                      [](Component_list& comp_list) {                               \
                                          comp_list.add_component<_typename>(_typename{});          \
                                      },                                                            \
                                      [](Component_list& comp_list) {                               \
                                          comp_list.remove_component<_typename>();                  \
                                      });                                                           \
        } while (false);

    // ---- List of components to register ---------------------------------------------------------
    REGISTER_COMPONENT(Component_model_animator);
    REGISTER_COMPONENT(Component_hitcapsule_group_set);
    REGISTER_COMPONENT(Component_physics_object);
    REGISTER_COMPONENT(Component_char_con_movement_state);
    REGISTER_COMPONENT(Component_anim_editor_tool_communicator_state);
    // ---------------------------------------------------------------------------------------------

    #undef REGISTER_COMPONENT
}

BT::component_system::Registry::Component_metadata const& BT::component_system::Registry::
    find_component_metadata_by_typename_str(std::string const& typename_str) const
{
    return m_components[m_component_name_to_list_idx_map.at(typename_str)];
}

std::string BT::component_system::Registry::find_typename_str_by_component_typeid(
    std::type_index comp_typeid_idx) const
{   // @NOTE: this is a lot more inefficient then the other direction but hopefully this just
    // doesn't get called too too often eh!
    for (auto& comp : m_components)
        if (comp.typename_id_idx == comp_typeid_idx)
            for (auto it : m_component_name_to_list_idx_map)
                if (it.second == comp.component_idx)
                {   // Found string equivalent!
                    return it.first;
                }

    // Failed to find string equivalent of typeid idx.
    assert(false);
    return "";
}

void BT::component_system::Registry::add_component_list(Component_list* comp_list)
{
    if (std::find(m_added_component_lists.begin(), m_added_component_lists.end(), comp_list) ==
        m_added_component_lists.end())
    {   // Add component list.
        m_added_component_lists.emplace_back(comp_list);
    }
    else
    {   // Error: component list already added.
        assert(false);
    }
}

void BT::component_system::Registry::remove_component_list(Component_list* comp_list)
{
    auto erase_it{ std::find(m_added_component_lists.begin(),
                             m_added_component_lists.end(),
                             comp_list) };
    if (erase_it == m_added_component_lists.end())
    {   // Remove component list.
        m_added_component_lists.erase(erase_it);
    }
    else
    {  // Error: component list did not exist (or perhaps `comp_list` changed for some reason bc of
       // reallocation?).
        assert(false);
    }
}

std::vector<BT::component_system::Component_list*>
BT::component_system::Registry::query_component_lists(Component_list_query const& query)
{   // Grab list of component lists that match the query.
    // @TODO: @IDEA: @MAYBE: Make a cache system for the lists these queries find.
    std::vector<Component_list*> comp_lists;
    for (auto comp_list : m_added_component_lists)
        if (query.query_component_list_match(*comp_list))
        {
            comp_lists.emplace_back(comp_list);
        }

    return comp_lists;
}
