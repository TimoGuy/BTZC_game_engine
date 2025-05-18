#include "mesh.h"

// @NOTE: Must be imported in this order
#include "glad/glad.h"
#include "GLFW/glfw3.h"
////////////////////////////////////////

#include "cglm/vec3-ext.h"
#include "cglm/vec3.h"
#include "material.h"
#include "tiny_obj_loader.h"
#include <cassert>
#include <fmt/base.h>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

using std::numeric_limits;
using std::string;
using std::vector;


void BT::AA_bounding_box::reset()
{
    min[0] = min[1] = min[2] = numeric_limits<float_t>::max();
    max[0] = max[1] = max[2] = numeric_limits<float_t>::min();
}

void BT::AA_bounding_box::feed_position(vec3 position)
{
    glm_vec3_minv(min, position, min);
    glm_vec3_maxv(max, position, max);
}

BT::Mesh::Mesh(vector<uint32_t>&& indices, string const& material_name)
    : m_indices(std::move(indices))
{
    m_material = Material_bank::get_material(material_name);
    assert(m_material != nullptr);

    // @UNUSED.
    // // Calculate AABB.
    // m_mesh_aabb.reset();
    // for (auto& vertex : m_vertices)
    //     m_mesh_aabb.feed_position(vertex.position);

    // @TODO: Put opengl here.
    assert(false);
}

BT::Mesh::~Mesh()
{
    // @TODO: Delete the opengl mesh.
    assert(false);
}

void BT::Mesh::render_mesh(mat4 transform)
{
    // @NOTE: All meshes share vertices, so they are stored and bound at the model level.
    // @TODO: @CHECK: Perhaps this method will mess with driver stuff. We'll see.
    // @TODO: @CHECK: I think that the element array buffer should be included here. idk.
    m_material->bind_material(transform);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mesh_index_ebo);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    m_material->unbind_material();
}

BT::Model::Model(string const& fname)
{
    load_obj_as_meshes(fname);
}

void BT::Model::render_model(mat4 transform)
{
    // @NOTE: All meshes share the same vertices, just use different indices.
    glBindVertexArray(m_model_vertex_vao);

    for (auto& mesh : m_meshes)
    {
        mesh.render_mesh(transform);
    }

    glBindVertexArray(0);
}

void BT::Model::load_obj_as_meshes(string const& fname)
{
    if (!std::filesystem::exists(fname) ||
        !std::filesystem::is_regular_file(fname))
    {
        // Exit early if this isn't a good fname.
        fmt::println("ERROR: \"%s\" does not exist or is not a file.", fname);
        assert(false);
        return;
    }

    auto fname_path{ std::filesystem::path(fname) };
    if (!fname_path.has_extension() ||
        fname_path.extension().string() != ".obj")
    {
        // Exit early if this isn't an obj file.
        fmt::println("ERROR: \"%s\" is not a .obj file.", fname);
        assert(false);
        return;
    }

    // Load obj file.
    tinyobj::attrib_t attrib;
    vector<tinyobj::shape_t> shapes;
    vector<tinyobj::material_t> materials;

    string base_dir{ fname_path.parent_path().generic_string() };

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

    // Find whole AABB.
    m_model_aabb.reset();
    for (size_t i = 0; i < attrib.vertices.size(); i += 3)
    {
        m_model_aabb.feed_position(vec3{ attrib.vertices[i + 0],
                                                   attrib.vertices[i + 1],
                                                   attrib.vertices[i + 2] });
    }

    // @TODO: Continue this.
    assert(false);
}
