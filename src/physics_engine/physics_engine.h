#pragma once

#include "physics_object.h"
#include <atomic>
#include <cmath>
#include <memory>
#include <unordered_map>

using std::atomic_bool;
using std::atomic_uint64_t;
using std::unique_ptr;
using std::unordered_map;

using physics_object_key_t = uint64_t;


namespace BT
{

class Physics_engine
{
public:
    Physics_engine();
    ~Physics_engine();

    void* get_physics_system_ptr();
    void* get_physics_temp_allocator_ptr();

    static constexpr uint32_t k_simulation_hz{ 50 };
    static constexpr float_t k_simulation_delta_time{ 1.0f / k_simulation_hz };

    float_t limit_delta_time(float_t delta_time);
    void accumulate_delta_time(float_t delta_time);
    bool calc_wants_to_tick();

    void calc_interpolation_alpha();
    float_t get_interpolation_alpha() const { return m_interpolation_alpha; }

    void update_physics();

    // Add/remove physics objects.
    physics_object_key_t emplace_physics_object(unique_ptr<Physics_object>&& phys_obj);
    void remove_physics_object(physics_object_key_t key);
    Physics_object* checkout_physics_object(physics_object_key_t key);
    void return_physics_object(Physics_object* phys_obj);

private:
    static constexpr float_t k_accumulate_delta_time_limit{ k_simulation_delta_time * 3 };

    // Pause delta time accumulation if accumulation reaches this.
    static constexpr float_t k_accumulation_hard_stop_limit{ 0.5f };

    float_t m_accumulated_delta_time{ 0 };
    bool m_accumulation_hard_stop{ false };

    float_t m_interpolation_alpha;

    // Physics object pool.
    // @COPYPASTA: see "game_object.h"
    atomic_uint64_t m_next_key{ 0 };
    unordered_map<physics_object_key_t, unique_ptr<Physics_object>> m_physics_objects;

    atomic_bool m_blocked{ false };

    void phys_obj_pool_wait_until_free_then_block();
    void phys_obj_pool_unblock();

    // Jolt physics implementation.
    class Phys_impl;
    unique_ptr<Phys_impl> m_pimpl;
};

}  // namespace BT
