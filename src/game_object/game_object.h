#pragma once

#include "../physics_engine/physics_engine.h"
#include "../physics_engine/physics_object.h"
#include "../renderer/renderer.h"
#include "../renderer/render_object.h"
#include "../scene/scene_serialization_ifc.h"
#include "../uuid/uuid.h"
#include "scripts/pre_physics_scripts.h"
#include "scripts/pre_render_scripts.h"
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

class Game_object : public Scene_serialization_ifc
{
public:
    Game_object(string const& name,
                Physics_engine& phys_engine,
                Renderer& renderer,
                // Everything below this is planned to be taken care of by `scene_serialize()` for loading.
                physics_object_key_t phys_obj_key,
                render_object_key_t rend_obj_key,
                vector<Pre_physics_script::Script_type>&& pre_physics_scripts,
                vector<uint64_t>&& pre_physics_user_datas,
                vector<Pre_render_script::Script_type>&& pre_render_scripts,
                vector<uint64_t>&& pre_render_user_datas);

    void run_pre_physics_scripts(float_t physics_delta_time);
    void run_pre_render_scripts(float_t delta_time);
    void generate_uuid();
    UUID const& get_uuid();

    // Scene_serialization_ifc.
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

private:
    string m_name;
    UUID m_uuid;
    Physics_engine& m_phys_engine;
    Renderer& m_renderer;

    vector<Pre_physics_script::Script_type> m_pre_physics_scripts;
    vector<uint64_t> m_pre_physics_user_datas;
    vector<Pre_render_script::Script_type> m_pre_render_scripts;
    vector<uint64_t> m_pre_render_user_datas;
};

// vv See below ~~@COPYPASTA. See "mesh.h" "material.h" "shader.h"~~
// @NOTE: Not quite copypasta. It's a little bit different.
class Game_object_pool
{
public:
    using gob_key_t = UUID;
    gob_key_t emplace(unique_ptr<Game_object>&& game_object);
    void remove(gob_key_t key);

    vector<Game_object*> const checkout_all_as_list();
    Game_object* checkout_one(gob_key_t key);
    void return_list(vector<Game_object*> const&& all_as_list);

private:
    unordered_map<gob_key_t, unique_ptr<Game_object>> m_game_objects;

    // Synchronization.
    atomic_bool m_blocked{ false };

    void wait_until_free_then_block();
    void unblock();
};

}  // namespace BT
