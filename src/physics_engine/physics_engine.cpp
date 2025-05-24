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
    m_pimpl->update(k_simulation_delta_time);

    // @TODO: @HERE: Deposit all transforms into the physics objects.
}

// Add/remove physics objects.
BT::Physics_engine::physics_object_key_t BT::Physics_engine::emplace_physics_object(Physics_object&& phys_obj)
{
    phys_obj.set_physics_engine_reference(this);

    // @TODO: Do emplacing code.
    assert(false);
}

void BT::Physics_engine::remove_physics_object(physics_object_key_t key)
{
    // @TODO: Do remove code.
    assert(false);
}
