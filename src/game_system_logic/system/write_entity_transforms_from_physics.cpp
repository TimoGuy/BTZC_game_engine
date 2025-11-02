#include "write_entity_transforms_from_physics.h"

#include "btlogger.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/physics_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "physics_engine/physics_engine.h"
#include "service_finder/service_finder.h"


void BT::system::write_entity_transforms_from_physics()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto& phys_engine{ service_finder::find_service<Physics_engine>() };

    // Submit transform changes.
    auto view{
        reg.view<component::Transform const, component::Created_physics_object_reference const>()
    };
    for (auto entity : view)
    {
        auto const& transform{ view.get<component::Transform const>(entity) };
        auto const& phys_obj_ref{
            view.get<component::Created_physics_object_reference const>(entity) };

        // Read transform from physics object.
        auto& phys_obj{ *phys_engine.checkout_physics_object(phys_obj_ref.physics_obj_uuid_ref) };

        rvec3s new_pos;
        versors new_rot;
        vec3s new_sca;
        phys_obj.get_transform_for_entity(new_pos.raw, new_rot.raw);

        phys_engine.return_physics_object(&phys_obj);

        // Scale is not written by physics obj.
        new_sca = transform.scale;

        // Submit new transform.
        component::submit_transform_change_helper(reg, entity, new_pos, new_rot, new_sca);
    }
}
