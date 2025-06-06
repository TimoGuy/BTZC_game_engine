#pragma once

#include "../input_handler/input_handler.h"
#include "../physics_engine/physics_engine.h"
#include "../physics_engine/physics_object.h"
#include "../renderer/renderer.h"
#include "../renderer/render_object.h"
#include "../scene/scene_serialization_ifc.h"
#include "../uuid/uuid_ifc.h"
#include "scripts/scripts.h"
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

using std::atomic_bool;
using std::atomic_uint64_t;
using std::function;
using std::pair;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;


namespace BT
{

class Game_object : public Scene_serialization_ifc, public UUID_ifc
{
public:
    Game_object(Input_handler& input_handler,
                Physics_engine& phys_engine,
                Renderer& renderer);

    void run_pre_physics_scripts(float_t physics_delta_time);
    void run_pre_render_scripts(float_t delta_time);

    void set_name(string&& name);
    string get_name();
    UUID get_parent_uuid();
    vector<UUID> get_children_uuids();
    void insert_child(Game_object& new_child, size_t position = 0);
    void remove_child(Game_object& remove_child);

    // Scene_serialization_ifc.
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

private:
    Input_handler& m_input_handler;
    Physics_engine& m_phys_engine;
    Renderer& m_renderer;

    string m_name;
    UUID m_phys_obj_key;
    UUID m_rend_obj_key;
    vector<unique_ptr<Scripts::Script_ifc>> m_scripts;

    UUID m_parent;
    vector<UUID> m_children;
};

// vv See below ~~@COPYPASTA. See "mesh.h" "material.h" "shader.h"~~
// @NOTE: Not quite copypasta. It's a little bit different.
class Game_object_pool
{
public:
    Game_object_pool(function<unique_ptr<Game_object>()>&& create_new_empty_game_obj_callback_fn);

    UUID emplace(unique_ptr<Game_object>&& game_object);
    void remove(UUID key);

    vector<Game_object*> const checkout_all_as_list();
    Game_object* checkout_one(UUID key);
    void return_list(vector<Game_object*> const&& all_as_list);

    // Debug ImGui.
    void render_imgui_scene_hierarchy();

private:
    UUID emplace_no_lock(unique_ptr<Game_object>&& game_object);

    unordered_map<UUID, unique_ptr<Game_object>> m_game_objects;

    // Synchronization.
    atomic_bool m_blocked{ false };

    void wait_until_free_then_block();
    void unblock();

    // Debug ImGui data.
    struct Modify_scene_hierarchy_action
    {
        bool commit{ false };
        enum Action_type
        {
            INSERT_AS_CHILD,
            INSERT_BEFORE,
            INSERT_AFTER,
        } type;
        UUID anchor_subject;
        UUID modifying_object;
    };

    function<unique_ptr<Game_object>()> m_create_new_empty_game_obj_callback_fn;
    UUID m_selected_game_obj;
    void render_imgui_scene_hierarchy_node_recursive(void* node_void_ptr,
                                                     Modify_scene_hierarchy_action& modify_action,
                                                     intptr_t& next_id);
};

}  // namespace BT
