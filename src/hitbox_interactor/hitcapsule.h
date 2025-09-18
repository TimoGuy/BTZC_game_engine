#pragma once

#include "cglm/cglm.h"


namespace BT
{

class Hitcapsule
{
public:
    enum Type
    {
        HITBOX_TYPE_RECEIVE_HURT,
        HITBOX_TYPE_GIVE_HURT,
    };

private:
    Type m_type;

    // @NOTE: Height is the distance between the two capsule end origins.
    //        As in, height excludes end cap radii.
    vec3 m_origin;
    vec3 m_up;  // Must be unit length.
    float_t m_radius;
    float_t m_half_height;
};

}  // namespace BT
