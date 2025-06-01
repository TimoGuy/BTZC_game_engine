#pragma once

#include "../../scene/scene_serialization_ifc.h"
#include "logger.h"
#include <array>
#include <cassert>
#include <string>
#include <vector>

using std::array;
using std::string;
using std::vector;


#define LIST_OF_SCRIPTS \
    X(apply_physics_transform_to_render_object)


namespace BT
{

class Input_handler;
class Physics_engine;
class Renderer;

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

inline Script_type get_script_type_from_name(string const& name)
{
    for (size_t i = 0; i < k_script_names.size(); i++)
    {
        if (k_script_names[i] == name)
        {
            // Return found script type.
            return Script_type(i);
        }
    }

    // No type found.
    logger::printef(logger::ERROR, "Script type not found with name: %s", name.c_str());
    assert(false);
    return NUM_SCRIPT_TYPES;
}

// All script serialization func prototypes.
#define X(name)  void script_ ## name ## _serialize(Input_handler* input_handler, Physics_engine* phys_engine, Renderer* renderer, Scene_serialization_mode mode, json& node_ref, vector<uint64_t> const& datas, size_t& in_out_read_data_idx);
LIST_OF_SCRIPTS
#undef X

inline void execute_pre_render_script_serialize(Input_handler* input_handler,
                                                 Physics_engine* phys_engine,
                                                 Renderer* renderer,
                                                 Script_type script_type,
                                                 Scene_serialization_mode mode,
                                                 json& node_ref,
                                                 vector<uint64_t> const& datas,
                                                 size_t& in_out_read_data_idx)
{
    switch (script_type)
    {
        #define X(name)  case SCRIPT_TYPE_ ## name: script_ ## name ## _serialize(input_handler, phys_engine, renderer, mode, node_ref, datas, in_out_read_data_idx); break;
        LIST_OF_SCRIPTS
        #undef X

        default:
            // Unknown/undefined script type entered.
            assert(false);
            break;
    }
}

// All script func prototypes.
#define X(name)  void script_ ## name(Renderer* renderer, vector<uint64_t> const& datas, size_t& in_out_read_data_idx);
LIST_OF_SCRIPTS
#undef X

inline void execute_pre_render_script(Renderer* renderer,
                                      Script_type script_type,
                                      vector<uint64_t> const& datas,
                                      size_t& in_out_read_data_idx)
{
    switch (script_type)
    {
        #define X(name)  case SCRIPT_TYPE_ ## name: script_ ## name(renderer, datas, in_out_read_data_idx); break;
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
