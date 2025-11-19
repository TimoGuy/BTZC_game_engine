#include "watchdog_timer.h"

#include "btlogger.h"
#include <chrono>
#include <cstdlib>
#include <thread>

using namespace std::chrono_literals;  // For `ms` suffix.


// Watchdog thread function.
namespace
{

void watchdog_thread_fn(BT::Watchdog_timer& watchdog)
{
    while (watchdog.is_active())
    {
        std::this_thread::sleep_for(2000ms);

        if (watchdog.is_timed_out())
        {
            // Crash the program.
            BT::logger::printe(BT::logger::ERROR, "FATAL: Watchdog timed out. Aborting program.");
            abort();
            exit(0xB00B1E55);
        }
    }
}

}  // namespace


BT::Watchdog_timer::Watchdog_timer()
    : m_pet_time{ std::chrono::high_resolution_clock::now() }
    , m_timeout_time{ 60000ms }
    , m_watchdog_thread(watchdog_thread_fn, std::ref(*this))
{
}

BT::Watchdog_timer::~Watchdog_timer()
{
    // Shut down watchdog thread.
    m_active = false;
}

void BT::Watchdog_timer::pet()
{
    m_pet_time.store(std::chrono::high_resolution_clock::now());
}

bool BT::Watchdog_timer::is_timed_out()
{
    return (std::chrono::high_resolution_clock::now() - m_pet_time.load() > m_timeout_time);
}
