#pragma once

#include "cglm/cglm.h"
#include "material.h"


namespace BT
{

class Material_debug_picking : public Material_ifc
{
public:
    Material_debug_picking();

    void set_color(vec3 color);

    virtual void bind_material(mat4 transform) override;
    virtual void unbind_material() override;

private:
    vec3 m_color;
};

}  // namespace BT
