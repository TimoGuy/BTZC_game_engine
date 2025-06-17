#include "game_object.h"

#include "../input_handler/input_handler.h"
#include "../physics_engine/physics_engine.h"
#include "../renderer/renderer.h"
#include "cglm/euler.h"
#include "cglm/mat4.h"
#include "cglm/quat.h"
#include "cglm/vec3.h"
#include "imgui.h"
#include "imgui_internal.h"
#include "logger.h"
#include "misc/cpp/imgui_stdlib.h"
#include "scripts/scripts.h"
#include <atomic>
#include <cassert>

using std::atomic_uint8_t;
using std::atomic_uint64_t;
using std::max;


void BT::Transform_data::scene_serialize(Scene_serialization_mode mode,
                                         json& node_ref)
{
    if (mode == SCENE_SERIAL_MODE_DESERIALIZE)
    {
        node_ref["position"][0] = position[0];
        node_ref["position"][1] = position[1];
        node_ref["position"][2] = position[2];
        node_ref["rotation"][0] = rotation[0];
        node_ref["rotation"][1] = rotation[1];
        node_ref["rotation"][2] = rotation[2];
        node_ref["rotation"][3] = rotation[3];
        node_ref["scale"][0]    = scale[0];
        node_ref["scale"][1]    = scale[1];
        node_ref["scale"][2]    = scale[2];
    }
    else if (mode == SCENE_SERIAL_MODE_DESERIALIZE)
    {
        position[0] = node_ref["position"][0];
        position[1] = node_ref["position"][1];
        position[2] = node_ref["position"][2];
        rotation[0] = node_ref["rotation"][0];
        rotation[1] = node_ref["rotation"][1];
        rotation[2] = node_ref["rotation"][2];
        rotation[3] = node_ref["rotation"][3];
        scale[0]    = node_ref["scale"][0];
        scale[1]    = node_ref["scale"][1];
        scale[2]    = node_ref["scale"][2];
    }
}

BT::Transform_data BT::Transform_data::append_transform(Transform_data next)
{
    #define GLM_RVEC3_SCALE_V3(a, v, dest)                                      \
    dest[0] = a[0] * v[0];                                                      \
    dest[1] = a[1] * v[1];                                                      \
    dest[2] = a[2] * v[2]

    #define GLM_MAT3_MUL_RVEC3(m, v, dest)                                      \
    do {                                                                        \
    rvec3 temp;                                                                 \
    temp[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2];                 \
    temp[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2];                 \
    temp[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2];                 \
    dest[0] = temp[0];                                                          \
    dest[1] = temp[1];                                                          \
    dest[2] = temp[2];                                                          \
    } while(false)

    #define GLM_RVEC3_ADD(a, b, dest)                                           \
    dest[0] = a[0] + b[0];                                                      \
    dest[1] = a[1] + b[1];                                                      \
    dest[2] = a[2] + b[2]

    Transform_data result;

    mat3 my_rot_mat3;
    glm_quat_mat3(rotation, my_rot_mat3);

    GLM_RVEC3_SCALE_V3(next.position, scale, result.position);
    GLM_MAT3_MUL_RVEC3(my_rot_mat3, result.position, result.position);
    GLM_RVEC3_ADD(position, result.position, result.position);

    glm_quat_mul(next.rotation, rotation, result.rotation);  // @NOTE: p is the rotation after q.

    glm_vec3_mul(next.scale, scale, result.scale);

    #undef GLM_RVEC3_SCALE_V3
    #undef GLM_MAT3_MUL_RVEC3
    #undef GLM_RVEC3_ADD

    return result;
}

BT::Transform_data BT::Transform_data::calc_inverse()
{
    Transform_data result;
    result.position[0] = -position[0];
    result.position[1] = -position[1];
    result.position[2] = -position[2];
    glm_quat_inv(rotation, result.rotation);
    result.scale[0] = 1.0f / scale[0];
    result.scale[1] = 1.0f / scale[1];
    result.scale[2] = 1.0f / scale[2];
    return result;
}

bool BT::Game_object_transform::update_to_clean(Game_object_transform* parent_transform)
{
    bool changed{ false };
    switch (m_dirty_flag.load())
    {
        case k_not_dirty:
            // Do nothing.
            break;

        case k_global_trans_dirty:
            // Update local transform.
            m_local_transform = (parent_transform == nullptr ?
                                 m_global_transform :
                                 parent_transform->m_global_transform
                                     .calc_inverse()
                                     .append_transform(m_global_transform));
            changed = true;
            break;

        case k_local_trans_dirty:
            // Update global transform.
            m_global_transform = (parent_transform == nullptr ?
                                  m_local_transform :
                                  parent_transform->m_global_transform
                                      .append_transform(m_local_transform));
            changed = true;
            break;
    }

    m_dirty_flag.store(k_not_dirty);
    return changed;
}

void BT::Game_object_transform::scene_serialize(Scene_serialization_mode mode, json& node_ref)
{
    m_global_transform.scene_serialize(mode, node_ref["global"]);
    m_local_transform.scene_serialize(mode, node_ref["local"]);
}

BT::Game_object::Game_object(Input_handler& input_handler,
                             Physics_engine& phys_engine,
                             Renderer& renderer,
                             Game_object_pool& obj_pool)
    : m_input_handler(input_handler)
    , m_phys_engine(phys_engine)
    , m_renderer(renderer)
    , m_obj_pool(obj_pool)
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

void BT::Game_object::set_name(string&& name)
{
    m_name = std::move(name);
}

string BT::Game_object::get_name()
{
    return m_name;
}

BT::UUID BT::Game_object::get_phys_obj_key()
{
    return m_phys_obj_key;
}

BT::UUID BT::Game_object::get_rend_obj_key()
{
    return m_rend_obj_key;
}

BT::UUID BT::Game_object::get_parent_uuid()
{
    return m_parent;
}

vector<BT::UUID> BT::Game_object::get_children_uuids()
{
    return m_children;
}

void BT::Game_object::insert_child(Game_object& new_child, size_t position /*= 0*/)
{
    new_child.m_parent = get_uuid();
    m_children.insert(m_children.begin() + position, new_child.get_uuid());
}

void BT::Game_object::remove_child(Game_object& remove_child)
{
    for (auto it = m_children.begin(); it != m_children.end(); it++)
    {
        auto child_uuid{ *it };
        if (child_uuid == remove_child.get_uuid())
        {
            // Sever parent-child relationship.
            remove_child.m_parent = UUID();
            m_children.erase(it);
            return;
        }
    }

    logger::printe(logger::ERROR, "Unchilding game object is not a child.");
    assert(false);
}

void BT::Game_object::propagate_transform_changes(Game_object* parent_game_object /*= nullptr*/)
{
    bool propagate_dirty{ false };
    if (parent_game_object == nullptr && !m_parent.is_nil())
    {
        // Try querying for the parent game object.
        parent_game_object = m_obj_pool.get_one_no_lock(m_parent);
    }

    propagate_dirty = m_transform.update_to_clean(parent_game_object == nullptr ?
                                                  nullptr :
                                                  &parent_game_object->m_transform);
    for (auto child : m_children)
    {
        auto child_go{ m_obj_pool.get_one_no_lock(child) };
        if (propagate_dirty)
            child_go->m_transform.mark_dirty();
        child_go->propagate_transform_changes(this);
    }
}

// Scene_serialization_ifc.
void BT::Game_object::scene_serialize(Scene_serialization_mode mode, json& node_ref)
{
    m_transform.scene_serialize(mode, node_ref["transform"]);

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

        if (m_parent.is_nil())
            node_ref["parent"] = nullptr;
        else
            node_ref["parent"] = UUID_helper::to_pretty_repr(m_parent);

        node_ref["children"] = json::array();
        for (auto& child_uuid : m_children)
        {
            node_ref["children"].emplace_back(UUID_helper::to_pretty_repr(child_uuid));
        }

        // Serialize physics obj.
        if (m_phys_obj_key.is_nil())
        {
            node_ref["physics_obj"] = nullptr;
        }
        else
        {
            auto phys_obj{ m_phys_engine.checkout_physics_object(m_phys_obj_key) };
            phys_obj->scene_serialize(mode, node_ref["physics_obj"]);
            m_phys_engine.return_physics_object(phys_obj);
        }

        // Serialize render obj.
        if (m_rend_obj_key.is_nil())
        {
            node_ref["render_obj"] = nullptr;
        }
        else
        {
            auto rend_objs{
                m_renderer.get_render_object_pool()
                    .checkout_render_obj_by_key({ m_rend_obj_key }) };
            assert(rend_objs.size() == 1);
            rend_objs.front()->scene_serialize(mode, node_ref["render_obj"]);
            m_renderer.get_render_object_pool()
                .return_render_objs(std::move(rend_objs));
        }
    }
    else if (mode == SCENE_SERIAL_MODE_DESERIALIZE)
    {
        m_name = node_ref["name"];
        assign_uuid(node_ref["guid"], true);

        if (!node_ref["scripts"].is_null())
        {
            assert(node_ref["scripts"].is_array());
            for (size_t scripts_idx = 0; scripts_idx < node_ref["scripts"].size();)
            {
                m_scripts.emplace_back(
                    Scripts::create_script_from_serialized_datas(&m_input_handler,
                                                                 &m_phys_engine,
                                                                 &m_renderer,
                                                                 &m_obj_pool,
                                                                 node_ref["scripts"][scripts_idx++]));
            }
        }

        m_parent = (node_ref["parent"].is_null() ?
                    UUID() :
                    UUID_helper::to_UUID(node_ref["parent"]));

        if (!node_ref["children"].is_null())
        {
            assert(node_ref["children"].is_array());
            for (auto& child_uuid : node_ref["children"])
            {
                m_children.emplace_back(UUID_helper::to_UUID(child_uuid));
            }
        }

        // Deserialize physics obj.
        if (node_ref["physics_obj"].is_object())
        {
            auto new_phys_obj{
                Physics_object::create_physics_object_from_serialization(*this,
                                                                         m_phys_engine,
                                                                         node_ref["physics_obj"]) };
            m_phys_obj_key =
                m_phys_engine.emplace_physics_object(std::move(new_phys_obj));
        }

        // Deserialize render obj.
        if (node_ref["render_obj"].is_object())
        {
            Render_object new_rend_obj{ *this,
                                        nullptr,
                                        Render_layer::RENDER_LAYER_DEFAULT };
            new_rend_obj.scene_serialize(mode, node_ref["render_obj"]);
            m_rend_obj_key =
                m_renderer.get_render_object_pool().emplace(std::move(new_rend_obj));
        }
    }
}

void BT::Game_object::render_imgui_local_transform()
{
    rvec3 pos;
    versor rot;
    vec3 sca;
    m_transform.get_local_transform_decomposed_data(pos, rot, sca);

    bool changed{ false };

    if (ImGui::DragScalarN("Position",
                           (sizeof(pos[0]) == sizeof(float_t) ?
                                ImGuiDataType_Float :
                                ImGuiDataType_Double),
                           pos,
                           3))
    {
        // Apply position.
        m_transform.set_local_pos(pos);
        changed = true;
    }

    mat4 rot_mat;
    glm_quat_mat4(rot, rot_mat);
    vec3 euler_angles;
    glm_euler_angles(rot_mat, euler_angles);
    euler_angles[0] = glm_deg(euler_angles[0]);
    euler_angles[1] = glm_deg(euler_angles[1]);
    euler_angles[2] = glm_deg(euler_angles[2]);
    if (ImGui::DragFloat3("Rotation", euler_angles))
    {
        // Apply rotation.
        euler_angles[0] = glm_rad(euler_angles[0]);
        euler_angles[1] = glm_rad(euler_angles[1]);
        euler_angles[2] = glm_rad(euler_angles[2]);
        mat4 rot_mat_apply;
        // glm_euler_zyx(euler_angles, rot_mat_apply);
        glm_euler_xyz(euler_angles, rot_mat_apply);
        versor rot_quat_apply;
        glm_mat4_quat(rot_mat_apply, rot_quat_apply);
        m_transform.set_local_rot(rot_quat_apply);
        changed = true;
    }

    if (ImGui::DragFloat3("Scale", sca))
    {
        // Apply scale.
        m_transform.set_local_sca(sca);
        changed = true;
    }

    if (changed)
    {
        // Update dirty transform.
        propagate_transform_changes();
    }
}


void BT::Game_object_pool::set_callback_fn(
    function<unique_ptr<Game_object>()>&& create_new_empty_game_obj_callback_fn)
{
    m_create_new_empty_game_obj_callback_fn = std::move(create_new_empty_game_obj_callback_fn);
}

BT::UUID BT::Game_object_pool::emplace(unique_ptr<Game_object>&& game_object)
{
    wait_until_free_then_block();
    UUID uuid{ emplace_no_lock(std::move(game_object)) };
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

    if (m_game_objects.at(key)->get_parent_uuid().is_nil())
        remove_root_level_status(key);

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
    return get_one_no_lock(uuid);
}

BT::Game_object* BT::Game_object_pool::get_one_no_lock(UUID uuid)
{
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

// Scene_serialization_ifc.
void BT::Game_object_pool::scene_serialize(Scene_serialization_mode mode, json& node_ref)
{
    if (mode == SCENE_SERIAL_MODE_SERIALIZE)
    {

    }
    else if (mode == SCENE_SERIAL_MODE_DESERIALIZE)
    {
        
    }
}

// Debug ImGui.
namespace
{

struct Hierarchy_node
{
    bool is_root{ true };
    BT::Game_object* game_obj{ nullptr };
    vector<Hierarchy_node*> children;
};

}  // namespace

void BT::Game_object_pool::render_imgui_scene_hierarchy()
{
    // @NOTE: Called from `renderer.render()` so game objs already checked out.
    // wait_until_free_then_block();
    
    // Gather structured hierarchy.
    vector<Hierarchy_node> flat_scene_hierarchy;
    vector<Hierarchy_node*> root_nodes_scene_hierarchy;

    for (auto& game_obj : m_game_objects)
        flat_scene_hierarchy.emplace_back(true,
                                     game_obj.second.get());

    for (auto& scene_node : flat_scene_hierarchy)
        root_nodes_scene_hierarchy.emplace_back(&scene_node);
    
    for (auto& scene_node : flat_scene_hierarchy)
        for(auto child_uuid : scene_node.game_obj->get_children_uuids())
            for (size_t i = 0; i < root_nodes_scene_hierarchy.size(); i++)
            {
                auto root_node{ root_nodes_scene_hierarchy[i] };
                if (child_uuid == root_node->game_obj->get_uuid())
                {
                    // Turns out wasn't a root node.
                    root_node->is_root = false;
                    scene_node.children.emplace_back(root_node);
                    root_nodes_scene_hierarchy.erase(root_nodes_scene_hierarchy.begin() + i);
                    break;
                }
            }
    
    // Reorder root nodes list according to provided uuid list.
    assert(m_root_level_game_objects_ordering.size() == root_nodes_scene_hierarchy.size());
    vector<Hierarchy_node*> root_nodes_ordered_scene_hierarchy;
    root_nodes_ordered_scene_hierarchy.reserve(root_nodes_scene_hierarchy.size());
    for (auto root_uuid : m_root_level_game_objects_ordering)
        for (auto it = root_nodes_scene_hierarchy.begin(); it != root_nodes_scene_hierarchy.end(); it++)
        {
            auto root_node{ *it };
            if (root_uuid == root_node->game_obj->get_uuid())
            {
                // Move to the back of the ordered list.
                root_nodes_ordered_scene_hierarchy.emplace_back(root_node);
            }
        }
    assert(root_nodes_scene_hierarchy.size() == root_nodes_ordered_scene_hierarchy.size());
    root_nodes_scene_hierarchy.clear();

    // Draw out scene hierarchy.
    auto next_id{ reinterpret_cast<intptr_t>(this) };
    ImGui::Begin("Scene hierarchy");

    // Reset selected obj when clicking.
    if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered())
        m_selected_game_obj = UUID();

    if (ImGui::Button("Create new empty##create_new_game_obj"))
    {
        // Create new empty game obj.
        emplace_no_lock(m_create_new_empty_game_obj_callback_fn());
    }

    ImGui::Separator();

    ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, 0.0f);

    Modify_scene_hierarchy_action modify_action;

    // Draw scene nodes.
    for (auto root_node : root_nodes_ordered_scene_hierarchy)
    {
        render_imgui_scene_hierarchy_node_recursive(root_node, modify_action, next_id);
    }

    // Draw final after node.
    ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0x00000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0x00000000);
    ImGui::ButtonEx(("##between_node_button__" + std::to_string(next_id)).c_str(),
                    ImVec2(ImGui::GetContentRegionAvail().x,
                           max(6.0f, ImGui::GetContentRegionAvail().y)),
                    ImGuiButtonFlags_NoNavFocus);
    ImGui::PopStyleColor(3);
    if (ImGui::BeginDragDropTarget())
    {
        ImGuiDragDropFlags drop_target_flags = 0;
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_GAMEOBJ_TREENODE", drop_target_flags))
        {
            modify_action.commit = true;
            modify_action.type = Modify_scene_hierarchy_action::INSERT_AT_END;
            modify_action.modifying_object = *reinterpret_cast<UUID*>(payload->Data);
        }
        ImGui::EndDragDropTarget();
    }

    ImGui::PopStyleVar();

    ImGui::End();

    // Draw out inspector (if >0 objs are selected).
    ImGui::Begin("Properties inspector");
    if (!m_selected_game_obj.is_nil())
    {
        auto game_obj{ m_game_objects.at(m_selected_game_obj).get() };

        auto name{ game_obj->get_name() };
        if (ImGui::InputText("Name", &name))
            game_obj->set_name(std::move(name));

        ImGui::Text("UUID: %s", UUID_helper::to_pretty_repr(game_obj->get_uuid()).c_str());

        if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
        {
            game_obj->render_imgui_local_transform();
        }
    }
    else
    {
        ImGui::Text("Select a game object to inspect its properties.");
    }
    ImGui::End();

    // Commit modify action if made.
    if (modify_action.commit)
    {
        switch (modify_action.type)
        {
            case Modify_scene_hierarchy_action::INSERT_AS_CHILD:
            {
                auto& modifying_obj_game_obj{
                    m_game_objects.at(modify_action.modifying_object) };

                // @COPYPASTA: Appears 3 times vv.
                if (modifying_obj_game_obj->get_parent_uuid().is_nil())
                    remove_root_level_status(modifying_obj_game_obj->get_uuid());
                else
                {
                    // Goto parent of `modifying_object` and sever parent-child relationship.
                    m_game_objects.at(modifying_obj_game_obj->get_parent_uuid())
                        ->remove_child(*modifying_obj_game_obj);
                }
                // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

                // Add new parent-child relationship.
                m_game_objects.at(modify_action.anchor_subject)
                    ->insert_child(*modifying_obj_game_obj);
                break;
            }

            case Modify_scene_hierarchy_action::INSERT_BEFORE:
            {
                if (modify_action.modifying_object == modify_action.anchor_subject)
                    // Cancel operation if moving the object to the same exact place.
                    break;

                auto& modifying_obj_game_obj{
                    m_game_objects.at(modify_action.modifying_object) };

                // @COPYPASTA: Appears 3 times vv.
                if (modifying_obj_game_obj->get_parent_uuid().is_nil())
                    remove_root_level_status(modifying_obj_game_obj->get_uuid());
                else
                {
                    // Goto parent of `modifying_object` and sever parent-child relationship.
                    m_game_objects.at(modifying_obj_game_obj->get_parent_uuid())
                        ->remove_child(*modifying_obj_game_obj);
                }
                // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

                auto anchor_parent_uuid{
                    m_game_objects.at(modify_action.anchor_subject)->get_parent_uuid() };
                if (anchor_parent_uuid.is_nil())
                {
                    // Add game object as a root level object.
                    for (auto it = m_root_level_game_objects_ordering.begin();
                         it != m_root_level_game_objects_ordering.end(); it++)
                        if (*it == modify_action.anchor_subject)
                        {
                            // Insert placement to before this index.
                            m_root_level_game_objects_ordering.emplace(
                                it,
                                modifying_obj_game_obj->get_uuid());
                            break;
                        }
                }
                else
                {
                    // Add new parent-child relationship (so that modify-GO is sibling of anchor-GO).
                    size_t position{ 0 };
                    auto& anchor_parent_game_obj{ m_game_objects.at(anchor_parent_uuid) };
                    auto children_uuids{ anchor_parent_game_obj->get_children_uuids() };
                    for (; position < children_uuids.size(); position++)
                        if (children_uuids[position] == modify_action.anchor_subject)
                        {
                            m_game_objects.at(anchor_parent_uuid)
                                ->insert_child(*modifying_obj_game_obj, position);
                            break;
                        }
                }

                break;
            }

            case Modify_scene_hierarchy_action::INSERT_AT_END:
            {
                auto& modifying_obj_game_obj{
                    m_game_objects.at(modify_action.modifying_object) };

                // @COPYPASTA: Appears 3 times vv.
                if (modifying_obj_game_obj->get_parent_uuid().is_nil())
                    remove_root_level_status(modifying_obj_game_obj->get_uuid());
                else
                {
                    // Goto parent of `modifying_object` and sever parent-child relationship.
                    m_game_objects.at(modifying_obj_game_obj->get_parent_uuid())
                        ->remove_child(*modifying_obj_game_obj);
                }
                // ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

                // Add game object as a root level object at end.
                m_root_level_game_objects_ordering.emplace_back(modifying_obj_game_obj->get_uuid());

                break;
            }

            default:
                assert(false);
                break;
        }
    }

    // @NOTE: Called from `renderer.render()` so game objs already checked out.
    // unblock();
}

BT::UUID BT::Game_object_pool::emplace_no_lock(unique_ptr<Game_object>&& game_object)
{
    UUID uuid{ game_object->get_uuid() };
    if (uuid.is_nil())
    {
        logger::printe(logger::ERROR, "Invalid UUID passed in.");
        assert(false);
    }

    if (game_object->get_parent_uuid().is_nil())
    {
        // Add game object as a root level object.
        m_root_level_game_objects_ordering.emplace_back(uuid);
    }

    m_game_objects.emplace(uuid, std::move(game_object));

    return uuid;
}

void BT::Game_object_pool::remove_root_level_status(UUID key)
{
    m_root_level_game_objects_ordering.erase(
        std::remove(m_root_level_game_objects_ordering.begin(),
                    m_root_level_game_objects_ordering.end(),
                    key),
        m_root_level_game_objects_ordering.end());
}

void BT::Game_object_pool::render_imgui_scene_hierarchy_node_recursive(void* node_void_ptr,
                                                                       Modify_scene_hierarchy_action& modify_action,
                                                                       intptr_t& next_id)
{
    auto node{ reinterpret_cast<Hierarchy_node*>(node_void_ptr) };  // I like the stink.  -Thea 2025/06/03

    constexpr ImGuiTreeNodeFlags k_base_flags{ ImGuiTreeNodeFlags_SpanAvailWidth |
                                               ImGuiTreeNodeFlags_DrawLinesToNodes |
                                               ImGuiTreeNodeFlags_DefaultOpen };
    ImGuiTreeNodeFlags node_flags{ k_base_flags };

    auto cur_uuid{ node->game_obj->get_uuid() };
    if (cur_uuid == m_selected_game_obj)
        node_flags |= ImGuiTreeNodeFlags_Selected;

    auto const& child_nodes{ node->children };
    bool treat_as_leaf_node{ child_nodes.empty() };
    if (treat_as_leaf_node)
        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    // Draw between/before node.
    ImGui::PushStyleColor(ImGuiCol_Button, 0x00000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, 0x00000000);
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, 0x00000000);
    ImGui::ButtonEx(("##between_node_button__" + std::to_string(next_id)).c_str(),
                    ImVec2(ImGui::GetContentRegionAvail().x, 6),
                    ImGuiButtonFlags_NoNavFocus);
    ImGui::PopStyleColor(3);
    if (ImGui::BeginDragDropTarget())
    {
        ImGuiDragDropFlags drop_target_flags = 0;
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_GAMEOBJ_TREENODE", drop_target_flags))
        {
            modify_action.commit = true;
            modify_action.type = Modify_scene_hierarchy_action::INSERT_BEFORE;
            modify_action.anchor_subject = node->game_obj->get_uuid();
            modify_action.modifying_object = *reinterpret_cast<UUID*>(payload->Data);
        }
        ImGui::EndDragDropTarget();
    }

    // Draw my node.
    bool tree_node_open = ImGui::TreeNodeEx(reinterpret_cast<void*>(next_id),
                                            node_flags,
                                            "%s",
                                            node->game_obj->get_name().c_str());
    if (ImGui::IsItemClicked())
        m_selected_game_obj = cur_uuid;
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("_GAMEOBJ_TREENODE", &m_selected_game_obj, sizeof(m_selected_game_obj));
        ImGui::Text("This is a drag and drop source");
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget())
    {
        ImGuiDragDropFlags drop_target_flags = 0;
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("_GAMEOBJ_TREENODE", drop_target_flags))
        {
            modify_action.commit = true;
            modify_action.type = Modify_scene_hierarchy_action::INSERT_AS_CHILD;
            modify_action.anchor_subject = node->game_obj->get_uuid();
            modify_action.modifying_object = *reinterpret_cast<UUID*>(payload->Data);
        }
        ImGui::EndDragDropTarget();
    }

    // Increment id.
    next_id++;

    if (tree_node_open && !treat_as_leaf_node)
    {
        // Draw children nodes.
        for (auto child_node : child_nodes)
        {
            render_imgui_scene_hierarchy_node_recursive(child_node, modify_action, next_id);
        }
        ImGui::TreePop();
    }
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
