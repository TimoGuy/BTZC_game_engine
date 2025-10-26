#pragma once

#include "btjson.h"
#include "uuid/uuid.h"

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
        json components;

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

}  // namespace world
}  // namespace BT
