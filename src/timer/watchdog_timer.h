#pragma once

#include <atomic>
#include <chrono>
#include <thread>

using std::atomic;
using std::atomic_bool;
using std::jthread;


namespace BT
{

using high_res_time_t = std::chrono::high_resolution_clock::time_point;
using high_res_duration_t = std::chrono::high_resolution_clock::duration;

class Watchdog_timer
{
public:
    Watchdog_timer();
    ~Watchdog_timer();

    bool is_active() { return m_active; }
    void pet();
    bool is_timed_out();

private:
    atomic_bool m_active{ true };
    atomic<high_res_time_t> m_pet_time;
    high_res_duration_t m_timeout_time;
    jthread m_watchdog_thread;
};

}  // namespace BT
