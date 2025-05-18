#include "material.h"

#include <cassert>
#include <fmt/base.h>


BT::Material_ifc::~Material_ifc() = default;

void BT::Material_bank::emplace_material(string const& name, unique_ptr<Material_ifc>&& material)
{
    if (get_material(name) != nullptr)
    {
        // Report material already exists.
        fmt::println("ERROR: Material \"%s\" already exists.", name);
        assert(false);
        return;
    }

    s_materials.emplace_back(name, std::move(material));
}

BT::Material_ifc* BT::Material_bank::get_material(string const& name)
{
    Material_ifc* material_ptr{ nullptr };

    for (auto& material : s_materials)
        if (material.first == name)
        {
            material_ptr = material.second.get();
            break;
        }

    return material_ptr;
}
