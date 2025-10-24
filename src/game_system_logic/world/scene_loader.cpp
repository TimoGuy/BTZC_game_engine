#include "scene_loader.h"

#include "btlogger.h"
#include "entt/entity/registry.hpp"
#include "service_finder/service_finder.h"


BT::world::Scene_loader::Scene_loader()
{
    BT_SERVICE_FINDER_ADD_SERVICE(Scene_loader, this);
}

void BT::world::Scene_loader::load_scene(std::string const& scene_name)
{
    m_load_scene_requests.emplace_back(scene_name);
}

void BT::world::Scene_loader::unload_all_scenes()
{
    m_unload_all_scenes_request = true;

    // Clear load requests since these haven't been processed yet.
    // (and would have been unloaded immediately if they did get processed)
    m_load_scene_requests.clear();
}


namespace
{

using namespace BT;
using Scene_entity_list_t = world::Scene_loader::Scene_entity_list_t;

void internal_unload_scene(entt::registry& reg,
                           std::string const& scene_name,
                           Scene_entity_list_t const& scene_entity_list)
{   // Destroy all entities associated with scene.
    for (auto entity : scene_entity_list)
    {
        reg.destroy(entity);
    }

    BT_TRACEF("Unloaded scene \"%s\"", scene_name.c_str());
}

Scene_entity_list_t internal_load_scene(entt::registry& reg, std::string const& scene_name)
{   // Deserialize scene into creation list.

    // Create entities with components.
    Scene_entity_list_t created_entities;

    // Remap entity id to `entt::entity`
    assert(false);

    BT_TRACEF("Loaded scene \"%s\"", scene_name.c_str());

    return created_entities;
}

}  // namespace


void BT::world::Scene_loader::process_scene_loading_requests(entt::registry& reg)
{   // Process unload request first.
    if (m_unload_all_scenes_request)
    {
        for (auto& loaded_scene : m_loaded_scenes)
        {
            internal_unload_scene(reg, loaded_scene.first, loaded_scene.second);
        }
        m_loaded_scenes.clear();

        m_unload_all_scenes_request = false;
    }

    // Process load requests.
    for (auto& req : m_load_scene_requests)
    {
        auto created_entity_list{ internal_load_scene(reg, req) };
    }
    m_load_scene_requests.clear();
}
