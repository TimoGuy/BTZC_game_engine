#include "physics_engine.h"

#include "physics_engine_impl.h"
#include <algorithm>
#include <cassert>
#include <memory>

using std::make_unique;
using std::min;


BT::Physics_engine::Physics_engine()
    : m_pimpl(make_unique<Phys_impl>())
{
}

BT::Physics_engine::~Physics_engine() = default;

float_t BT::Physics_engine::limit_delta_time(float_t delta_time)
{
    if (m_accumulation_hard_stop)
    {
        // Prevent delta time from moving forward until physics engine has caught up.
        return 0.0f;
    }
    return min(delta_time, k_simulation_delta_time);
}

void* BT::Physics_engine::get_physics_system_ptr()
{
    return m_pimpl->get_physics_system_ptr();
}

void BT::Physics_engine::accumulate_delta_time(float_t delta_time)
{
    m_accumulated_delta_time += delta_time;
}

bool BT::Physics_engine::calc_wants_to_tick()
{
    bool tick_wanted{ false };

    if (m_accumulated_delta_time >= k_simulation_delta_time)
    {
        // Wants to tick.
        tick_wanted = true;
        m_accumulated_delta_time -= k_simulation_delta_time;

        if (m_accumulated_delta_time >= k_accumulation_hard_stop_limit)
        {
            // Put a hard stop on the accumulation of delta time.
            m_accumulation_hard_stop = true;
        }
    }
    else if (m_accumulation_hard_stop)
    {
        // Release hard stop now that accumulated delta time has reached normal amounts again.
        m_accumulation_hard_stop = false;
    }

    return tick_wanted;
}

void BT::Physics_engine::calc_interpolation_alpha()
{
    m_interpolation_alpha = (m_accumulated_delta_time / k_simulation_delta_time);
    assert(m_interpolation_alpha >= 0.0f && m_interpolation_alpha < 1.0f);  // Should always be [0, 1).
}

void BT::Physics_engine::update_physics()
{
    phys_obj_pool_wait_until_free_then_block();
    m_pimpl->update(k_simulation_delta_time);
    // @TODO: @HERE: Deposit all transforms into the physics objects.
    phys_obj_pool_unblock();
}

// Add/remove physics objects.
physics_object_key_t BT::Physics_engine::emplace_physics_object(
    unique_ptr<Physics_object>&& phys_obj)
{
    physics_object_key_t key{ m_next_key++ };

    phys_obj_pool_wait_until_free_then_block();
    m_game_objects.emplace(key, std::move(phys_obj));
    phys_obj_pool_unblock();

    return key;
}

void BT::Physics_engine::remove_physics_object(physics_object_key_t key)
{
    phys_obj_pool_wait_until_free_then_block();
    if (m_game_objects.find(key) == m_game_objects.end())
    {
        // Fail bc key was invalid.
        assert(false);
        return;
    }

    m_game_objects.erase(key);
    phys_obj_pool_unblock();
}

// Physics object pool.
// @COPYPASTA: see "game_object.cpp"
void BT::Physics_engine::phys_obj_pool_wait_until_free_then_block()
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

void BT::Physics_engine::phys_obj_pool_unblock()
{
    m_blocked.store(false);
}
