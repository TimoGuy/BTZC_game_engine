#include "character_movement.h"

#include "btglm.h"

#include <cassert>


void BT::component::Character_mvt_state::set_facing_angle(float_t angle_radians)
{
    grounded_state.facing_angle = airborne_state.input_facing_angle = angle_radians;
}

float_t BT::component::Character_mvt_state::get_facing_angle() const
{
    assert(glm_eq(grounded_state.facing_angle, airborne_state.input_facing_angle));
    return grounded_state.facing_angle;
}
