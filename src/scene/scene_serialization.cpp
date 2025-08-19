#include "scene_serialization_ifc.h"

#include "../btzc_game_engine.h"
#include "../game_object/game_object.h"
#include <cassert>
#include <fstream>


namespace
{

using Game_object = BT::Game_object;
using UUID = BT::UUID;

static std::function<Game_object*()>     s_new_game_obj_fn;
static std::function<void(Game_object*)> s_emplace_game_obj_into_scene_fn;
static std::function<void(UUID)>         s_set_cam_following_game_obj_fn;

}  // namespace


void BT::scene_serialization_io_helper::set_load_scene_callbacks(
    std::function<Game_object*()>&& new_game_obj_fn,
    std::function<void(Game_object*)>&& emplace_game_obj_into_scene_fn,
    std::function<void(UUID)>&& set_cam_following_game_obj_fn)
{
    s_new_game_obj_fn                = std::move(new_game_obj_fn);
    s_emplace_game_obj_into_scene_fn = std::move(emplace_game_obj_into_scene_fn);
    s_set_cam_following_game_obj_fn  = std::move(set_cam_following_game_obj_fn);
}

void BT::scene_serialization_io_helper::load_scene_from_disk(std::string const& scene_name)
{   // Load from disk.
    static auto load_to_json_fn = [](std::string const& fname) {
        std::ifstream f{ fname };
        return json::parse(f);
    };
    json root = load_to_json_fn(BTZC_GAME_ENGINE_ASSET_SCENE_PATH + scene_name);

    // Create scene from json.
    size_t game_obj_idx{ 0 };
    assert(root["game_objects"].is_array());
    for (auto& game_obj_node : root["game_objects"])
    {
        auto new_game_obj{ s_new_game_obj_fn() };
        new_game_obj->scene_serialize(SCENE_SERIAL_MODE_DESERIALIZE, game_obj_node);
        s_emplace_game_obj_into_scene_fn(new_game_obj);
    }

    assert(root["cam_following_game_obj"].is_string());
    s_set_cam_following_game_obj_fn(
        UUID_helper::to_UUID(root["cam_following_game_obj"]));
}

void BT::scene_serialization_io_helper::write_scene_to_disk(
    std::string const& scene_name,
    Scene_package&& scene)
{
    // Collect scene into json.
    json root = {};
    size_t game_obj_idx{ 0 };
    for (auto game_obj : scene.game_objs)
    {
        game_obj->scene_serialize(BT::SCENE_SERIAL_MODE_SERIALIZE, root["game_objects"][game_obj_idx++]);
    }

    root["cam_following_game_obj"] = BT::UUID_helper::to_pretty_repr(
                                         scene.cam_follow_game_obj);

    // Save to disk.
    std::ofstream f{ BTZC_GAME_ENGINE_ASSET_SCENE_PATH + scene_name };
    f << root.dump(4);
}
