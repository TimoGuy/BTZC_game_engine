#pragma once

#include "btjson.h"
#include "uuid/uuid.h"

#include <string>
#include <vector>


namespace BT
{
namespace world
{

/// Data structure that holds the whole saved/loaded scene in itself.
struct Scene_serialization
{
    struct Entity_serialization
    {
        UUID entity_uuid;

        struct Component_serialization
        {
            std::string type_name;
            json members_j;

            NLOHMANN_DEFINE_TYPE_INTRUSIVE(Component_serialization, type_name, members_j);
        };
        std::vector<Component_serialization> components;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Entity_serialization, entity_uuid, components);
    };
    std::vector<Entity_serialization> entities;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Scene_serialization, entities);
};

/// Loads scene data structure from disk and returns deserialized struct.
Scene_serialization deserialize_scene_data_from_disk(std::string const& scene_name);

/// Saves scene data structure to disk.
void serialize_scene_data_to_disk(Scene_serialization const& scene_data,
                                  std::string const& scene_name);

/// Serializes ECS entity and all its components into an entity.
Scene_serialization::Entity_serialization serialize_entity(UUID entity_uuid);

}  // namespace world
}  // namespace BT
