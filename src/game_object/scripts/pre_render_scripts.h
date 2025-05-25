#pragma once

#include <array>
#include <cassert>
#include <string>
#include <vector>

using std::array;
using std::string;
using std::vector;


#define LIST_OF_SCRIPTS \
    X(none) \
    X(apply_physics_transform_to_render_object)


namespace BT
{

class Render_object;

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
#define X(name)  void script_ ## name(Render_object* rend_obj, vector<uint64_t> const& datas, size_t& in_out_read_data_idx);
LIST_OF_SCRIPTS
#undef X

inline void execute_pre_render_script(Render_object* rend_obj,
                                      Script_type script_type,
                                      vector<uint64_t> const& datas,
                                      size_t& in_out_read_data_idx)
{
    if (script_type == SCRIPT_TYPE_none)
    {   // Exit early for none function.
        return;
    }

    switch (script_type)
    {
        #define X(name)  case SCRIPT_TYPE_ ## name: script_ ## name(rend_obj, datas, in_out_read_data_idx); break;
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
