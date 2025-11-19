#include "process_physics_object_lifetime.h"

#include "btlogger.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "renderer/mesh.h"
#include "service_finder/service_finder.h"
#include "uuid/uuid.h"

#include <memory>

// @NOTE: See `process_render_object_lifetime.cpp` for a similar system.


namespace
{

using namespace BT;

/// How to destroy physics objects.
enum Destroy_behavior
{
    DESTROY_ALL,
    DESTROY_ONLY_DANGLING,
};

/// Searches thru physics objects and finds and deletes certain ones.
void destroy_physics_objects(entt::registry& reg,
                             Physics_engine& phys_engine,
                             Destroy_behavior destroy_behavior)
{   // Get all UUIDs inside physics object pool.
    auto all_phys_objs{ phys_engine.checkout_all_physics_objects() };

    struct Found_metadata
    {
        bool found_tag{ false };
        entt::entity ecs_entity{ entt::null };
    };
    std::unordered_map<UUID, Found_metadata> phys_obj_uuid_to_found_metadata_map;
    phys_obj_uuid_to_found_metadata_map.reserve(all_phys_objs.size());

    // Populate data from physics engine.
    for (auto phys_obj : all_phys_objs)
    {
        phys_obj_uuid_to_found_metadata_map.emplace(phys_obj->get_uuid(), Found_metadata{});
    }

    phys_engine.return_physics_objects(std::move(all_phys_objs));

    // Populate data from ECS.
    auto view{ reg.view<component::Created_physics_object_reference>() };
    for (auto entity : view)
    {   // Mark UUID as non-dangling.
        auto& created_phys_obj_ref{ view.get<component::Created_physics_object_reference>(
            entity) };

        auto& found_metadata{ phys_obj_uuid_to_found_metadata_map.at(
            created_phys_obj_ref.physics_obj_uuid_ref) };
        found_metadata.found_tag = true;
        found_metadata.ecs_entity = entity;
    }

    // Remove certain UUIDs.
    for (auto& it : phys_obj_uuid_to_found_metadata_map)
    {
        bool destroy_this;
        switch (destroy_behavior)
        {
        case DESTROY_ALL:
            destroy_this = true;
            break;

        case DESTROY_ONLY_DANGLING:
            // `!it.second == true` means this UUID is dangling.
            destroy_this = !it.second.found_tag;
            break;
        }

        if (destroy_this)
        {   // Remove this UUID.
            phys_engine.remove_physics_object(it.first);

            if (it.second.ecs_entity != entt::null)
                reg.remove<component::Created_physics_object_reference>(it.second.ecs_entity);

            BT_TRACEF("Destroyed and removed \"%s\" from physics object pool.",
                      UUID_helper::to_pretty_repr(it.first).c_str());
        }
    }
}

/// Goes thru all non-created physics objects and creates physics objects.
void create_staged_physics_objects(entt::registry& reg, Physics_engine& phys_engine)
{
    auto view{ reg.view<component::Physics_object_settings>(
        entt::exclude<component::Created_physics_object_reference>) };

    // Create physics objects.
    for (auto entity : view)
    {
        auto const& phys_obj_settings{ view.get<component::Physics_object_settings const>(entity) };
        auto const& transform{ reg.get<component::Transform const>(entity) };

        // Create physics object.
        std::unique_ptr<Physics_object> new_phys_obj{ nullptr };
        switch (phys_obj_settings.phys_obj_type)
        {
        case PHYSICS_OBJECT_TYPE_TRIANGLE_MESH:
        {
            auto const& tm_settings{
                reg.get<component::Physics_obj_type_triangle_mesh_settings const>(entity)
            };
            new_phys_obj = Physics_object::create_triangle_mesh(
                false,
                Model_bank::get_model(tm_settings.model_name),
                JPH::EMotionType{ tm_settings.motion_type },
                Physics_transform::make_phys_trans(transform.position, transform.rotation));
            break;
        }

        case PHYSICS_OBJECT_TYPE_CHARACTER_CONTROLLER:
        {
            auto const& cc_settings{ reg.get<component::Physics_obj_type_char_con_settings const>(
                entity) };
            new_phys_obj = Physics_object::create_character_controller(
                false,
                cc_settings.radius,
                cc_settings.height,
                cc_settings.crouch_height,
                Physics_transform::make_phys_trans(transform.position, transform.rotation));
            break;
        }

        default:
            // Unimplemented type!!! (or invalid type)
            assert(false);
            break;
        }

        UUID phys_obj_uuid{ phys_engine.emplace_physics_object(std::move(new_phys_obj)) };

        // Attach physics object id as new component.
        reg.emplace<component::Created_physics_object_reference>(entity, phys_obj_uuid);

        BT_TRACEF("Created and emplaced \"%s\" into physics object pool.",
                  UUID_helper::to_pretty_repr(phys_obj_uuid).c_str());
    }
}

}  // namespace


void BT::system::process_physics_object_lifetime()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto& phys_engine{ service_finder::find_service<Physics_engine>() };

    if (service_finder::find_service<world::World_properties_container>()
            .get_data_handle()
            .is_simulation_running)
    {
        destroy_physics_objects(reg, phys_engine, DESTROY_ONLY_DANGLING);
        create_staged_physics_objects(reg, phys_engine);
    }
    else
    {
        destroy_physics_objects(reg, phys_engine, DESTROY_ALL);
    }
}
