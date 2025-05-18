#pragma once

#include "cglm/cglm.h"
#include "material.h"
#include <string>
#include <vector>

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

    void render_mesh(mat4 transform);

private:
    // Mesh data.
    vector<uint32_t> m_indices;
    Material_ifc*    m_material;
    AA_bounding_box  m_mesh_aabb;  // @UNUSED: Unknown whether to get this used or not.

    uint32_t m_mesh_index_ebo;  // @TODO
};

class Model
{
public:
    Model(string const& fname);

    void render_model(mat4 transform);

private:
    // @NOTE: These meshes should have some kind of offset inside them, but just
    //   apply all the transforms of the meshes inside of the model during loading.
    //   So that the meshes are just identity from the model.
    vector<Mesh>    m_meshes;
    vector<Vertex>  m_vertices;
    AA_bounding_box m_model_aabb;

    uint32_t m_model_vertex_vao;  // @TODO

    void load_obj_as_meshes(string const& fname);
};

}  // namespace BT
