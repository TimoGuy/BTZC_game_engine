#include "mesh.h"

#include "cglm/vec2-ext.h"
#include "cglm/vec3-ext.h"
#include "cglm/vec3.h"
#include "glad/glad.h"
#include "material.h"
#include "tiny_obj_loader.h"
#include <cassert>
#include <cmath>
#include <fmt/base.h>
#include <filesystem>
#include <limits>
#include <string>
#include <unordered_map>
#include <vector>

using std::numeric_limits;
using std::string;
using std::unordered_map;
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

void BT::Mesh::render_mesh(mat4 transform) const
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

BT::Model::Model(string const& fname, string const& material_name)
{
    load_obj_as_meshes(fname, material_name);
}

void BT::Model::render_model(mat4 transform) const
{
    // @NOTE: All meshes share the same vertices, just use different indices.
    glBindVertexArray(m_model_vertex_vao);

    for (auto& mesh : m_meshes)
    {
        mesh.render_mesh(transform);
    }

    glBindVertexArray(0);
}

void BT::Model::load_obj_as_meshes(string const& fname, string const& material_name)
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
    vector<tinyobj::material_t> materials;  // @NOTE: Materials are simply ignored (for obj files).

    string base_dir{ fname_path.parent_path().generic_string() };

    string warn;
    string err;
    if (!tinyobj::LoadObj(&attrib,
                          &shapes,
                          &materials,
                          &warn,
                          &err,
                          fname.c_str(),
                          base_dir.c_str()))
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

    // Get all unique combinations of attributes together.
    assert(attrib.vertices.size() < std::pow(2, 21));
    assert(attrib.normals.size() < std::pow(2, 21));
    assert(attrib.texcoords.size() < std::pow(2, 21));
    auto generate_key_fn = [](tinyobj::index_t const& index) {
        assert(index.vertex_index >= 0);
        assert(index.normal_index >= 0);
        assert(index.texcoord_index >= 0);
        uint64_t key{ (static_cast<uint64_t>(index.vertex_index) << 42) |
                      (static_cast<uint64_t>(index.normal_index) << 21) |
                      (static_cast<uint64_t>(index.texcoord_index) << 0) };
        return key;
    };

    struct Vertex_with_index
    {
        uint32_t index;
        Vertex vertex;
    };

    unordered_map<uint64_t, Vertex_with_index> key_to_vertex_map;
    uint32_t current_index{ 0 };
    for (auto& shape : shapes)
    for (auto& index : shape.mesh.indices)
    {
        // Generate and emplace key if unique.
        uint64_t key = generate_key_fn(index);
        
        if (key_to_vertex_map.find(key) == key_to_vertex_map.end())
        {
            // Add new vertex.
            Vertex new_gpu_vertex;
            glm_vec3_copy(vec3{ attrib.vertices[index.vertex_index + 0],
                                attrib.vertices[index.vertex_index + 1],
                                attrib.vertices[index.vertex_index + 2] }, new_gpu_vertex.position);
            glm_vec3_copy(vec3{ attrib.normals[index.normal_index + 0],
                                attrib.normals[index.normal_index + 1],
                                attrib.normals[index.normal_index + 2] }, new_gpu_vertex.normal);
            glm_vec2_copy(vec2{ attrib.texcoords[index.texcoord_index + 0],
                                attrib.texcoords[index.texcoord_index + 1] }, new_gpu_vertex.tex_coord);

            Vertex_with_index vwi{ current_index++, new_gpu_vertex };
            key_to_vertex_map.emplace(key, vwi);
        }
    }

    // Transform vertices into model structure.
    m_vertices.clear();
    m_vertices.resize(key_to_vertex_map.size());

    for (auto it = key_to_vertex_map.begin(); it != key_to_vertex_map.end(); it++)
    {
        m_vertices[it->second.index] = it->second.vertex;
    }

    // Transform indices into mesh structures.
    m_meshes.clear();
    m_meshes.reserve(shapes.size());

    for (auto& shape : shapes)
    {
        vector<uint32_t> indices;
        indices.reserve(shape.mesh.indices.size());
        for (auto& index : shape.mesh.indices)
        {
            uint64_t key = generate_key_fn(index);
            indices.emplace_back(key_to_vertex_map.at(key).index);
        }

        // Create mesh.
        m_meshes.emplace_back(std::move(indices), material_name);
    }

    // @TODO: Continue this.
    assert(false);
}

void BT::Model_bank::emplace_model(string const& name, Model&& model)
{
    if (get_model(name) != nullptr)
    {
        // Report model already exists.
        fmt::println("ERROR: Model \"%s\" already exists.", name);
        assert(false);
        return;
    }

    s_models.emplace_back(name, std::move(model));
}

BT::Model const* BT::Model_bank::get_model(string const& name)
{
    Model* model_ptr{ nullptr };

    for (auto& model : s_models)
        if (model.first == name)
        {
            model_ptr = &model.second;
            break;
        }

    return model_ptr;
}
