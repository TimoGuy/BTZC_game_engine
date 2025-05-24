#pragma once

#include <format>
#include <string>
#include <utility>

using std::format;
using std::format_string;
using std::forward;
using std::string;


namespace BT
{
namespace logger
{

enum Log_type : uint32_t
{
    NONE  = 0b0000,
    ALL   = 0xffffffff,

    TRACE = 0b0001,
    WARN  = 0b0010,
    ERROR = 0b0100,
};

void notify_start_new_mainloop_iteration();
void set_logging_print_mask(Log_type types);
void printe(Log_type type, string entry);

template<typename... T>
void printef(Log_type type, format_string<T...> const format_str, T&&... args)
{
    string entry = format(format_str, forward<T>(args)...);
    printe(type, entry);
}

}  // namespace logger
}  // namespace BT
