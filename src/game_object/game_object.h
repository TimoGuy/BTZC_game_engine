#pragma once

#include "../renderer/renderer.h"
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

class Game_object
{
public:
    Game_object(Renderer& renderer,
                vector<Pre_physics_script::Script_type>&& pre_physics_scripts,
                vector<Pre_render_script::Script_type>&& pre_render_scripts,
                vector<uint64_t>&& user_datas);
    void run_pre_physics_scripts(float_t physics_delta_time);
    void run_pre_render_scripts(float_t delta_time);

private:
    Renderer& m_renderer;

    vector<Pre_physics_script::Script_type> m_pre_physics_scripts;
    vector<Pre_render_script::Script_type> m_pre_render_scripts;

    // @TEMP: @INCOMPLETE: Temporary implementation of the data game objects contain (without constructing stuff inside it).
    vector<uint64_t> m_user_datas;
};

// vv See below ~~@COPYPASTA. See "mesh.h" "material.h" "shader.h"~~
// @NOTE: Not quite copypasta. It's a little bit different.
class Game_object_pool
{
public:
    using gob_key_t = uint64_t;
    gob_key_t emplace(unique_ptr<Game_object>&& game_object);
    void remove(gob_key_t key);

    vector<Game_object*> const checkout_all_as_list();
    void return_all_as_list(vector<Game_object*> const&& all_as_list);

private:
    atomic_uint64_t m_next_key{ 0 };
    unordered_map<gob_key_t, unique_ptr<Game_object>> m_game_objects;

    // Synchronization.
    atomic_bool m_blocked{ false };

    void wait_until_free_then_block();
    void unblock();
};

}  // namespace BT
