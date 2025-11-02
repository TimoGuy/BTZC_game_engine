#include "physics_engine.h"

#include "btlogger.h"
#include "physics_engine_impl.h"
#include "physics_object.h"
#include "service_finder/service_finder.h"

#include <algorithm>
#include <cassert>
#include <memory>

using std::make_unique;
using std::min;


BT::Physics_engine::Physics_engine()
    : m_pimpl(make_unique<Phys_impl>())
{
    BT_SERVICE_FINDER_ADD_SERVICE(Physics_engine, this);
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

void* BT::Physics_engine::get_physics_body_ifc()
{
    return m_pimpl->get_physics_body_ifc();
}

void* BT::Physics_engine::get_physics_temp_allocator_ptr()
{
    return m_pimpl->get_physics_temp_allocator_ptr();
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

    // Run all physics objects' pre update.
    for (auto& phys_obj : m_physics_objects)
        phys_obj.second->get_impl()->on_pre_update(k_simulation_delta_time);

    m_pimpl->update(k_simulation_delta_time);

    // Notify all physics objects to read transforms.
    for (auto& phys_obj : m_physics_objects)
        phys_obj.second->read_and_store_new_transform();

    phys_obj_pool_unblock();
}

// Add/remove physics objects.
BT::UUID BT::Physics_engine::emplace_physics_object(
    unique_ptr<Physics_object>&& phys_obj)
{
    UUID uuid{ phys_obj->get_uuid() };
    if (uuid.is_nil())
    {   // Create UUID during emplacement.
        phys_obj->assign_generated_uuid();
        uuid = phys_obj->get_uuid();
    }
    else
    {
        logger::printe(
            logger::ERROR,
            "Pre-generated UUID passed in. Nil UUID expected so it can be created in this step.");
        assert(false);
    }

    phys_obj_pool_wait_until_free_then_block();
    if (m_physics_objects.find(uuid) != m_physics_objects.end())
    {
        logger::printef(logger::ERROR, "UUID already exists in phys obj pool: %s", UUID_helper::to_pretty_repr(uuid).c_str());
        assert(false);
    }

    m_physics_objects.emplace(uuid, std::move(phys_obj));
    phys_obj_pool_unblock();

    return uuid;
}

void BT::Physics_engine::remove_physics_object(UUID key)
{
    phys_obj_pool_wait_until_free_then_block();
    if (m_physics_objects.find(key) == m_physics_objects.end())
    {
        logger::printef(logger::ERROR, "UUID does not exist in phys obj pool: %s", UUID_helper::to_pretty_repr(key).c_str());
        assert(false);
        return;
    }

    m_physics_objects.erase(key);
    phys_obj_pool_unblock();
}

BT::Physics_object* BT::Physics_engine::checkout_physics_object(UUID key)
{
    phys_obj_pool_wait_until_free_then_block();
    return m_physics_objects.at(key).get();
}

vector<BT::Physics_object*> BT::Physics_engine::checkout_all_physics_objects()
{
    phys_obj_pool_wait_until_free_then_block();

    vector<Physics_object*> all_phys_objs;
    all_phys_objs.reserve(m_physics_objects.size());
    for (auto& phys_obj : m_physics_objects)
    {
        all_phys_objs.emplace_back(phys_obj.second.get());
    }

    return all_phys_objs;
}

void BT::Physics_engine::return_physics_object(Physics_object* phys_obj)
{
    (void)phys_obj;
    phys_obj_pool_unblock();
}

void BT::Physics_engine::return_physics_objects(vector<Physics_object*>&& phys_objs)
{
    (void)phys_objs;
    phys_obj_pool_unblock();
}

size_t BT::Physics_engine::get_num_physics_objects() const
{
    return m_physics_objects.size();
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
