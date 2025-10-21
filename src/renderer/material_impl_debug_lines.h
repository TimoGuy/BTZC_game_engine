#pragma once

#include "btglm.h"
#include "material.h"


namespace BT
{

class Material_debug_lines : public Material_ifc
{
public:
    Material_debug_lines(bool foreground);

    void set_lines_ssbo(uint32_t ssbo);

    virtual void bind_material(mat4 transform) override;
    virtual void unbind_material() override;

private:
    bool m_foreground;
    uint32_t m_ssbo;
};

}  // namespace BT
