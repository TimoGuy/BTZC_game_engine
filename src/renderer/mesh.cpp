#include "mesh.h"

#include "cglm/vec2-ext.h"
#include "cglm/vec3-ext.h"
#include "cglm/vec3.h"
#include "glad/glad.h"
#include "logger.h"
#include "material.h"
#include "tiny_obj_loader.h"
#include <cassert>
#include <cmath>
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

    // Create mesh in OpenGL.
    glGenBuffers(1, &m_mesh_index_ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mesh_index_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_indices.size() * sizeof(uint32_t), m_indices.data(), GL_STATIC_DRAW);
}

BT::Mesh::~Mesh()
{
    // Delete the opengl mesh.
    glDeleteBuffers(1, &m_mesh_index_ebo);
}

void BT::Mesh::render_mesh(mat4 transform, Material_ifc* override_material /*= nullptr*/) const
{
    // @NOTE: All meshes share vertices, so they are stored and bound at the model level.
    // @TODO: @CHECK: Perhaps this method will mess with driver stuff. We'll see.
    // @TODO: @CHECK: I think that the element array buffer should be included here. idk.
    if (override_material == nullptr)
    {
        override_material = m_material;
    }

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_mesh_index_ebo);
    override_material->bind_material(transform);
    glDrawElements(GL_TRIANGLES, m_indices.size(), GL_UNSIGNED_INT, reinterpret_cast<void*>(0));
    override_material->unbind_material();
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

vector<uint32_t> const& BT::Mesh::get_indices() const
{
    return m_indices;
}


BT::Model::Model(string const& fname, string const& material_name)
{
    load_obj_as_meshes(fname, material_name);
}

BT::Model::~Model()
{
    glDeleteBuffers(1, &m_model_vertex_vbo);
    glDeleteVertexArrays(1, &m_model_vertex_vao);
}

void BT::Model::render_model(mat4 transform, Material_ifc* override_material /*= nullptr*/) const
{
    // @NOTE: All meshes share the same vertices, just use different indices.
    glBindVertexArray(m_model_vertex_vao);

    for (auto& mesh : m_meshes)
    {
        mesh.render_mesh(transform, override_material);
    }

    glBindVertexArray(0);
}

pair<vector<BT::Vertex> const&, vector<uint32_t>> BT::Model::get_all_vertices_and_indices() const
{
    vector<uint32_t> all_indices;
    for (auto& mesh : m_meshes)
    {
        auto const& indices{ mesh.get_indices() };
        size_t start_idx{ all_indices.size() };
        all_indices.resize(start_idx + indices.size());

        for (size_t i = 0; i < indices.size(); i++)
        {
            all_indices[start_idx + i] = indices[i];
        }
    }
    all_indices.shrink_to_fit();
    return { m_vertices, all_indices };
}

void BT::Model::load_obj_as_meshes(string const& fname, string const& material_name)
{
    if (!std::filesystem::exists(fname) ||
        !std::filesystem::is_regular_file(fname))
    {
        // Exit early if this isn't a good fname.
        logger::printef(logger::ERROR, "\"%s\" does not exist or is not a file.", fname.c_str());
        assert(false);
        return;
    }

    auto fname_path{ std::filesystem::path(fname) };
    if (!fname_path.has_extension() ||
        fname_path.extension().string() != ".obj")
    {
        // Exit early if this isn't an obj file.
        logger::printef(logger::ERROR, "\"%s\" is not a .obj file.", fname.c_str());
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
        logger::printef(logger::ERROR, "OBJ file \"%s\" failed to load.", fname.c_str());
        assert(false);
        return;
    }

    if (!warn.empty())
    {
        logger::printe(logger::WARN, warn);
        assert(false);
    }

    if (!err.empty())
    {
        logger::printe(logger::ERROR, err);
        assert(false);
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
            glm_vec3_copy(vec3{ attrib.vertices[3 * index.vertex_index + 0],
                                attrib.vertices[3 * index.vertex_index + 1],
                                attrib.vertices[3 * index.vertex_index + 2] }, new_gpu_vertex.position);
            glm_vec3_copy(vec3{ attrib.normals[3 * index.normal_index + 0],
                                attrib.normals[3 * index.normal_index + 1],
                                attrib.normals[3 * index.normal_index + 2] }, new_gpu_vertex.normal);
            glm_vec2_copy(vec2{ attrib.texcoords[2 * index.texcoord_index + 0],
                                attrib.texcoords[2 * index.texcoord_index + 1] }, new_gpu_vertex.tex_coord);

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

    // Find whole AABB.
    m_model_aabb.reset();
    for (auto& vertex : m_vertices)
    {
        m_model_aabb.feed_position(vertex.position);
    }

    // Upload vertices to GPU.
    glGenVertexArrays(1, &m_model_vertex_vao);
    glGenBuffers(1, &m_model_vertex_vbo);

    glBindVertexArray(m_model_vertex_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_model_vertex_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);

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

    // Register vertex attributes.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tex_coord)));

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}


// @TODO: Deformed_model vvv
// Takes `Model` const ref and uses its vertex buffer and skin datas buffer as input for deforming compute shader.
// Then outputs result into the `m_deform_vertex_vbo`.
// A memory barrier waits for all of these vbo's to be written.
// When connected to a render object, the render object will take a `unique_ptr` of the deformed model, and if it exists,
// it will get the `m_deform_vertex_vao` and call `render_model()` with the `override_vao` param set.
BT::Deformed_model::Deformed_model(Model const& model)
    : m_model{ model }
{
    // Create empty buffer for resulting deformed vertices.
    glGenVertexArrays(1, &m_deform_vertex_vao);
    glGenBuffers(1, &m_deform_vertex_vbo);

    glBindVertexArray(m_deform_vertex_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_deform_vertex_vbo);
    glBufferData(GL_ARRAY_BUFFER, m_model.m_vertices.size() * sizeof(Vertex), nullptr, GL_STATIC_DRAW);

    // Register vertex attributes.
    // @COPYPASTA.
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, position)));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, normal)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE,
                          sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, tex_coord)));

    // Unbind.
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

BT::Deformed_model::~Deformed_model()
{
    glDeleteBuffers(1, &m_deform_vertex_vbo);
    glDeleteVertexArrays(1, &m_deform_vertex_vao);
}

void BT::Deformed_model::dispatch_compute_deform(vector<mat4s>&& joint_matrices)
{
    // @TODO: Figure out how to do compute shaders!!!
    assert(false);
}


void BT::Model_bank::emplace_model(string const& name, unique_ptr<Model>&& model)
{
    if (get_model(name) != nullptr)
    {
        // Report model already exists.
        logger::printef(logger::ERROR, "Model \"%s\" already exists.", name.c_str());
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
            model_ptr = model.second.get();
            break;
        }

    return model_ptr;
}

string BT::Model_bank::get_model_name(Model const* model_ptr)
{
    string model_name{ "" };

    for (auto& model : s_models)
        if (model.second.get() == model_ptr)
        {
            model_name = model.first;
            break;
        }

    return model_name;
}
