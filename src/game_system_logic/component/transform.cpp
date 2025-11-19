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

void BT::component::submit_transform_change_no_scale_helper(entt::registry& reg,
                                                            entt::entity entity,
                                                            rvec3s pos,
                                                            versors rot)
{   // Grab scale from somewhere.
    vec3s scale;
    if (reg.any_of<component::Transform_changed>(entity))
    {   // Get scale from transform request.
        scale = reg.get<component::Transform_changed>(entity).next_transform.scale;
    }
    else
    {   // Get scale from current transform.
        auto poss_transform{ reg.try_get<component::Transform>(entity) };
        if (!poss_transform)
        {   // If there is no transform to read scale from then no no!
            assert(false);
            return;
        }

        scale = poss_transform->scale;
    }

    // Pass off to other function.
    submit_transform_change_helper(reg, entity, pos, rot, scale);
}

void BT::component::submit_transform_change_only_rotation_helper(entt::registry& reg,
                                                                 entt::entity entity,
                                                                 versors rot)
{   // Grab position and scale from somewhere.
    rvec3s position;
    vec3s scale;
    if (reg.any_of<component::Transform_changed>(entity))
    {   // Get from transform request.
        auto& nxt_trans{ reg.get<component::Transform_changed>(entity).next_transform };
        position = nxt_trans.position;
        scale = nxt_trans.scale;
    }
    else
    {   // Get from current transform.
        auto poss_transform{ reg.try_get<component::Transform>(entity) };
        if (!poss_transform)
        {   // If there is no transform to read pos/scale from then no no!
            assert(false);
            return;
        }

        position = poss_transform->position;
        scale = poss_transform->scale;
    }

    // Pass off to other function.
    submit_transform_change_helper(reg, entity, position, rot, scale);
}
