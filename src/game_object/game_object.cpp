#include "game_object.h"

#include "../input_handler/input_handler.h"
#include "../physics_engine/physics_engine.h"
#include "../renderer/renderer.h"
#include "imgui.h"
#include "logger.h"
#include "misc/cpp/imgui_stdlib.h"
#include "scripts/scripts.h"
#include <atomic>
#include <cassert>

using std::atomic_uint8_t;
using std::atomic_uint64_t;


BT::Game_object::Game_object(Input_handler& input_handler,
                             Physics_engine& phys_engine,
                             Renderer& renderer)
    : m_input_handler(input_handler)
    , m_phys_engine(phys_engine)
    , m_renderer(renderer)
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

vector<BT::UUID> BT::Game_object::get_children_uuids()
{
    return m_children;
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

        // Deserialize physics obj.
        if (node_ref["physics_obj"].is_object())
        {
            auto new_phys_obj{
                Physics_object::create_physics_object_from_serialization(m_phys_engine,
                                                                         node_ref["physics_obj"]) };
            m_phys_obj_key =
                m_phys_engine.emplace_physics_object(std::move(new_phys_obj));
        }

        // Deserialize render obj.
        if (node_ref["render_obj"].is_object())
        {
            Render_object new_rend_obj{ nullptr,
                                        Render_layer::RENDER_LAYER_DEFAULT,
                                        GLM_MAT4_IDENTITY };
            new_rend_obj.scene_serialize(mode, node_ref["render_obj"]);
            m_rend_obj_key =
                m_renderer.get_render_object_pool().emplace(std::move(new_rend_obj));
        }
    }
}


BT::Game_object_pool::Game_object_pool(
    function<unique_ptr<Game_object>()>&& create_new_empty_game_obj_callback_fn)
    : m_create_new_empty_game_obj_callback_fn{ std::move(create_new_empty_game_obj_callback_fn) }
{
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

    for (auto root_node : root_nodes_scene_hierarchy)
    {
        render_imgui_scene_hierarchy_node_recursive(root_node, next_id);
    }
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

        // @TODO: Add more like transform and stuff.
    }
    else
    {
        ImGui::Text("Select a game object to inspect its properties.");
    }
    ImGui::End();

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

    m_game_objects.emplace(uuid, std::move(game_object));

    return uuid;
}

void BT::Game_object_pool::render_imgui_scene_hierarchy_node_recursive(void* node_void_ptr, intptr_t& next_id)
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
    if (child_nodes.empty())
        node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

    // Draw my node.
    ImGui::TreeNodeEx(reinterpret_cast<void*>(next_id++),
                        node_flags,
                        "%s",
                        node->game_obj->get_name().c_str());
    if (ImGui::IsItemClicked())
        m_selected_game_obj = cur_uuid;
    if (ImGui::BeginDragDropSource())
    {
        ImGui::SetDragDropPayload("_TREENODE", NULL, 0);
        ImGui::Text("This is a drag and drop source");
        ImGui::EndDragDropSource();
    }

    // Draw children nodes.
    for (auto child_node : child_nodes)
    {
        render_imgui_scene_hierarchy_node_recursive(child_node, next_id);
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
