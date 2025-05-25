#pragma once

#include "physics_object.h"
#include <cmath>
#include <memory>

using std::unique_ptr;


namespace BT
{

class Physics_engine
{
public:
    Physics_engine();
    ~Physics_engine();

    static constexpr uint32_t k_simulation_hz{ 50 };
    static constexpr float_t k_simulation_delta_time{ 1.0f / k_simulation_hz };

    float_t limit_delta_time(float_t delta_time);
    void accumulate_delta_time(float_t delta_time);
    bool calc_wants_to_tick();

    void calc_interpolation_alpha();
    float_t get_interpolation_alpha() const { return m_interpolation_alpha; }

    void update_physics();

    // Add/remove physics objects.
    using physics_object_key_t = uint64_t;
    physics_object_key_t emplace_physics_object(unique_ptr<Physics_object>&& phys_obj);
    void remove_physics_object(physics_object_key_t key);

private:
    static constexpr float_t k_accumulate_delta_time_limit{ k_simulation_delta_time * 3 };

    // Pause delta time accumulation if accumulation reaches this.
    static constexpr float_t k_accumulation_hard_stop_limit{ 0.5f };

    float_t m_accumulated_delta_time{ 0 };
    bool m_accumulation_hard_stop{ false };

    float_t m_interpolation_alpha;

    // Physics object pool.
    // @TODO: @HERE.

    // Jolt physics implementation.
    class Phys_impl;
    unique_ptr<Phys_impl> m_pimpl;
};

}  // namespace BT
