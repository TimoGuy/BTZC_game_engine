#pragma once

#include "btglm.h"
#include "btjson.h"
#include "uuid/uuid.h"

#include <entt/entity/entity.hpp>
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
    UUID parent_entity;
    std::vector<UUID> children_entities;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Transform_hierarchy,
        parent_entity,
        children_entities
    );
};

/// Tag that transform was changed (this is used for transform propagation thru the hierarchy).
struct Transform_changed
{   /// For calculating delta transform.
    /// @NOTE: Do not overwrite if this component already exists.
    Transform prev_transform;
};

}  // namespace component
}  // namespace BT
