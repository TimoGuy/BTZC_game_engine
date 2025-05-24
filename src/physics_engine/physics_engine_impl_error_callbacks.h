#pragma once

#include "Jolt/Jolt.h"  // @NOTE: Must appear first.
#include "logger.h"
#include <cassert>
#include <cstdarg>
#include <string>
#include <sstream>

using std::string;
using std::stringstream;
using std::to_string;


namespace BT
{
namespace phys_engine
{

// Callback for traces, connect this to your own trace function if you have one.
static void trace_impl(char const* in_fmt, ...)
{
    // Format message.
    va_list list;
    va_start(list, in_fmt);
    char buffer[1024];
    vsnprintf(buffer, sizeof(buffer), in_fmt, list);
    va_end(list);

    logger::printe(logger::ERROR, string(buffer));
}

#ifdef JPH_ENABLE_ASSERTS
// Callback for asserts, connect this to your own assert handler if you have one
static bool assert_failed_impl(char const* in_expression,
                               char const* in_message,
                               char const* in_file,
                               uint32_t in_line)
{
    stringstream ss;
    ss << in_file << ":" << in_line << ": (" << in_expression << ") "
        << (in_message != nullptr ? in_message : "");
    logger::printe(logger::ERROR, ss.str());

    // Breakpoint.
    assert(false);
    return true;
};
#endif  // JPH_ENABLE_ASSERTS

}  // namespace phys_engine
}  // namespace BT
