#pragma once

#include "cglm/cglm.h"
#include "material.h"


namespace BT
{

class Material_opaque_texture_shaded : public Material_ifc
{
public:
    Material_opaque_texture_shaded(uint32_t color_image,
                                   vec3 tint_standable,
                                   vec3 tint_non_standable,
                                   float_t standable_angle_deg);

    virtual void bind_material(mat4 transform) override;
    virtual void unbind_material() override;

private:
    uint32_t m_color_image;
    vec3 m_tint_standable;
    vec3 m_tint_non_standable;
    float_t m_sin_standable_angle;
};

}  // namespace BT

