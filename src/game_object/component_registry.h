////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Registry of component lists
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <type_traits>
#include <typeindex>


namespace BT
{

class Game_object;

namespace component
{

/// List of components for game objects. Automatically gets registered in the `Registry` so that the
/// `Registry` can query for `Component_list`s.
class Component_list
{
public:
    Component_list(Game_object& attached_game_obj);
    ~Component_list();

    Component_list(const Component_list&)            = delete;
    Component_list(Component_list&&)                 = delete;
    Component_list& operator=(const Component_list&) = delete;
    Component_list& operator=(Component_list&&)      = delete;

    /// Adds component with a value.
    template<typename T>
    void add_component(T init_value)
    {
        size_t data_offset{ m_components_datas.size() };
        m_type_to_data_offset_map.emplace(std::type_index(typeid(T)), data_offset);

        m_components_datas.resize(m_components_datas.size() + sizeof(T));
        get_component_handle<T>() = init_value;
    }

    /// Removes component.
    template<typename T>
    void remove_component()
    {   // @TODO: Implement (if needed rly).
        // @NOTE: Good luck w/ fragmentation and stuff! Or maybe you could just shift around the
        //        memory data offsets and stuff.
        assert(false);
    }

    /// Gets a reference to the data of the component.
    template<typename T>
    T& get_component_handle()
    {
        size_t data_offset{ m_type_to_data_offset_map.at(std::type_index(typeid(T))) };
        return *reinterpret_cast<T*>(&m_components_datas[data_offset]);
    }

private:
    Game_object& m_attached_game_obj;

    std::unordered_map<std::type_index, size_t> m_type_to_data_offset_map;
    std::vector<uint8_t> m_components_datas;  // Components are stored as bytes inside here.
};

/// Registers and stores components in the ECS. Used for querying component lists.
class Registry
{
public:
    Registry();
    ~Registry() = default;

    Registry(const Registry&)            = delete;
    Registry(Registry&&)                 = delete;
    Registry& operator=(const Registry&) = delete;
    Registry& operator=(Registry&&)      = delete;

    void register_all_components();

    void add_component_list(Component_list* comp_list);
    void remove_component_list(Component_list* comp_list);
    std::vector<Component_list*> query_component_lists();  // @TODO: figure out the asdfasdfasdf interface.

private:
    // Registered components.
    std::unordered_map<std::string, size_t> m_component_name_to_list_idx_map;

    struct Component_metadata
    {
        size_t component_idx;
        std::type_index typename_id_idx;
    };
    std::vector<Component_metadata> m_components;

    // Registered component lists.
    std::vector<Component_list*> m_added_component_lists;
};

}  // namespace component
}  // namespace BT
