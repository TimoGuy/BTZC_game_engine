////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Registry of component lists so that component lists can get queried. A component list is
///        a list of components packed into a byte list.
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "refactor_to_entt.h"
#if !BTZC_REFACTOR_TO_ENTT

#include "../scene/scene_serialization_ifc.h"
#include "nlohmann/detail/macro_scope.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>


namespace BT
{

class Game_object;

namespace component_system
{

/// List of components for game objects. Automatically gets registered in the `Registry` so that the
/// `Registry` can query for `Component_list`s.
class Component_list // : public Scene_serialization_ifc
{
public:
    Component_list();
    // Component_list(Game_object& attached_game_obj);
    ~Component_list();

    Component_list(const Component_list&)            = delete;
    Component_list(Component_list&&)                 = delete;
    Component_list& operator=(const Component_list&) = delete;
    Component_list& operator=(Component_list&&)      = delete;

    // /// Gets the attached game object (the game object that owns this component list).
    // Game_object& get_attached_game_obj()
    // {
    //     return m_attached_game_obj;
    // }

    /// Adds component with a value.
    template<typename T>
    void add_component(T init_value)
    {
        size_t data_offset{ m_components_datas.size() };
        auto result{ m_type_to_data_offset_map.emplace(std::type_index(typeid(T)), data_offset) };

        // Make sure that emplace successfully occurred.
        assert(result.second);

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

    /// Commits staged changes to the component list during this.
    /// @details There needs to be a buffer with stored requested changes and then have them be
    ///          committed so that the changes to component lists only happen at one moment in time,
    ///          so that systems in the ECS can depend on entities and components being immutable
    ///          (meaning just the containers that hold entities and components, bc data in
    ///          components is mutable for systems) at least during the time the systems run.
    void commit_change_requests();

    /// Gets a reference to the data of the component.
    template<typename T>
    T& get_component_handle()
    {
        size_t data_offset{ m_type_to_data_offset_map.at(std::type_index(typeid(T))) };
        return *reinterpret_cast<T*>(&m_components_datas[data_offset]);
    }

    /// Checks whether component is in component list.
    bool check_component_exists(std::type_index comp_typename_idx) const;

    // // Scene_serialization_ifc
    // void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

private:
    // Game_object& m_attached_game_obj;

    std::unordered_map<std::type_index, size_t> m_type_to_data_offset_map;
    std::vector<uint8_t> m_components_datas;  // Components are stored as bytes inside here.

public:
    /// Serialization/deserialization
    /// @NOTE: Copied from `NLOHMANN_DEFINE_TYPE_INTRUSIVE()` and modified.
    template<typename BasicJsonType,
             nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value,
                                           int> = 0>
    friend void to_json(BasicJsonType& nlohmann_json_j, const Component_list& nlohmann_json_t)
    {
        // @TODO: DEFINE THISvv
        assert(false);
        // nlohmann_json_j["m_type_to_data_offset_map"] = nlohmann_json_t.m_type_to_data_offset_map;
    }
    template<typename BasicJsonType,
             nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value,
                                           int> = 0>
    friend void from_json(const BasicJsonType& nlohmann_json_j, Component_list& nlohmann_json_t)
    {
        // @TODO: DEFINE THISvv
        assert(false);
        // nlohmann_json_j.at("m_type_to_data_offset_map")
        //     .get_to(nlohmann_json_t.m_type_to_data_offset_map);
    }
};

/// Forward declare.
struct Component_list_query;

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

    struct Component_metadata
    {
        size_t component_idx;
        std::type_index typename_id_idx;

        std::function<void(Component_list&, json&)> add_component_from_json_fn;
        std::function<void(Component_list&, json&)> serialize_component_to_json_fn;
        std::function<void(Component_list&)> remove_component_fn;
    };
    Component_metadata const& find_component_metadata_by_typename_str(
        std::string const& typename_str) const;
    std::string find_typename_str_by_component_typeid(std::type_index comp_typeid_idx) const;

    void add_component_list(Component_list* comp_list);
    void remove_component_list(Component_list* comp_list);
    std::vector<Component_list*> query_component_lists(Component_list_query const& query);

private:
    // Registered components.
    std::unordered_map<std::string, size_t> m_component_name_to_list_idx_map;
    std::vector<Component_metadata> m_components;

    // Registered component lists.
    std::vector<Component_list*> m_added_component_lists;
};

}  // namespace component_system
}  // namespace BT

#endif  // !BTZC_REFACTOR_TO_ENTT
