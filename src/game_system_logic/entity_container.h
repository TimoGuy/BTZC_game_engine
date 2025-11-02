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

    /// Gets the number of entities registered in this container.
    size_t get_num_entities() const;

    /// Gets a handle to the ECS registry.
    /// @warning DO NOT USE `.create()` AND/OR `.destroy()` FUNCTIONS DIRECTLY.
    entt::registry& get_ecs_registry();

private:
    std::unordered_map<UUID, entt::entity> m_uuid_to_inner_entity_map;
    entt::registry m_ecs_registry;
};

}  // namespace BT
