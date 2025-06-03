#pragma once

#include "nlohmann/json.hpp"
using json = nlohmann::json;


namespace BT
{

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

}  // namespace BT
