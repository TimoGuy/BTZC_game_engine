#pragma once

#include <cmath>


namespace BT
{

class Physics_engine
{
public:
    float_t limit_delta_time(float_t delta_time);
    void accumulate_delta_time(float_t delta_time);
    bool calc_wants_to_tick();
    void update_interpolation_alpha();

    void update_physics();

    static constexpr uint32_t k_simulation_hz{ 50 };
    static constexpr float_t k_simulation_delta_time{ 1.0f / k_simulation_hz };

private:
    static constexpr float_t k_accumulate_delta_time_limit{ k_simulation_delta_time * 3 };

    // Pause delta time accumulation if accumulation reaches this.
    static constexpr float_t k_accumulation_hard_stop_limit{ 0.5f };

    float_t m_accumulated_delta_time{ 0 };
    bool m_accumulation_hard_stop{ false };

    float_t m_interpolation_alpha;
};

}  // namespace BT
