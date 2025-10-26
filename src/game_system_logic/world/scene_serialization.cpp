#include "scene_serialization.h"

#include "btjson.h"
#include "btzc_game_engine.h"


BT::world::Scene_serialization BT::world::deserialize_scene_data_from_disk(
    std::string const& scene_name)
{
    return Scene_serialization(json_load_from_disk(BTZC_GAME_ENGINE_ASSET_SCENE_PATH + scene_name));
}

void BT::world::serialize_scene_data_to_disk(Scene_serialization const& scene_data,
                                             std::string const& scene_name)
{
    json_save_to_disk(json(scene_data), BTZC_GAME_ENGINE_ASSET_SCENE_PATH + scene_name);
}

BT::world::Scene_serialization::Entity_serialization BT::world::serialize_entity(UUID entity_uuid)
{
    // @TODO: Implement.
    // This'll be for the `scene_saver.h` thingy I think.
    // Reference: https://github.com/skypjack/entt/issues/88#issuecomment-1276858022
    assert(false);
}
