#include "component_registry.h"

#include "../service_finder/service_finder.h"
#include "components.h"
#include "game_object.h"

#include <cassert>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>


BT::component::Component_list::Component_list(Game_object& attached_game_obj)
    : m_attached_game_obj(attached_game_obj)
{
    service_finder::find_service<Registry>().add_component_list(this);
}

BT::component::Component_list::~Component_list()
{
    service_finder::find_service<Registry>().remove_component_list(this);
}


BT::component::Registry::Registry()
{
    BT_SERVICE_FINDER_ADD_SERVICE(Registry, this);
}

void BT::component::Registry::register_all_components()
{   // Make sure that this function only runs once.
    assert(m_components.empty());

    #define REGISTER_COMPONENT(_typename)                                                           \
        do                                                                                          \
        {                                                                                           \
            size_t next_component_idx{ m_components.size() };                                       \
            m_component_name_to_list_idx_map.emplace(#_typename, next_component_idx);               \
                                                                                                    \
            m_components.emplace_back(next_component_idx, std::type_index(typeid(_typename)));      \
        } while (false);

    // ---- List of components to register ---------------------------------------------------------
    REGISTER_COMPONENT(Character_controller_move_delta);
    // ---------------------------------------------------------------------------------------------

    #undef REGISTER_COMPONENT
}

void BT::component::Registry::add_component_list(Component_list* comp_list)
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

void BT::component::Registry::remove_component_list(Component_list* comp_list)
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

std::vector<BT::component::Component_list*> BT::component::Registry::query_component_lists()
{   // @TODO
    assert(false);
}
