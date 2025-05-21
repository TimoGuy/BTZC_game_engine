#pragma once

#include "cglm/cglm.h"
#include "material.h"


namespace BT
{

class Material_impl_post_process : public Material_ifc
{
public:
    Material_impl_post_process(float_t exposure);

    virtual void bind_material(mat4 transform) override;
    virtual void unbind_material() override;

private:
    float_t m_exposure;
};

}  // namespace BT

