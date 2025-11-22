#pragma once

#include "entt/entity/registry.hpp"
#include "uuid/uuid.h"

#include <unordered_map>


namespace BT
{

class Entity_container
{
public:
    Entity_container();

    /// Creates an entity in the ECS and assigns it the ID `uuid`.
    entt::entity create_entity(UUID uuid);

    /// Destroys the entity in the ECS with the ID `uuid`.
    void destroy_entity(UUID uuid);

    /// Gets the ECS inner version of the entity.
    entt::entity find_entity(UUID uuid) const;

    /// Gets the container UUID of the entity.
    UUID find_entity_uuid(entt::entity ecs_entity) const;

    /// Gets the number of entities registered in this container.
    size_t get_num_entities() const;

    /// Gets all of the registered UUIDs inside this container.
    std::vector<UUID> get_all_entity_uuids() const;

    /// Gets a handle to the ECS registry.
    /// @warning DO NOT USE `.create()` AND/OR `.destroy()` FUNCTIONS DIRECTLY.
    entt::registry& get_ecs_registry();

private:
    std::unordered_map<UUID, entt::entity> m_uuid_to_inner_entity_map;
    std::unordered_map<entt::entity, UUID> m_inner_entity_to_uuid_map;
    entt::registry m_ecs_registry;
};

}  // namespace BT
