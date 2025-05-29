#pragma once

#include "cglm/cglm.h"
#include "material.h"


namespace BT
{

class Material_opaque_texture_shaded : public Material_ifc
{
public:
    Material_opaque_texture_shaded(uint32_t color_image, vec3 tint);

    virtual void bind_material(mat4 transform) override;
    virtual void unbind_material() override;

private:
    uint32_t m_color_image;
    vec3 m_tint;
};

}  // namespace BT

