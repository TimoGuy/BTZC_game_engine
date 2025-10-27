#include "process_render_object_lifetime.h"

#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"


namespace
{

using namespace BT;

/// Searches thru render objects and finds and deletes 
void destroy_dangling_render_objects(entt::registry& reg)
{
    auto view{ reg.view<component::Created_render_object_reference>() };

    assert(false);  // @TODO: IMPLEMENT

    // Search for dangling UUIDs in renderer pool.

    // Remove dangling UUIDs.
}

/// Goes thru all non-created render objects and creates render objects.
void create_staged_render_objects(entt::registry& reg)
{
    auto view{ reg.view<component::Render_object_settings>(
        entt::exclude<component::Created_render_object_reference>) };

    // .
    for (auto entity : view)
    {
        //
    }
}

}  // namespace


void BT::system::process_render_object_lifetime()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };

    // @TODO: Perhaps right @HERE there needs to be a lock on the renderer, since it's basically an
    //        update to the renderer of "hey here's everything that updated". 

    destroy_dangling_render_objects(reg);
    create_staged_render_objects(reg);
}
