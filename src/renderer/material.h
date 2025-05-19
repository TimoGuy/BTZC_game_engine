#pragma once

#include "cglm/cglm.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

using std::pair;
using std::string;
using std::unique_ptr;
using std::vector;


namespace BT
{

class Material_ifc
{
public:
    // For unique_ptr.
    virtual ~Material_ifc();

    virtual void bind_material(mat4 transform) = 0;
    virtual void unbind_material() = 0;
};

// @COPYPASTA. See "mesh.h"
class Material_bank
{
public:
    static void emplace_material(string const& name, unique_ptr<Material_ifc>&& material);
    static Material_ifc* get_material(string const& name);

private:
    inline static vector<pair<string, unique_ptr<Material_ifc>>> s_materials;
};

}  // namespace BT
