#include "process_render_object_lifetime.h"

#include "btlogger.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/anim_frame_action_controller.h"
#include "game_system_logic/component/animator_driven_hitcapsule_set.h"
#include "game_system_logic/component/animator_root_motion.h"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/entity_container.h"
#include "game_system_logic/world/world_properties.h"
#include "renderer/animator_template.h"
#include "renderer/mesh.h"
#include "renderer/model_animator.h"
#include "renderer/render_object.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"
#include "uuid/uuid.h"

#include <memory>


namespace
{

using namespace BT;

/// How to destroy render objects.
enum Destroy_behavior
{
    DESTROY_DANGLING_OR_DEFORMABLE,
    DESTROY_DANGLING_OR_SUPPOSED_TO_BE_DEFORMABLE,
};

/// Searches thru render objects and finds and deletes certain ones.
void destroy_render_objects(entt::registry& reg,
                            Render_object_pool& rend_obj_pool,
                            Destroy_behavior destroy_behavior)
{   // Get all UUIDs inside render object pool.
    auto all_rend_objs{ rend_obj_pool.checkout_all_render_objs() };

    struct Found_metadata
    {
        bool found_tag{ false };
        entt::entity ecs_entity{ entt::null };
        bool is_deformable_in_settings{ false };
        bool is_deformed_in_created{ false };
    };
    std::unordered_map<UUID, Found_metadata> rend_obj_uuid_to_metadata_map;
    rend_obj_uuid_to_metadata_map.reserve(all_rend_objs.size());

    // Populate data from renderer.
    for (auto rend_obj : all_rend_objs)
    {
        Found_metadata metadata{
            .is_deformed_in_created = (rend_obj->get_deformed_model() != nullptr)
        };
        rend_obj_uuid_to_metadata_map.emplace(rend_obj->get_uuid(), std::move(metadata));
    }

    rend_obj_pool.return_render_objs(std::move(all_rend_objs));

    // Populate data from ECS.
    auto view{ reg.view<component::Render_object_settings const,
                        component::Created_render_object_reference const>() };
    for (auto entity : view)
    {   // Mark UUID as non-dangling.
        auto const& rend_obj_settings{ view.get<component::Render_object_settings const>(entity) };
        auto const& created_rend_obj_ref{
            view.get<component::Created_render_object_reference const>(entity)
        };

        auto& found_metadata{ rend_obj_uuid_to_metadata_map.at(
            created_rend_obj_ref.render_obj_uuid_ref) };
        found_metadata.found_tag                 = true;
        found_metadata.ecs_entity                = entity;
        found_metadata.is_deformable_in_settings = rend_obj_settings.is_deformed;
    }

    // Remove certain UUIDs.
    for (auto& it : rend_obj_uuid_to_metadata_map)
    {   // `!it.second == true` means this UUID is dangling.
        bool destroy_this{ !it.second.found_tag };
        switch (destroy_behavior)
        {
        case DESTROY_DANGLING_OR_DEFORMABLE:
            destroy_this |= it.second.is_deformed_in_created;
            break;

        case DESTROY_DANGLING_OR_SUPPOSED_TO_BE_DEFORMABLE:
            destroy_this |=
                (!it.second.is_deformed_in_created && it.second.is_deformable_in_settings);
            break;
        }

        if (destroy_this)
        {   // Remove this UUID.
            rend_obj_pool.remove(it.first);

            if (it.second.ecs_entity != entt::null)
                reg.remove<component::Created_render_object_reference>(it.second.ecs_entity);

            BT_TRACEF("Destroyed and removed \"%s\" from render object pool.",
                      UUID_helper::to_pretty_repr(it.first).c_str());
        }
    }
}

/// Goes thru all non-created render objects and creates render objects.
void create_staged_render_objects(Entity_container& entity_container,
                                  entt::registry& reg,
                                  Render_object_pool& rend_obj_pool,
                                  bool allow_deformed_creation)
{
    auto view{ reg.view<component::Render_object_settings>(
        entt::exclude<component::Created_render_object_reference>) };

    // Create render objects.
    for (auto entity : view)
    {
        auto& rend_obj_settings{ view.get<component::Render_object_settings>(entity) };

        // Create render object.
        Render_object new_rend_obj{ rend_obj_settings.render_layer };

        auto const& model{ *Model_bank::get_model(rend_obj_settings.model_name) };
        if (allow_deformed_creation && rend_obj_settings.is_deformed)
        {   // Create deformed model w/ animator.
            auto deformed_model{ std::make_unique<Deformed_model>(model) };

            bool has_root_motion_tag{ reg.any_of<component::Animator_root_motion>(entity) };
            auto model_animator{ std::make_unique<Model_animator>(model, has_root_motion_tag) };

            service_finder::find_service<Animator_template_bank>()
                .load_animator_template_into_animator(*model_animator,
                                                      rend_obj_settings.animator_template_name);

            new_rend_obj.set_deformed_model(std::move(deformed_model));
            new_rend_obj.set_model_animator(std::move(model_animator));

            // Check for anim frame action controller configuration.
            auto afa_ctrller{ reg.try_get<component::Anim_frame_action_controller>(entity) };
            if (afa_ctrller != nullptr)
            {   // Configure anim frame action data.
                new_rend_obj.get_model_animator()->configure_anim_frame_action_controls(
                    &anim_frame_action::Bank::get(afa_ctrller->anim_frame_action_controller_name),
                    entity_container.find_entity_uuid(entity));

                // Add hitcapsule set driver.
                reg.emplace_or_replace<component::Animator_driven_hitcapsule_set>(entity);
            }
        }
        else
        {   // Use static model from bank.
            new_rend_obj.set_model(Model_bank::get_model(rend_obj_settings.model_name));
        }

        UUID rend_obj_uuid{ rend_obj_pool.emplace(std::move(new_rend_obj)) };

        // Attach render object id as new component.
        reg.emplace<component::Created_render_object_reference>(entity, rend_obj_uuid);

        BT_TRACEF("Created and emplaced \"%s\" into render object pool.",
                  UUID_helper::to_pretty_repr(rend_obj_uuid).c_str());
    }
}

}  // namespace


void BT::system::process_render_object_lifetime(bool force_allow_deformed_render_objs)
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };
    auto& reg{ entity_container.get_ecs_registry() };
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };

    // @TODO: Perhaps right @HERE there needs to be a lock on the renderer, since it's basically an
    //        update to the renderer of "hey here's everything that updated".

    if (force_allow_deformed_render_objs ||
        service_finder::find_service<world::World_properties_container>()
            .get_data_handle()
            .is_simulation_running)
    {
        destroy_render_objects(reg, rend_obj_pool, DESTROY_DANGLING_OR_SUPPOSED_TO_BE_DEFORMABLE);
        create_staged_render_objects(entity_container, reg, rend_obj_pool, true);
    }
    else
    {
        destroy_render_objects(reg, rend_obj_pool, DESTROY_DANGLING_OR_DEFORMABLE);
        create_staged_render_objects(entity_container, reg, rend_obj_pool, false);
    }
}
