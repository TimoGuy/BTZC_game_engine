#include "game_object.h"

#include <atomic>
#include <cassert>

using std::atomic_uint8_t;
using std::atomic_uint64_t;


void BT::Game_object::run_pre_physics_scripts(float_t physics_delta_time)
{
    // @TODO: Iterate thru script enum list and execute scripts.
    assert(false);
}

void BT::Game_object::run_pre_render_scripts(float_t delta_time)
{
    // @TODO
    assert(false);
}

BT::Game_object_pool::gob_key_t BT::Game_object_pool::emplace(unique_ptr<Game_object>&& game_object)
{
    gob_key_t key{ m_next_key++ };

    wait_until_free_then_block();
    m_game_objects.emplace(key, std::move(game_object));
    unblock();

    return key;
}

void BT::Game_object_pool::remove(gob_key_t key)
{
    wait_until_free_then_block();
    if (m_game_objects.find(key) == m_game_objects.end())
    {
        // Fail bc key was invalid.
        assert(false);
        return;
    }

    m_game_objects.erase(key);
    unblock();
}

vector<BT::Game_object*> const BT::Game_object_pool::checkout_all_as_list()
{
    wait_until_free_then_block();

    vector<Game_object*> all_game_objs;
    all_game_objs.reserve(m_game_objects.size());

    for (auto it = m_game_objects.begin(); it != m_game_objects.end(); it++)
    {
        all_game_objs.emplace_back(it->second.get());
    }

    return all_game_objs;
}

void BT::Game_object_pool::return_all_as_list(vector<Game_object*> const&& all_as_list)
{
    // Assumed that this is the end of using the gameobject list.
    // @TODO: Prevent misuse of this function.
    // (@IDEA: Make it so that instead of a vector use a class that has a release function or a dtor)
    (void)all_as_list;
    unblock();
}

// Synchronization.
void BT::Game_object_pool::wait_until_free_then_block()
{
    while (true)
    {
        bool unblocked{ false };
        if (m_blocked.compare_exchange_weak(unblocked, true))
        {   // Exchange succeeded and is now in blocking state.
            break;
        }
    }
}

void BT::Game_object_pool::unblock()
{
    m_blocked.store(false);
}
