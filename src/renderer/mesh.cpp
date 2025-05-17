#include "mesh.h"

#include "tiny_obj_loader.h"
#include <fmt/base.h>
#include <filesystem>
#include <string>
#include <vector>

using std::string;
using std::vector;




BT::Mesh::Mesh(string const& obj_fname)
{
    // float bmin[3], bmax[3];
    // tinyobj::
    load_obj_as_mesh(obj_fname);
}

void BT::Mesh::load_obj_as_mesh(string const& fname)
{
    if (!std::filesystem::exists(fname) ||
        !std::filesystem::is_regular_file(fname))
    {
        // Exit early if this isn't a good fname.
        fmt::println("ERROR: \"%s\" does not exist or is not a file.", fname);
        assert(false);
        return;
    }

    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;

    string base_dir{ std::filesystem::path(fname).parent_path().generic_string() };

    string warn;
    string err;
    if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, fname.c_str()))
    {
        fmt::println("ERROR: OBJ file \"%s\" failed to load.", fname);
        assert(false);
        return;
    }

    if (!warn.empty())
    {
        fmt::println("WARNING: %s", warn);
        assert(false);
    }

    if (!err.empty())
    {
        fmt::println("ERROR: %s", err);
        assert(false);
    }

    // @TODO: Continue this.
    assert(false);
}
