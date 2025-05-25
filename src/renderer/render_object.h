#pragma once

#include "../physics_engine/physics_engine.h"
#include "cglm/cglm.h"
#include "mesh.h"
#include <string>

using std::string;


namespace BT
{

enum Render_layer : uint8_t
{
    RENDER_LAYER_ALL          = 0b11111111,
    RENDER_LAYER_NONE         = 0b00000000,

    RENDER_LAYER_DEFAULT      = 0b00000001,
    RENDER_LAYER_INVISIBLE    = 0b00000010,
    RENDER_LAYER_LEVEL_EDITOR = 0b00000100,
};

class Physics_object;

class Render_object
{
public:
    Render_object(Model const& model,
                  Render_layer layer,
                  mat4 init_transform,
                  physics_object_key_t tethered_phys_obj = (physics_object_key_t)-1);

    void render(Render_layer active_layers);

private:
    Model const& m_model;
    Render_layer m_layer;
    mat4 m_transform;

    // (Optional) Tethered physics object.
    physics_object_key_t m_tethered_phys_obj;
};

}  // namespace BT
