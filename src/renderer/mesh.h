#pragma once

#include "cglm/cglm.h"
#include "material.h"
#include <string>
#include <utility>
#include <vector>

using std::pair;
using std::string;
using std::vector;


namespace BT
{

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 tex_coord;
};

struct AA_bounding_box
{
    vec3 min;
    vec3 max;

    void reset();
    void feed_position(vec3 position);
};

class Mesh
{
public:
    Mesh(vector<uint32_t>&& indices, string const& material_name);
    ~Mesh();

    void render_mesh(mat4 transform, Material_ifc* override_material = nullptr) const;

    vector<uint32_t> const& get_indices() const;

private:
    // Mesh data.
    vector<uint32_t> m_indices;
    Material_ifc*    m_material;
    AA_bounding_box  m_mesh_aabb;  // @UNUSED: Unknown whether to get this used or not.

    uint32_t m_mesh_index_ebo;
};

class Model
{
public:
    Model(string const& fname, string const& material_name);
    Model(const Model&)            = delete;
    Model(Model&&)                 = delete;
    Model& operator=(const Model&) = delete;
    Model& operator=(Model&&)      = delete;
    ~Model();

    void render_model(mat4 transform, Material_ifc* override_material = nullptr) const;

    pair<vector<Vertex> const&, vector<uint32_t>> get_all_vertices_and_indices() const;

private:
    // @NOTE: These meshes should have some kind of offset inside them, but just
    //   apply all the transforms of the meshes inside of the model during loading.
    //   So that the meshes are just identity from the model.
    vector<Mesh>    m_meshes;
    vector<Vertex>  m_vertices;
    AA_bounding_box m_model_aabb;

    uint32_t m_model_vertex_vao;
    uint32_t m_model_vertex_vbo;

    void load_obj_as_meshes(string const& fname, string const& material_name);
};

// @COPYPASTA: See "material.h"
class Model_bank
{
public:
    static void emplace_model(string const& name, unique_ptr<Model>&& model);
    static Model const* get_model(string const& name);
    static string get_model_name(Model const* model_ptr);

private:
    inline static vector<pair<string, unique_ptr<Model>>> s_models;
};

}  // namespace BT
