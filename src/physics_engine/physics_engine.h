#pragma once

#include "../uuid/uuid_ifc.h"
#include "physics_object.h"
#include <atomic>
#include <cmath>
#include <memory>
#include <unordered_map>

using std::atomic_bool;
using std::atomic_uint64_t;
using std::unique_ptr;
using std::unordered_map;


namespace BT
{

class Physics_engine
{
public:
    Physics_engine();
    ~Physics_engine();

    void* get_physics_system_ptr();
    void* get_physics_body_ifc();
    void* get_physics_temp_allocator_ptr();

    static constexpr uint32_t k_simulation_hz{ 60 };
    static constexpr float_t k_simulation_delta_time{ 1.0f / k_simulation_hz };

    float_t limit_delta_time(float_t delta_time);
    void accumulate_delta_time(float_t delta_time);
    bool calc_wants_to_tick();

    void calc_interpolation_alpha();
    float_t get_interpolation_alpha() const { return m_interpolation_alpha; }

    void update_physics();

    // Add/remove physics objects.
    UUID emplace_physics_object(unique_ptr<Physics_object>&& phys_obj);
    void remove_physics_object(UUID key);
    Physics_object* checkout_physics_object(UUID key);
    vector<Physics_object*> checkout_all_physics_objects();
    void return_physics_object(Physics_object* phys_obj);
    void return_physics_objects(vector<Physics_object*>&& phys_objs);

private:
    static constexpr float_t k_accumulate_delta_time_limit{ k_simulation_delta_time * 3 };

    // Pause delta time accumulation if accumulation reaches this.
    static constexpr float_t k_accumulation_hard_stop_limit{ 0.5f };

    float_t m_accumulated_delta_time{ 0 };
    bool m_accumulation_hard_stop{ false };

    float_t m_interpolation_alpha;

    // Physics object pool.
    // @COPYPASTA: see "game_object.h"
    unordered_map<UUID, unique_ptr<Physics_object>> m_physics_objects;

    atomic_bool m_blocked{ false };

    void phys_obj_pool_wait_until_free_then_block();
    void phys_obj_pool_unblock();

    // Jolt physics implementation.
    class Phys_impl;
    unique_ptr<Phys_impl> m_pimpl;
};

}  // namespace BT
