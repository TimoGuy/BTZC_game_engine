#pragma once

#include "../btzc_game_engine.h"
#include "btjson.h"
#include "cglm/cglm.h"
#include "cglm/types-struct.h"

#include <cmath>


/// Whether to use double or single precision for real type.
//  @NOTE: There's an issue where `using rvec3 = real_t[3];` causes errors. This is due to the type
//         alias being something not quite the same type. It shows up as `AKA double[3]` in clang.
//         Due to this, it cannot be used in initializer lists. So using a typedef actually sets the
//         type to the other type without any soft aliases, thus causing no issue when using in an
//         initializer list.
//           -Thea 2025/10/21
#ifdef BTZC_REAL_TYPE
#error This macro `BTZC_REAL_TYPE` already defined!!!
#endif  // BTZC_REAL_TYPE

#if BTZC_GAME_ENGINE_SETTING_REAL_TYPE_USES_DBL_PRECISION
#define BTZC_REAL_TYPE double_t
#else
#define BTZC_REAL_TYPE float_t
#endif  // BTZC_GAME_ENGINE_SETTING_REAL_TYPE_USES_DBL_PRECISION

typedef BTZC_REAL_TYPE real_t;
typedef BTZC_REAL_TYPE rvec3[3];
#undef BTZC_REAL_TYPE


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
