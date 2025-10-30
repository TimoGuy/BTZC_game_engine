#include "transform.h"

#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"


void BT::component::submit_transform_change_helper(entt::registry& reg,
                                                   entt::entity entity,
                                                   rvec3s pos,
                                                   versors rot,
                                                   vec3s sca)
{   // Write new transform as a change request.
    auto& trans_changed{ reg.get_or_emplace<component::Transform_changed>(entity) };
    trans_changed.next_transform.position = pos;
    trans_changed.next_transform.rotation = rot;
    trans_changed.next_transform.scale    = sca;
}
