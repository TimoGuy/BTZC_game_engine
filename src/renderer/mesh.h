#pragma once

#include "btglm.h"
#include "material.h"
#include <string>
#include "model_animator.h"
#include <unordered_map>
#include <utility>
#include <vector>

using std::pair;
using std::string;
using std::unordered_map;
using std::vector;


namespace BT
{

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

struct Vertex
{
    vec3 position;
    vec3 normal;
    vec2 tex_coord;
};

struct Vertex_skin_data
{
    uint32_t joint_mat_idxs[4];
    vec4     weights;
};

class Renderable_ifc
{
public:
    virtual ~Renderable_ifc() = default;
    virtual std::string get_type_str() const = 0;
    virtual std::string get_model_name() const = 0;
    virtual void render(mat4 transform, Material_ifc* override_material = nullptr) const = 0;
};

class Model : public Renderable_ifc
{
public:
    Model(string const& fname, string const& material_name);
    Model(const Model&)            = delete;
    Model(Model&&)                 = delete;
    Model& operator=(const Model&) = delete;
    Model& operator=(Model&&)      = delete;
    ~Model();

    std::string get_type_str() const override { return "Model"; }
    std::string get_model_name() const override;
    void render(mat4 transform, Material_ifc* override_material = nullptr) const override;

    vector<Model_joint_animation> const& get_joint_animations() const;
    pair<vector<Vertex> const&, vector<uint32_t>> get_all_vertices_and_indices() const;

private:
    // @NOTE: These meshes should have some kind of offset inside them, but just
    //   apply all the transforms of the meshes inside of the model during loading
    //   so that the meshes are on the same transform as model.
    vector<Mesh>    m_meshes;
    vector<Vertex>  m_vertices;  // Kept for physics mesh colliders.
    AA_bounding_box m_model_aabb;

    uint32_t m_model_vertex_vao;
    uint32_t m_model_vertex_vbo;

    vector<Vertex_skin_data> m_vert_skin_datas;  // @TODO: See if this needs to be stored CPU-side.
    Model_skin m_model_skin;
    vector<Model_joint_animation> m_animations;
    
    uint32_t m_model_vertex_skin_datas_buffer{ 0 };  // 0 if no vertex skin data.

    void load_obj_as_meshes(string const& fname, string const& material_name);
    void load_gltf2_as_meshes(string const& fname, string const& material_name);

    friend class Deformed_model;
    friend class Model_animator;
};

class Deformed_model : public Renderable_ifc
{
public:
    Deformed_model(Model const& model);
    ~Deformed_model();

    void dispatch_compute_deform(vector<mat4s>&& joint_matrices);

    std::string get_type_str() const override { return "Deformed_model"; }
    std::string get_model_name() const override;
    void render(mat4 transform, Material_ifc* override_material = nullptr) const override;

private:
    Model const& m_model;

    uint32_t m_deform_vertex_vao;
    uint32_t m_deform_vertex_vbo;

    static constexpr size_t k_max_num_joints{ 128 };  // @NOTE: Must match `skinned_mesh.comp`.
    uint32_t m_mesh_joint_deform_data_ssbo;
};

// @COPYPASTA: See "material.h"
class Model_bank
{
public:
    static void emplace_model(string const& name, unique_ptr<Model>&& model);
    static Model const* get_model(string const& name);
    static string get_model_name(Model const* model_ptr);
    static vector<string> get_all_model_names();

private:
    inline static vector<pair<string, unique_ptr<Model>>> s_models;
};

}  // namespace BT
