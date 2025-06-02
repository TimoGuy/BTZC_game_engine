#pragma once

#include "../input_handler/input_handler.h"
#include "../physics_engine/physics_engine.h"
#include "../physics_engine/physics_object.h"
#include "../renderer/renderer.h"
#include "../renderer/render_object.h"
#include "../scene/scene_serialization_ifc.h"
#include "../uuid/uuid_ifc.h"
#include "scripts/scripts.h"
#include <atomic>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using std::atomic_bool;
using std::atomic_uint64_t;
using std::pair;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;


namespace BT
{

class Game_object : public Scene_serialization_ifc, public UUID_ifc
{
public:
    Game_object(Input_handler& input_handler,
                Physics_engine& phys_engine,
                Renderer& renderer);

    void run_pre_physics_scripts(float_t physics_delta_time);
    void run_pre_render_scripts(float_t delta_time);

    // Scene_serialization_ifc.
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

private:
    Input_handler& m_input_handler;
    Physics_engine& m_phys_engine;
    Renderer& m_renderer;

    string m_name;
    UUID m_phys_obj_key;
    UUID m_rend_obj_key;
    vector<unique_ptr<Scripts::Script_ifc>> m_scripts;

    vector<UUID> m_children;
};

// vv See below ~~@COPYPASTA. See "mesh.h" "material.h" "shader.h"~~
// @NOTE: Not quite copypasta. It's a little bit different.
class Game_object_pool
{
public:
    UUID emplace(unique_ptr<Game_object>&& game_object);
    void remove(UUID key);

    vector<Game_object*> const checkout_all_as_list();
    Game_object* checkout_one(UUID key);
    void return_list(vector<Game_object*> const&& all_as_list);

private:
    unordered_map<UUID, unique_ptr<Game_object>> m_game_objects;

    // Synchronization.
    atomic_bool m_blocked{ false };

    void wait_until_free_then_block();
    void unblock();
};

}  // namespace BT
