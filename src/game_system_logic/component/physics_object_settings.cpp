#include "physics_object_settings.h"

#include "entt/entity/registry.hpp"
#include "physics_engine/physics_engine.h"
#include "physics_engine/physics_object.h"
#include "service_finder/service_finder.h"


void BT::component::try_set_physics_object_transform_helper(entt::registry& reg,
                                                            entt::entity entity,
                                                            rvec3s pos,
                                                            versors rot)
{   // Make sure that there is a physics object component.
    auto poss_phys_obj_ref{ reg.try_get<component::Created_physics_object_reference const>(
        entity) };
    if (!poss_phys_obj_ref)
        return;

    // Checkout physics object.
    auto& phys_engine{ service_finder::find_service<Physics_engine>() };
    auto& phys_obj{ *phys_engine.checkout_physics_object(poss_phys_obj_ref->physics_obj_uuid_ref) };

    // Apply transform.
    phys_obj.get_impl()->move_kinematic(Physics_transform::make_phys_trans(pos, rot));

    phys_engine.return_physics_object(&phys_obj);
}
