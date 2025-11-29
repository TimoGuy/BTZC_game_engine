#include "entity_container.h"

#include "btlogger.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "service_finder/service_finder.h"
#include "uuid/uuid.h"

#include <cassert>


BT::Entity_container::Entity_container()
{
    BT_SERVICE_FINDER_ADD_SERVICE(Entity_container, this);
}


namespace
{

size_t internal_get_num_ecs_entities(entt::registry& reg)
{
    return reg.view<entt::entity>().size();
}

}  // namespace


entt::entity BT::Entity_container::create_entity(UUID uuid)
{   // Assert that there were no illegal direct entity additions/deletions within registry.
    assert(m_uuid_to_inner_entity_map.size() == internal_get_num_ecs_entities(m_ecs_registry));

    auto entity{ m_ecs_registry.create() };

    auto result{ m_uuid_to_inner_entity_map.emplace(uuid, entity).second };
    auto result2{ m_inner_entity_to_uuid_map.emplace(entity, uuid).second };
    if (!result || !result2)
    {   // Error: that emplace did not succeed. There is a duplicate UUID, likely.
        BT_ERRORF("`create_entity()` emplace failed. Likely duplicate UUID: %s",
                  UUID_helper::to_pretty_repr(uuid).c_str());
        assert(false);
        return entt::null;
    }

    assert(m_uuid_to_inner_entity_map.size() == m_inner_entity_to_uuid_map.size());

    return entity;
}

void BT::Entity_container::destroy_entity(UUID uuid)
{   // Assert that there were no illegal direct entity additions/deletions within registry.
    assert(m_uuid_to_inner_entity_map.size() == internal_get_num_ecs_entities(m_ecs_registry));

    auto ecs_entity{ m_uuid_to_inner_entity_map.at(uuid) };

    m_ecs_registry.destroy(ecs_entity);
    m_uuid_to_inner_entity_map.erase(uuid);
    m_inner_entity_to_uuid_map.erase(ecs_entity);

    assert(m_uuid_to_inner_entity_map.size() == m_inner_entity_to_uuid_map.size());
}

entt::entity BT::Entity_container::find_entity(UUID uuid) const
{
    entt::entity found_entity{ entt::null };

    auto it{ m_uuid_to_inner_entity_map.find(uuid) };
    if (it != m_uuid_to_inner_entity_map.end())
    {   // Found entity.
        found_entity = it->second;
    }
    else
    {   // Entity does not exist.
        BT_ERRORF("`find_entity()` could not find UUID: %s",
                  UUID_helper::to_pretty_repr(uuid).c_str());
        assert(false);
    }

    return found_entity;
}

BT::UUID BT::Entity_container::find_entity_uuid(entt::entity ecs_entity) const
{
    UUID found_entity;

    auto it{ m_inner_entity_to_uuid_map.find(ecs_entity) };
    if (it != m_inner_entity_to_uuid_map.end())
    {   // Found entity.
        found_entity = it->second;
    }
    else
    {   // Entity does not exist.
        BT_ERRORF("`find_entity()` could not find UUID: %s",
                  std::to_string(static_cast<uint32_t>(ecs_entity)).c_str());
        assert(false);
    }

    return found_entity;
}

size_t BT::Entity_container::get_num_entities() const
{
    return m_uuid_to_inner_entity_map.size();
}

std::vector<BT::UUID> BT::Entity_container::get_all_entity_uuids() const
{
    std::vector<UUID> all_uuids;
    all_uuids.reserve(m_uuid_to_inner_entity_map.size());

    for (auto [ent_uuid, ent_ecs_id] : m_uuid_to_inner_entity_map)
        all_uuids.emplace_back(ent_uuid);

    return all_uuids;
}

entt::registry& BT::Entity_container::get_ecs_registry()
{
    return m_ecs_registry;
}
