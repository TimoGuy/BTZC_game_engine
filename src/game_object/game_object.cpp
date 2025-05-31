#include "game_object.h"

#include "../physics_engine/physics_engine.h"
#include "../renderer/renderer.h"
#include "logger.h"
#include "scripts/pre_physics_scripts.h"
#include "scripts/pre_render_scripts.h"
#include <atomic>
#include <cassert>

using std::atomic_uint8_t;
using std::atomic_uint64_t;


BT::Game_object::Game_object(string const& name,
                             Physics_engine& phys_engine,
                             Renderer& renderer,
                             UUID phys_obj_key,
                             UUID rend_obj_key,
                             vector<Pre_physics_script::Script_type>&& pre_physics_scripts,
                             vector<uint64_t>&& pre_physics_user_datas,
                             vector<Pre_render_script::Script_type>&& pre_render_scripts,
                             vector<uint64_t>&& pre_render_user_datas)
    : m_name(name)
    , m_phys_engine(phys_engine)
    , m_renderer(renderer)
    , m_pre_physics_scripts(std::move(pre_physics_scripts))
    , m_pre_physics_user_datas(std::move(pre_physics_user_datas))
    , m_pre_render_scripts(std::move(pre_render_scripts))
    , m_pre_render_user_datas(std::move(pre_render_user_datas))
{
}

void BT::Game_object::run_pre_physics_scripts(float_t physics_delta_time)
{
    size_t read_data_idx{ 0 };
    for (auto pre_phys_script : m_pre_physics_scripts)
    {
        BT::Pre_physics_script::execute_pre_physics_script(&m_phys_engine,
                                                           pre_phys_script,
                                                           m_pre_physics_user_datas,
                                                           read_data_idx);
    }
}

void BT::Game_object::run_pre_render_scripts(float_t delta_time)
{
    size_t read_data_idx{ 0 };
    for (auto pre_rend_script : m_pre_render_scripts)
    {
        BT::Pre_render_script::execute_pre_render_script(&m_renderer,
                                                         pre_rend_script,
                                                         m_pre_render_user_datas,
                                                         read_data_idx);
    }
}

// Scene_serialization_ifc.
void BT::Game_object::scene_serialize(Scene_serialization_mode mode, json& node_ref)
{
    if (mode == SCENE_SERIAL_MODE_SERIALIZE)
    {
        node_ref["name"] = m_name;
        node_ref["guid"] = UUID_helper::pretty_repr(get_uuid());
        node_ref["children"][0] = "isodesperately-wantajeff-bezosasmy-lovelydovely";
        node_ref["children"][1] = "santaclauseis-cumming-mytown-andbytownwellletsjustsay";

        size_t scripts_idx{ 0 };
        size_t datas_io_idx{ 0 };
        for (auto pre_phys_script : m_pre_physics_scripts)
        {
            node_ref["pre_physics_scripts"][scripts_idx++] =
                Pre_physics_script::get_script_name_from_type(pre_phys_script);

            // Add script datas.
            Pre_physics_script::execute_pre_physics_script_serialize(nullptr,
                                                                     &m_phys_engine,
                                                                     &m_renderer,
                                                                     pre_phys_script,
                                                                     mode,
                                                                     node_ref["pre_physics_scripts_datas"],
                                                                     m_pre_physics_user_datas,
                                                                     datas_io_idx);
        }

        scripts_idx = 0;
        datas_io_idx = 0;
        for (auto pre_rend_script : m_pre_render_scripts)
        {
            node_ref["pre_render_scripts"][scripts_idx++] =
                Pre_render_script::get_script_name_from_type(pre_rend_script);

            // @TODO: Add script datas.
        }
    }
    else if (mode == SCENE_SERIAL_MODE_DESERIALIZE)
    {
        // @TODO.
        assert(false);
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
        logger::printef(logger::ERROR, "UUID was invalid: %s", UUID_helper::pretty_repr(uuid).c_str());
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
