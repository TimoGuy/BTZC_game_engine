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
};

class Mesh
{
public:
    Mesh(vector<Vertex>&& vertices, vector<uint32_t>&& indices, string const& material_name);
    ~Mesh();

    void render_mesh(mat4 transform);

private:
    // Mesh data.
    vector<Vertex>   m_vertices;
    vector<uint32_t> m_indices;
    Material_ifc*    m_material;
    AA_bounding_box  m_mesh_aabb;
};

class Model
{
public:
    Model(string const& fname);
    ~Model();

    void render_model(mat4 transform);

private:
    // @NOTE: These meshes should have some kind of offset inside them, but just
    //   apply all the transforms of the meshes inside of the model during loading.
    //   So that the meshes are just identity from the model.
    vector<Mesh>    m_meshes;
    AA_bounding_box m_model_aabb;

    void load_obj_as_meshes(string const& fname);
};

}  // namespace BT
