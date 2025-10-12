#pragma once

#include "../input_handler/input_handler.h"
#include "../physics_engine/physics_engine.h"
#include "../physics_engine/rvec3.h"
#include "../renderer/renderer.h"
#include "../scene/scene_serialization_ifc.h"
#include "../uuid/uuid_ifc.h"
#include "cglm/affine.h"
#include "cglm/quat.h"
#include "component_registry.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


using std::atomic_bool;
using std::atomic_uint8_t;
using std::atomic_uint64_t;
using std::function;
using std::pair;
using std::string;
using std::unique_ptr;
using std::unordered_map;
using std::vector;


namespace BT
{

struct Transform_data : public Scene_serialization_ifc
{
    rvec3  position{ 0.0, 0.0, 0.0 };
    versor rotation = GLM_QUAT_IDENTITY_INIT;
    vec3   scale{ 1.0f, 1.0f, 1.0f };

    // Scene_serialization_ifc.
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

    Transform_data append_transform(Transform_data next);
    Transform_data calc_inverse();
};

class Game_object_transform : public Scene_serialization_ifc
{
public:
    #define BT_GAME_OBJECT_TRANSFORM_SET_POSITION_CONTENTS(x)  x.position[0] = position[0]; \
                                                               x.position[1] = position[1]; \
                                                               x.position[2] = position[2]
    #define BT_GAME_OBJECT_TRANSFORM_SET_ROTATION_CONTENTS(x)  glm_quat_copy(rotation, x.rotation)
    #define BT_GAME_OBJECT_TRANSFORM_SET_SCALE_CONTENTS(x)  glm_vec3_copy(scale, x.scale)

    #define BT_GAME_OBJECT_TRANSFORM_SET_FUNCTIONS(x)                           \
    void set_##x##_pos_rot_sca(rvec3 position, versor rotation, vec3 scale)     \
    {                                                                           \
        assert(m_dirty_flag == k_not_dirty);                                    \
        BT_GAME_OBJECT_TRANSFORM_SET_POSITION_CONTENTS(m_##x##_transform);      \
        BT_GAME_OBJECT_TRANSFORM_SET_ROTATION_CONTENTS(m_##x##_transform);      \
        BT_GAME_OBJECT_TRANSFORM_SET_SCALE_CONTENTS(m_##x##_transform);         \
        m_dirty_flag = k_##x##_trans_dirty;                                     \
    }                                                                           \
                                                                                \
    void set_##x##_pos_rot(rvec3 position, versor rotation)                     \
    {                                                                           \
        assert(m_dirty_flag == k_not_dirty);                                    \
        BT_GAME_OBJECT_TRANSFORM_SET_POSITION_CONTENTS(m_##x##_transform);      \
        BT_GAME_OBJECT_TRANSFORM_SET_ROTATION_CONTENTS(m_##x##_transform);      \
        m_dirty_flag = k_##x##_trans_dirty;                                     \
    }                                                                           \
                                                                                \
    void set_##x##_pos(rvec3 position)                                          \
    {                                                                           \
        assert(m_dirty_flag == k_not_dirty);                                    \
        BT_GAME_OBJECT_TRANSFORM_SET_POSITION_CONTENTS(m_##x##_transform);      \
        m_dirty_flag = k_##x##_trans_dirty;                                     \
    }                                                                           \
                                                                                \
    void set_##x##_rot(versor rotation)                                         \
    {                                                                           \
        assert(m_dirty_flag == k_not_dirty);                                    \
        BT_GAME_OBJECT_TRANSFORM_SET_ROTATION_CONTENTS(m_##x##_transform);      \
        m_dirty_flag = k_##x##_trans_dirty;                                     \
    }                                                                           \
                                                                                \
    void set_##x##_sca(vec3 scale)                                              \
    {                                                                           \
        assert(m_dirty_flag == k_not_dirty);                                    \
        BT_GAME_OBJECT_TRANSFORM_SET_SCALE_CONTENTS(m_##x##_transform);         \
        m_dirty_flag = k_##x##_trans_dirty;                                     \
    }

    BT_GAME_OBJECT_TRANSFORM_SET_FUNCTIONS(global)
    BT_GAME_OBJECT_TRANSFORM_SET_FUNCTIONS(local)
    #undef BT_GAME_OBJECT_TRANSFORM_SET_FUNCTIONS

    #undef BT_GAME_OBJECT_TRANSFORM_SET_POSITION_CONTENTS
    #undef BT_GAME_OBJECT_TRANSFORM_SET_ROTATION_CONTENTS
    #undef BT_GAME_OBJECT_TRANSFORM_SET_SCALE_CONTENTS

    #define BT_GAME_OBJECT_TRANSFORM_GET_DECOMPOSED_FUNC(x)                     \
    void get_##x##_transform_decomposed_data(rvec3& out_position,               \
                                             versor& out_rotation,              \
                                             vec3& out_scale)                   \
    {                                                                           \
        assert(m_dirty_flag == k_not_dirty);                                    \
        out_position[0] = m_##x##_transform.position[0];                        \
        out_position[1] = m_##x##_transform.position[1];                        \
        out_position[2] = m_##x##_transform.position[2];                        \
        glm_quat_copy(m_##x##_transform.rotation, out_rotation);                \
        glm_vec3_copy(m_##x##_transform.scale, out_scale);                      \
    }

    BT_GAME_OBJECT_TRANSFORM_GET_DECOMPOSED_FUNC(global)
    BT_GAME_OBJECT_TRANSFORM_GET_DECOMPOSED_FUNC(local)
    #undef BT_GAME_OBJECT_TRANSFORM_GET_DECOMPOSED_FUNC

    void get_position(vec3& out_position)
    {
        out_position[0] = m_global_transform.position[0];
        out_position[1] = m_global_transform.position[1];
        out_position[2] = m_global_transform.position[2];
    }

    void get_transform_as_mat4(mat4& out_transform)
    {
        assert(m_dirty_flag == k_not_dirty);
        // @TODO: Include camera partitioning. Or camera position subtraction.
        glm_translate_make(out_transform, vec3{ m_global_transform.position[0],
                                                m_global_transform.position[1],
                                                m_global_transform.position[2] });
        glm_quat_rotate(out_transform, m_global_transform.rotation, out_transform);
        glm_scale(out_transform, m_global_transform.scale);
    }

    void mark_propagation_dirty() { assert(m_dirty_flag.load() == k_not_dirty); m_dirty_flag.store(k_propagation_dirty); }
    void mark_new_child_dirty() { assert(m_dirty_flag.load() == k_not_dirty); m_dirty_flag.store(k_new_child_dirty); }
    bool update_to_clean(Game_object_transform* parent_transform);

    // Scene_serialization_ifc.
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

private:
    static constexpr uint8_t k_not_dirty          = 0;
    static constexpr uint8_t k_global_trans_dirty = 1;
    static constexpr uint8_t k_local_trans_dirty  = 2;
    static constexpr uint8_t k_propagation_dirty  = 3;
    static constexpr uint8_t k_new_child_dirty    = 4;
    atomic_uint8_t m_dirty_flag{ k_not_dirty };

    Transform_data m_global_transform;
    Transform_data m_local_transform;
};

class Game_object_pool;

class Game_object : public Scene_serialization_ifc, public UUID_ifc
{
public:
    Game_object(Input_handler& input_handler,
                Physics_engine& phys_engine,
                Renderer& renderer,
                Game_object_pool& obj_pool);
    ~Game_object();

    static void set_imgui_gizmo_trans_space(int32_t trans_space) { s_imgui_gizmo_trans_space = trans_space; }
    static int32_t get_imgui_gizmo_trans_space() { return s_imgui_gizmo_trans_space; }

    #if 0
    void run_pre_physics_scripts(float_t physics_delta_time);
    void run_pre_render_scripts(float_t delta_time);
    #endif  // 0

    #if 0
    // @WARNING: Not for production code.
    // Processes thru all requests for mutating the script list, removing first.
    void process_script_list_mutation_requests();
    #endif  // 0

    void set_name(string&& name);
    string get_name();
    UUID get_phys_obj_key();
    UUID get_rend_obj_key();
    UUID get_parent_uuid();
    vector<UUID> get_children_uuids();
    void insert_child(Game_object& new_child, size_t position = 0);
    void remove_child(Game_object& remove_child);

    inline Game_object_transform& get_transform_handle() { return m_transform; }
    void propagate_transform_changes(Game_object* parent_game_object = nullptr);

    // Scene_serialization_ifc.
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

    #if 0
    // @WARNING: Not for production code.
    // Adds a script dynamically to this gameobject.
    void add_script(json& node_ref);

    // @WARNING: Not for production code.
    // Removes a script dynamically from this gameobject.
    void remove_script(std::string const& script_type);
    #endif  // 0

    void render_imgui_local_transform();
    void render_imgui_transform_gizmo();

private:
    Input_handler& m_input_handler;
    Physics_engine& m_phys_engine;
    Renderer& m_renderer;
    Game_object_pool& m_obj_pool;

    inline static int32_t s_imgui_gizmo_trans_space{ 1 };

    Game_object_transform m_transform;

    string m_name;
    UUID m_phys_obj_key;
    UUID m_rend_obj_key;
    
    // vector<unique_ptr<Scripts::Script_ifc>> m_scripts;  @NOCHECKIN: @THEA
    component_system::Component_list m_component_list;
    // @TODO: ^^Keep^^ this in mind as the replac ///////  @NOCHECKIN: @THEA

    #if 0
    // @WARNING: Not for production code.
    std::vector<std::string> m_remove_script_requests;
    // @WARNING: Not for production code.
    std::vector<json> m_add_script_requests;
    // @WARNING: Not for production code.
    std::mutex m_mutate_script_requests_mutex;
    #endif  // 0

    UUID m_parent;
    vector<UUID> m_children;
};

// vv See below ~~@COPYPASTA. See "mesh.h" "material.h" "shader.h"~~
// @NOTE: Not quite copypasta. It's a little bit different.
class Game_object_pool : public Scene_serialization_ifc
{
public:
    Game_object_pool();

    void set_callback_fn(function<unique_ptr<Game_object>()>&& create_new_empty_game_obj_callback_fn);

    UUID emplace(unique_ptr<Game_object>&& game_object);
    void remove(UUID key);

    vector<Game_object*> const checkout_all_as_list();
    vector<Game_object*> const get_all_as_list_no_lock();
    Game_object* checkout_one(UUID key);
    Game_object* get_one_no_lock(UUID key);
    void return_list(vector<Game_object*> const&& all_as_list);

    // Scene_serialization_ifc.
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

    // Debug ImGui.
    void render_imgui_scene_hierarchy();

    void set_selected_game_obj(Game_object* game_object)
    {
        m_selected_game_obj = (game_object ? game_object->get_uuid() : UUID());
    }

    UUID get_selected_game_obj() { return m_selected_game_obj; }

private:
    UUID emplace_no_lock(unique_ptr<Game_object>&& game_object);

    void remove_root_level_status(UUID key);

    bool validate_game_object_hierarchy();

    unordered_map<UUID, unique_ptr<Game_object>> m_game_objects;
    vector<UUID> m_root_level_game_objects_ordering;

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
            INSERT_AT_END,
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
