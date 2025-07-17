#pragma once

#include "../physics_engine/physics_engine.h"
#include "../scene/scene_serialization_ifc.h"
#include "../uuid/uuid_ifc.h"
#include "cglm/cglm.h"
#include "material.h"
#include "mesh.h"
#include "model_animator.h"
#include <atomic>
#include <memory>
#include <string>
#include <vector>

using std::atomic_bool;
using std::atomic_uint64_t;
using std::string;
using std::unique_ptr;
using std::vector;


namespace BT
{

class Game_object;

enum Render_layer : uint8_t
{
    RENDER_LAYER_ALL          = 0b11111111,
    RENDER_LAYER_NONE         = 0b00000000,

    RENDER_LAYER_DEFAULT      = 0b00000001,
    RENDER_LAYER_INVISIBLE    = 0b00000010,
    RENDER_LAYER_LEVEL_EDITOR = 0b00000100,
};

class Physics_object;

class Render_object : public Scene_serialization_ifc, public UUID_ifc
{
public:
    Render_object(Game_object& game_obj,
                  Render_layer layer,
                  Renderable_ifc const* renderable = nullptr);

    Game_object& get_owning_game_obj() { return m_game_obj; }

    void set_deformed_model(unique_ptr<Deformed_model>&& deformed_model)
    {
        m_deformed_model = std::move(deformed_model);
        if (m_deformed_model != nullptr)
            m_renderable = m_deformed_model.get();
    }
    Deformed_model* get_deformed_model() { return m_deformed_model.get(); }

    void render(Render_layer active_layers,
                Material_ifc* override_material = nullptr);

    // Scene_serialization_ifc.
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

private:
    Game_object& m_game_obj;
    Render_layer m_layer;
    Renderable_ifc const* m_renderable;
    unique_ptr<Deformed_model> m_deformed_model{ nullptr };  // For owning a deformed model (since models are stored in a bank).
    unique_ptr<Model_animator> m_model_animator{ nullptr };
};

// @COPYPASTA: Not quite copypasta. It's a little bit different.
class Render_object_pool
{
public:
    UUID emplace(Render_object&& rend_obj);
    void remove(UUID key);
    vector<Render_object*> checkout_all_render_objs();
    vector<Render_object*> checkout_render_obj_by_key(vector<UUID>&& keys);
    void return_render_objs(vector<Render_object*>&& render_objs);

private:
    unordered_map<UUID, Render_object> m_render_objects;

    // Synchronization.
    atomic_bool m_blocked{ false };

    void wait_until_free_then_block();
    void unblock();
};

}  // namespace BT
