#pragma once

#include "btglm.h"
#include "btjson.h"

#include <entt/entity/fwd.hpp>


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
    entt::entity parent_entity;
    std::vector<entt::entity> children_entities;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(
        Transform_hierarchy,
        parent_entity,
        children_entities
    );
};

/// Tag that transform was changed (this is used for transform propagation thru the hierarchy).
struct Transform_changed
{
};

}  // namespace component
}  // namespace BT
