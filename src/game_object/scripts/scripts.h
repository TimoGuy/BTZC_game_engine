#pragma once

#include "list_of_scripts_begin.h"

#include "../../scene/scene_serialization_ifc.h"
#include "logger.h"
#include <array>
#include <cassert>
#include <memory>
#include <string>
#include <vector>

using std::array;
using std::string;
using std::unique_ptr;
using std::vector;


namespace BT
{

class Input_handler;
class Physics_engine;
class Renderer;
class Game_object_pool;

namespace Scripts
{

enum Script_type
{
    #define X(name)  SCRIPT_TYPE_ ## name,
    LIST_OF_SCRIPTS
    #undef X

    NUM_SCRIPT_TYPES
};

class Script_ifc
{
public:
    virtual ~Script_ifc() = default;

    virtual Script_type get_type() = 0;
    virtual void serialize_datas(json& node_ref) = 0;
    virtual void on_pre_physics(float_t physics_delta_time) { }
    virtual void on_pre_render(float_t delta_time) { }
};

array<string const, NUM_SCRIPT_TYPES> const k_script_names{
    #define X(name)  string("script_" #name),
    LIST_OF_SCRIPTS
    #undef X
};

namespace Helper_funcs
{

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

}  // namespace Helper_funcs

namespace Factory_impl_funcs
{
#define X(name)  unique_ptr<Script_ifc> create_script_ ## name ## _from_serialized_datas(Input_handler* input_handler, Physics_engine* phys_engine, Renderer* renderer, Game_object_pool* game_obj_pool, json const& node_ref);
LIST_OF_SCRIPTS
#undef X
} // namespace Factory_impl_funcs

inline unique_ptr<Script_ifc> create_script_from_serialized_datas(
    Input_handler* input_handler,
    Physics_engine* phys_engine,
    Renderer* renderer,
    Game_object_pool* game_obj_pool,
    json const& node_ref)
{
    switch (Helper_funcs::get_script_type_from_name(node_ref["script_type"]))
    {
        #define X(name)  case SCRIPT_TYPE_ ## name: return Factory_impl_funcs::create_script_ ## name ## _from_serialized_datas(input_handler, phys_engine, renderer, game_obj_pool, node_ref["script_datas"]);
        LIST_OF_SCRIPTS
        #undef X

        default:
            // Unknown/undefined script type entered.
            assert(false);
            break;
    }

    return nullptr;
}

inline void serialize_script(Script_ifc* script, json& node_ref)
{
    node_ref["script_type"] = Helper_funcs::get_script_name_from_type(script->get_type());
    script->serialize_datas(node_ref["script_datas"]);
}

}  // namespace Scripts
}  // namespace BT


#include "list_of_scripts_end.h"
