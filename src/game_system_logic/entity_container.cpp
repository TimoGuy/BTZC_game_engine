#include "entity_container.h"

#include "btlogger.h"
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

size_t get_num_entities(entt::registry& reg)
{
    return reg.view<entt::entity>().size();
}

}  // namespace


entt::entity BT::Entity_container::create_entity(UUID uuid)
{   // Assert that there were no illegal direct entity additions/deletions within registry.
    assert(m_uuid_to_inner_entity_map.size() == get_num_entities(m_ecs_registry));

    auto entity{ m_ecs_registry.create() };

    auto result{ m_uuid_to_inner_entity_map.emplace(uuid, entity).second };
    if (!result)
    {   // Error: that emplace did not succeed. There is a duplicate UUID, likely.
        BT_ERRORF("`create_entity()` emplace failed. Likely duplicate UUID: %s",
                  UUID_helper::to_pretty_repr(uuid).c_str());
        assert(false);
        return entt::null;
    }

    return entity;
}

void BT::Entity_container::destroy_entity(UUID uuid)
{   // Assert that there were no illegal direct entity additions/deletions within registry.
    assert(m_uuid_to_inner_entity_map.size() == get_num_entities(m_ecs_registry));

    m_ecs_registry.destroy(m_uuid_to_inner_entity_map.at(uuid));
    m_uuid_to_inner_entity_map.erase(uuid);
}

entt::entity BT::Entity_container::find_entity(UUID uuid) const
{
    entt::entity found_entity{ entt::null };

    auto it{m_uuid_to_inner_entity_map.find(uuid)};
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

entt::registry& BT::Entity_container::get_ecs_registry()
{
    return m_ecs_registry;
}
