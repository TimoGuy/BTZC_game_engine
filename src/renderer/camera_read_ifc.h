#pragma once

#include "cglm/cglm.h"


namespace BT
{

class Camera_read_ifc
{
public:
    virtual void fetch_camera_matrices(mat4& out_projection,
                                       mat4& out_view,
                                       mat4& out_projection_view) = 0;
};

}  // namespace BT
