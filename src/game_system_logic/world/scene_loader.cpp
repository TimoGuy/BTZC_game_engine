#include "scene_loader.h"

#include "btlogger.h"
#include "game_system_logic/component/component_registry.h"
#include "game_system_logic/entity_container.h"
#include "scene_serialization.h"
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

void internal_unload_scene(Entity_container& entity_container,
                           std::string const& scene_name,
                           Scene_entity_list_t const& scene_entity_list)
{   // Destroy all entities associated with scene.
    for (auto entity : scene_entity_list)
    {
        entity_container.destroy_entity(entity);
    }

    BT_TRACEF("Unloaded scene \"%s\"", scene_name.c_str());
}

Scene_entity_list_t internal_load_scene(Entity_container& entity_container,
                                        std::string const& scene_name)
{   // Deserialize scene into creation list.
    auto scene_data{ world::deserialize_scene_data_from_disk(scene_name) };

    // Create entities with components.
    Scene_entity_list_t created_entities;
    for (auto& entity : scene_data.entities)
    {   // Assert that the provided UUID is valid.
        assert(!entity.entity_uuid.is_nil());

        auto ecs_entity = entity_container.create_entity(entity.entity_uuid);

        for (auto& component : entity.components)
        {   // Construct component inside entity.
            component::construct_component(ecs_entity, component.type_name, component.members_j);
        }
    }

    BT_TRACEF("Loaded scene \"%s\"", scene_name.c_str());

    return created_entities;
}

}  // namespace


void BT::world::Scene_loader::process_scene_loading_requests()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };

    // Process unload request first.
    if (m_unload_all_scenes_request)
    {
        for (auto& loaded_scene : m_loaded_scenes)
        {
            internal_unload_scene(entity_container, loaded_scene.first, loaded_scene.second);
        }
        m_loaded_scenes.clear();

        m_unload_all_scenes_request = false;
    }

    // Process load requests.
    for (auto& scene_name : m_load_scene_requests)
    {
        auto created_entity_list{ internal_load_scene(entity_container, scene_name) };
    }
    m_load_scene_requests.clear();
}
