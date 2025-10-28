#include "process_render_object_lifetime.h"

#include "btlogger.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/entity_container.h"
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

/// Searches thru render objects and finds and deletes 
void destroy_dangling_render_objects(entt::registry& reg, Render_object_pool& rend_obj_pool)
{   // Get all UUIDs inside render object pool.
    auto all_rend_objs{ rend_obj_pool.checkout_all_render_objs() };

    std::unordered_map<UUID, bool> rend_obj_uuid_to_found_tag_map;
    rend_obj_uuid_to_found_tag_map.reserve(all_rend_objs.size());

    for (auto rend_obj : all_rend_objs)
    {
        rend_obj_uuid_to_found_tag_map.emplace(rend_obj->get_uuid(), false);
    }

    rend_obj_pool.return_render_objs(std::move(all_rend_objs));

    // Mark UUIDs as non-dangling from render object pool collection.
    auto view{ reg.view<component::Created_render_object_reference>() };
    for (auto entity : view)
    {   // Mark UUID as non-dangling.
        auto& created_rend_obj_ref{ view.get<component::Created_render_object_reference>(entity) };
        rend_obj_uuid_to_found_tag_map.at(created_rend_obj_ref.render_obj_uuid_ref) = true;
    }

    // Remove dangling UUIDs.
    for (auto& it : rend_obj_uuid_to_found_tag_map)
        if (!it.second)
        {   // Remove this UUID since it's dangling.
            rend_obj_pool.remove(it.first);
            BT_TRACEF("Destroyed and removed \"%s\" from render object pool.",
                      UUID_helper::to_pretty_repr(it.first).c_str());
        }
}

/// Goes thru all non-created render objects and creates render objects.
void create_staged_render_objects(entt::registry& reg, Render_object_pool& rend_obj_pool)
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
        if (rend_obj_settings.is_deformed)
        {   // Create deformed model w/ animator.
            auto deformed_model{ std::make_unique<Deformed_model>(model) };
            auto model_animator{ std::make_unique<Model_animator>(model) };
            service_finder::find_service<Animator_template_bank>()
                .load_animator_template_into_animator(*model_animator,
                                                      rend_obj_settings.animator_template_name);

            new_rend_obj.set_deformed_model(std::move(deformed_model));
            new_rend_obj.set_model_animator(std::move(model_animator));
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


void BT::system::process_render_object_lifetime()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };

    // @TODO: Perhaps right @HERE there needs to be a lock on the renderer, since it's basically an
    //        update to the renderer of "hey here's everything that updated". 

    destroy_dangling_render_objects(reg, rend_obj_pool);
    create_staged_render_objects(reg, rend_obj_pool);
}
