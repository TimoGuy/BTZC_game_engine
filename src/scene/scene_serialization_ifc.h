#pragma once

#include "../uuid/uuid.h"
#include "nlohmann/json.hpp"
#include <functional>
#include <string>
#include <vector>

using json = nlohmann::json;


namespace BT
{

class Game_object;

enum Scene_serialization_mode
{
    SCENE_SERIAL_MODE_SERIALIZE = 0,
    SCENE_SERIAL_MODE_DESERIALIZE,
};

class Scene_serialization_ifc
{
public:
    virtual ~Scene_serialization_ifc() = default;
    virtual void scene_serialize(Scene_serialization_mode mode, json& node_ref) = 0;
};

namespace scene_serialization_io_helper
{

struct Scene_package
{
    UUID cam_follow_game_obj;
    std::vector<Game_object*> const& game_objs;
};

void set_load_scene_callbacks(std::function<Game_object*()>&& new_game_obj_fn,
                              std::function<void(Game_object*)>&& emplace_game_obj_into_scene_fn,
                              std::function<void(UUID)>&& set_cam_following_game_obj_fn);

void load_scene_from_disk(std::string const& scene_name,
                          Scene_package& out_scene);

void write_scene_to_disk(std::string const& scene_name,
                         Scene_package&& scene);

}  // namespace scene_serialization_io_helper

}  // namespace BT
