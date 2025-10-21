#pragma once

#include "../btzc_game_engine.h"
#include "btjson.h"
#include "cglm/cglm.h"
#include "cglm/types-struct.h"

#include <cmath>


/// Whether to use double or single precision for real type.
#if BTZC_GAME_ENGINE_SETTING_REAL_TYPE_USES_DBL_PRECISION
using real_t = double_t;
#else
using real_t = float_t;
#endif  // BTZC_GAME_ENGINE_SETTING_REAL_TYPE_USES_DBL_PRECISION

/// Real vec3.
using rvec3 = real_t[3];

union rvec3s
{
    rvec3 raw;
    struct
    {
        real_t x;
        real_t y;
        real_t z;
    };
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(rvec3s, x, y, z);

/// Types from cglm.
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec2s, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec3s, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(vec4s, x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ivec2s, x, y);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ivec3s, x, y, z);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(ivec4s, x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(versors, x, y, z, w);
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(mat4s, m00, m01, m02, m03,
                                          m10, m11, m12, m13,
                                          m20, m21, m22, m23,
                                          m30, m31, m32, m33);
