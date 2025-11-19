#pragma once

#include "btglm.h"
#include "btjson.h"
#include "entt/entity/entity.hpp"
#include "entt/entity/fwd.hpp"
#include "uuid/uuid.h"


namespace BT
{
namespace component
{

/// Global transform of an entity.
struct Transform
{
    rvec3s  position{ 0, 0, 0 };
    versors rotation{ 0, 0, 0, 1 };
    vec3s   scale{ 1, 1, 1 };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Transform,
        position,
        rotation,
        scale
    );
};

/// References to other entities connected to this transform within the transform hierarchy.
struct Transform_hierarchy
{
    UUID parent_entity;
    std::vector<UUID> children_entities;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Transform_hierarchy,
        parent_entity,
        children_entities
    );
};

/// Tag that transform was changed (this is used for transform propagation thru the hierarchy, also
/// to avoid directly mutating `Transform` component).
struct Transform_changed
{
    Transform next_transform;
};

/// Helper function for submitting new transform change.
void submit_transform_change_helper(entt::registry& reg,
                                    entt::entity entity,
                                    rvec3s pos,
                                    versors rot,
                                    vec3s sca);

/// Helper function for submitting new transform change (does not change scale).
void submit_transform_change_no_scale_helper(entt::registry& reg,
                                             entt::entity entity,
                                             rvec3s pos,
                                             versors rot);

/// Helper function for submitting new transform change (does not change position nor scale).
void submit_transform_change_only_rotation_helper(entt::registry& reg,
                                                  entt::entity entity,
                                                  versors rot);

}  // namespace component
}  // namespace BT
