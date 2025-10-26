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
