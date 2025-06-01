#include "game_object.h"

#include "../input_handler/input_handler.h"
#include "../physics_engine/physics_engine.h"
#include "../renderer/renderer.h"
#include "logger.h"
#include "scripts/scripts.h"
#include <atomic>
#include <cassert>

using std::atomic_uint8_t;
using std::atomic_uint64_t;


BT::Game_object::Game_object(string const& name,
                             Input_handler& input_handler,
                             Physics_engine& phys_engine,
                             Renderer& renderer,
                             UUID phys_obj_key,
                             UUID rend_obj_key,
                             vector<unique_ptr<Scripts::Script_ifc>>&& scripts)
    : m_name(name)
    , m_input_handler(input_handler)
    , m_phys_engine(phys_engine)
    , m_renderer(renderer)
    , m_scripts(std::move(scripts))
{
}

void BT::Game_object::run_pre_physics_scripts(float_t physics_delta_time)
{
    for (auto& script : m_scripts)
    {
        script->on_pre_physics(physics_delta_time);
    }
}

void BT::Game_object::run_pre_render_scripts(float_t delta_time)
{
    for (auto& script : m_scripts)
    {
        script->on_pre_render(delta_time);
    }
}

// Scene_serialization_ifc.
void BT::Game_object::scene_serialize(Scene_serialization_mode mode, json& node_ref)
{
    if (mode == SCENE_SERIAL_MODE_SERIALIZE)
    {
        node_ref["name"] = m_name;
        node_ref["guid"] = UUID_helper::to_pretty_repr(get_uuid());

        node_ref["scripts"] = json::array();
        size_t scripts_idx{ 0 };
        for (auto& script : m_scripts)
        {
            Scripts::serialize_script(script.get(),
                                      node_ref["scripts"][scripts_idx++]);
        }

        node_ref["children"] = json::array();
        for (auto& child_uuid : m_children)
        {
            node_ref["children"].emplace_back(UUID_helper::to_pretty_repr(child_uuid));
        }

        // @TODO: Serialize render and physics objs.
    }
    else if (mode == SCENE_SERIAL_MODE_DESERIALIZE)
    {
        m_name = node_ref["name"];
        assign_uuid(node_ref["guid"], true);

        assert(node_ref["scripts"].is_array());
        for (size_t scripts_idx = 0; scripts_idx < node_ref["scripts"].size();)
        {
            m_scripts.emplace_back(
                Scripts::create_script_from_serialized_datas(&m_input_handler,
                                                             &m_phys_engine,
                                                             &m_renderer,
                                                             node_ref["scripts"][scripts_idx++]));
        }

        assert(node_ref["children"].is_array());
        for (auto& child_uuid : node_ref["children"])
        {
            m_children.emplace_back(UUID_helper::to_UUID(child_uuid));
        }

        // @TODO: Deserialize render and physics objs.
    }
}


BT::UUID BT::Game_object_pool::emplace(unique_ptr<Game_object>&& game_object)
{
    UUID uuid{ game_object->get_uuid() };
    if (uuid.is_nil())
    {
        logger::printe(logger::ERROR, "Invalid UUID passed in.");
        assert(false);
    }

    wait_until_free_then_block();
    m_game_objects.emplace(uuid, std::move(game_object));
    unblock();

    return uuid;
}

void BT::Game_object_pool::remove(UUID key)
{
    wait_until_free_then_block();
    if (m_game_objects.find(key) == m_game_objects.end())
    {
        // Fail bc key was invalid.
        assert(false);
        return;
    }

    m_game_objects.erase(key);
    unblock();
}

vector<BT::Game_object*> const BT::Game_object_pool::checkout_all_as_list()
{
    wait_until_free_then_block();

    vector<Game_object*> all_game_objs;
    all_game_objs.reserve(m_game_objects.size());

    for (auto it = m_game_objects.begin(); it != m_game_objects.end(); it++)
    {
        all_game_objs.emplace_back(it->second.get());
    }

    return all_game_objs;
}

BT::Game_object* BT::Game_object_pool::checkout_one(UUID uuid)
{
    wait_until_free_then_block();

    if (m_game_objects.find(uuid) == m_game_objects.end())
    {
        logger::printef(logger::ERROR, "UUID was invalid: %s", UUID_helper::to_pretty_repr(uuid).c_str());
        assert(false);
        return nullptr;
    }

    return m_game_objects.at(uuid).get();
}

void BT::Game_object_pool::return_list(vector<Game_object*> const&& all_as_list)
{
    // Assumed that this is the end of using the gameobject list.
    // @TODO: Prevent misuse of this function.
    // (@IDEA: Make it so that instead of a vector use a class that has a release function or a dtor)
    // @COPYPASTA.
    (void)all_as_list;
    unblock();
}

// Synchronization.
void BT::Game_object_pool::wait_until_free_then_block()
{
    while (true)
    {
        bool unblocked{ false };
        if (m_blocked.compare_exchange_weak(unblocked, true))
        {   // Exchange succeeded and is now in blocking state.
            break;
        }
    }
}

void BT::Game_object_pool::unblock()
{
    m_blocked.store(false);
}
