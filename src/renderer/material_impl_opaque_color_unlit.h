#pragma once

#include "btglm.h"
#include "material.h"


namespace BT
{

static constexpr uint8_t k_depth_test_mode_any{ 0 };
static constexpr uint8_t k_depth_test_mode_front{ 1 };
static constexpr uint8_t k_depth_test_mode_back{ 2 };

class Material_opaque_color_unlit : public Material_ifc
{
public:
    Material_opaque_color_unlit(vec3 color, uint8_t depth_test_mode);

    virtual void bind_material(mat4 transform) override;
    virtual void unbind_material() override;

private:
    vec3 m_color;
    uint8_t m_depth_test_mode;
};

}  // namespace BT
