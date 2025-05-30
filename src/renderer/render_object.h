#pragma once

#include "../physics_engine/physics_engine.h"
#include "cglm/cglm.h"
#include "mesh.h"
#include <atomic>
#include <string>
#include <vector>

using std::atomic_bool;
using std::atomic_uint64_t;
using std::string;
using std::vector;


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

    void set_transform(mat4 transform);
    void get_position(vec3& position);
    inline physics_object_key_t get_tethered_phys_obj_key() const { return m_tethered_phys_obj; }

private:
    Model const& m_model;
    Render_layer m_layer;
    mat4 m_transform;

    // (Optional) Tethered physics object.
    physics_object_key_t m_tethered_phys_obj;
};

// @COPYPASTA: Not quite copypasta. It's a little bit different.
using render_object_key_t = uint64_t;
class Render_object_pool
{
public:
    render_object_key_t emplace(Render_object&& rend_obj);
    void remove(render_object_key_t key);
    vector<Render_object*> checkout_all_render_objs();
    vector<Render_object*> checkout_render_obj_by_key(vector<render_object_key_t>&& keys);
    void return_render_objs(vector<Render_object*>&& render_objs);

private:
    atomic_uint64_t m_next_key{ 0 };
    unordered_map<render_object_key_t, Render_object> m_render_objects;

    // Synchronization.
    atomic_bool m_blocked{ false };

    void wait_until_free_then_block();
    void unblock();
};

}  // namespace BT
