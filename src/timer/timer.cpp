#include "timer.h"


void BT::Timer::start_timer()
{
    m_prev_time = std::chrono::high_resolution_clock::now();
}

float_t BT::Timer::calc_delta_time()
{
    float_t delta_time{ 0.0f };
    high_res_time_t time_now{ std::chrono::high_resolution_clock::now() };
    delta_time =
        std::chrono::duration<float_t>(time_now - m_prev_time)
            .count();

    m_prev_time = time_now;
    return delta_time;
}
