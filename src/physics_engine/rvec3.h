#pragma once

#include <cmath>


namespace BT
{

#if BTZC_GAME_ENGINE_SETTING_REAL_TYPE_USES_DBL_PRECISION
using Real = double_t;
#else
using Real = float_t;
#endif  // BTZC_GAME_ENGINE_SETTING_REAL_TYPE_USES_DBL_PRECISION

using rvec3 = Real[3];

}  // namespace BT
