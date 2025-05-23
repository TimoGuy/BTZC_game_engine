#pragma once

#include <array>
#include <cassert>
#include <string>

using std::array;
using std::string;


#define LIST_OF_SCRIPTS \
    X(none) \
    X(example_pre_render_procedure)


namespace BT
{
namespace Pre_render_script
{

enum Script_type
{
    #define X(name)  SCRIPT_TYPE_ ## name,
    LIST_OF_SCRIPTS
    #undef X

    NUM_SCRIPT_TYPES
};

array<string const, NUM_SCRIPT_TYPES> const k_script_names{
    #define X(name)  string("pre_render_script_" #name),
    LIST_OF_SCRIPTS
    #undef X
};

inline string get_script_name_from_type(Script_type script_type)
{
    return k_script_names[script_type];
}

// All script func prototypes.
class Render_object;

#define X(name)  void script_ ## name(Render_object const& rend_obj);
LIST_OF_SCRIPTS
#undef X

inline void execute_pre_physics_script(Render_object const& rend_obj, Script_type script_type)
{
    if (script_type == SCRIPT_TYPE_none)
    {   // Exit early for none function.
        return;
    }

    switch (script_type)
    {
        #define X(name)  case SCRIPT_TYPE_ ## name: script_ ## name(rend_obj); break;
        LIST_OF_SCRIPTS
        #undef X

        default:
            // Unknown/undefined script type entered.
            assert(false);
            break;
    }
}

}  // namespace Pre_render_script
}  // namespace BT


#undef LIST_OF_SCRIPTS
