#pragma once

#include "btjson.h"
#include "uuid/uuid.h"


namespace BT
{
namespace component
{

/// Tag for showing camera what entity to follow. Must have a `Transform` component attached to the
/// same entity, and there must only be one (if multiple camera views then one per camera).
struct Follow_camera_follow_ref
{
    float_t follow_offset_y{ 1.0f };

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Follow_camera_follow_ref,
        follow_offset_y
    );

    struct State
    {
        UUID locked_on_entity;
    } state;
};

}  // namespace component
}  // namespace BT
