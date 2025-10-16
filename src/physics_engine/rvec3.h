#pragma once

#include <cmath>
#include "../btzc_game_engine.h"


namespace BT
{

#if BTZC_GAME_ENGINE_SETTING_REAL_TYPE_USES_DBL_PRECISION
using Real = double_t;
#else
using Real = float_t;
#endif  // BTZC_GAME_ENGINE_SETTING_REAL_TYPE_USES_DBL_PRECISION

using rvec3 = Real[3];

union rvec3s
{
    rvec3 raw;
    struct
    {
        Real x;
        Real y;
        Real z;
    };
};

}  // namespace BT
