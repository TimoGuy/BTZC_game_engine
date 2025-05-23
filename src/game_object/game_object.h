#pragma once

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

};

// ~~@COPYPASTA.~~ See "mesh.h" "material.h" "shader.h"
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
