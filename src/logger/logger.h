#pragma once

#include <string>
#include <utility>

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
void printef(Log_type type, string format_str, ...);
void printe(Log_type type, string entry);

}  // namespace logger
}  // namespace BT
