#pragma once

#include "btjson.h"
#include "entt/entity/fwd.hpp"
#include "physics_engine/physics_object.h"
#include "uuid/uuid.h"


namespace BT
{
namespace component
{

/// Serializable set of info that shows what type of physics object to create.
/// @NOTE: Use `Transform` component for creating initial physics transform.
struct Physics_object_settings
{
    Physics_object_type phys_obj_type{ NUM_PHYSICS_OBJECT_TYPES };  // Invalid value as default.

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Physics_object_settings,
        phys_obj_type
    );
};

/// Enum of motion types. This matches up with `JPH::EMotionType` enum.
enum Physics_object_motion_type : uint8_t
{
    PHYS_OBJ_MOTION_TYPE_STATIC,
    PHYS_OBJ_MOTION_TYPE_KINEMATIC,
    PHYS_OBJ_MOTION_TYPE_DYNAMIC,
};

/// Settings for a triangle mesh physics object.
struct Physics_obj_type_triangle_mesh_settings
{
    std::string model_name{ "" };
    Physics_object_motion_type motion_type{ 0 };

    // @NOTE: I think that before, there was `interpolate_movement` since `game_object`s were tied
    // to the renderer. Here, now the thought is for `entity`s to be tied to the simulation only.
    // And then when the renderer is ready to render, it will grab values from the simulation and
    // interpolate it itself, if that `entity` has `interpolate_transform=true`.  -Thea 2025/10/31
    // bool interpolate_movement{ false };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Physics_obj_type_triangle_mesh_settings,
        model_name,
        motion_type
    );
};

/// Settings for a character controller physics object.
struct Physics_obj_type_char_con_settings
{
    float_t radius{ -1 };
    float_t height{ -1 };
    float_t crouch_height{ -1 };

    // @DITTO: See above for lengthy note.
    // bool interpolate_movement{ false };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Physics_obj_type_char_con_settings,
        radius,
        height,
        crouch_height
    );
};

/// Holder of a reference to a created physics obj. The absence of this component w/ the presence of
/// `Physics_object_settings` means that a physics object needs to be created for this entity.
struct Created_physics_object_reference
{
    UUID physics_obj_uuid_ref;
};

/// Helper for setting the transform of a physics object.
/// @NOTE: Does nothing if there is no created physics object.
/// @NOTE: Emits an error if the physics object is static.
void try_set_physics_object_transform_helper(entt::registry& reg,
                                             entt::entity entity,
                                             rvec3s pos,
                                             versors rot);

}  // namespace component
}  // namespace BT
