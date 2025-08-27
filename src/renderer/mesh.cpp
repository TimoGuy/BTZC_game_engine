#include "mesh.h"

#include "cglm/affine.h"
#include "cglm/mat4.h"
#include "cglm/vec3.h"
#include "fastgltf/core.hpp"
#include "fastgltf/math.hpp"
#include "fastgltf/types.hpp"
#include "fastgltf/tools.hpp"
#include "glad/glad.h"
#include "logger.h"
#include "material.h"
#include "model_animator.h"
#include "shader.h"
#include "tiny_obj_loader.h"
#include <cassert>
#include <cmath>
#include <filesystem>
#include <limits>
#include <list>
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
    max[0] = max[1] = max[2] = numeric_limits<float_t>::lowest();
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
    auto fname_ext{ std::filesystem::path{ fname }.extension().string() };
    if (fname_ext == ".obj")
    {
        load_obj_as_meshes(fname, material_name);
    }
    else if (fname_ext == ".glb" || fname_ext == ".gltf")
    {
        load_gltf2_as_meshes(fname, material_name);
    }
    else
    {
        logger::printef(logger::ERROR,
                        "Model fname extension not recognized: %s",
                        fname_ext.c_str());
        assert(false);
    }
}

BT::Model::~Model()
{
    glDeleteBuffers(1, &m_model_vertex_vbo);
    glDeleteVertexArrays(1, &m_model_vertex_vao);
}

std::string BT::Model::get_model_name() const
{
    return Model_bank::get_model_name(this);
}

void BT::Model::render(mat4 transform, Material_ifc* override_material /*= nullptr*/) const
{
    // @NOTE: All meshes share the same vertices, just use different indices.
    glBindVertexArray(m_model_vertex_vao);

    for (auto& mesh : m_meshes)
    {
        mesh.render_mesh(transform, override_material);
    }

    glBindVertexArray(0);
}

vector<BT::Model_joint_animation> const& BT::Model::get_joint_animations() const
{
    return m_animations;
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
}

void BT::Model::load_gltf2_as_meshes(string const& fname, string const& material_name)
{
    if (!std::filesystem::exists(fname) ||
        !std::filesystem::is_regular_file(fname))
    {
        // Exit early if this isn't a good fname.
        logger::printef(logger::ERROR, "\"%s\" does not exist or is not a file.", fname.c_str());
        assert(false);
        return;
    }

    fastgltf::Asset asset;
    {
        // Parse gltf file into asset data structure.
        fastgltf::Parser parser{ fastgltf::Extensions::None };

        constexpr auto k_gltf_options{
            fastgltf::Options::LoadExternalBuffers |
            fastgltf::Options::DecomposeNodeMatrices |  // To ensure node trans is TRS variant.
            fastgltf::Options::GenerateMeshIndices };
        
        auto gltf_file{ fastgltf::MappedGltfFile::FromPath(fname) };
        if (!bool(gltf_file))
        {
            logger::printef(logger::ERROR,
                            "Failed to open glTF file: %s (err msg: %s)",
                            fname.c_str(),
                            fastgltf::getErrorMessage(gltf_file.error()));
            assert(false);
            return;
        }

        auto possible_asset{
            parser.loadGltf(gltf_file.get(),
                            std::filesystem::path{ fname }.parent_path(),
                            k_gltf_options) };
        if (possible_asset.error() != fastgltf::Error::None)
        {
            logger::printef(logger::ERROR,
                            "Failed to load glTF asset from file: %s (err msg: %s)",
                            fname.c_str(),
                            fastgltf::getErrorMessage(possible_asset.error()));
            assert(false);
            return;
        }

        asset = std::move(possible_asset.get());
    }

    // Calculate all nodes' global transform.
    std::unordered_map<size_t, mat4s> node_idx_to_global_transform_map;
    {
        std::vector<size_t> root_node_indices;
        {   // Build list of root nodes.
            std::vector<bool> per_node_is_root_node;
            per_node_is_root_node.resize(asset.nodes.size(), true);
            for (auto& node : asset.nodes)
            {   // Mark found children nodes as not a root node.
                for (size_t child_idx : node.children)
                    per_node_is_root_node[child_idx] = false;
            }

            // Return list of root nodes.
            for (size_t i = 0; i < per_node_is_root_node.size(); i++)
                if (per_node_is_root_node[i])
                    root_node_indices.emplace_back(i);
        }

        // Calc all global transforms.
        // @NOTE: Queue method is used to accomplish breadth first.
        struct Process_node
        {
            size_t parent_idx;
            size_t my_idx;
        };
        constexpr size_t k_invalid{ (size_t)-1 };

        std::list<Process_node> process_nodes;
        for (size_t root_node_idx : root_node_indices)
            process_nodes.emplace_back(k_invalid, root_node_idx);

        while (!process_nodes.empty())
        {   // Process front node in queue.
            auto process_node{ process_nodes.front() };
            process_nodes.pop_front();

            mat4 parent_transform = GLM_MAT4_IDENTITY_INIT;
            if (process_node.parent_idx != k_invalid)
            {   // Read parent transform.
                glm_mat4_copy(
                    node_idx_to_global_transform_map.at(process_node.parent_idx).raw,
                    parent_transform);
            }

            auto trs{ std::get<fastgltf::TRS>(asset.nodes[process_node.my_idx].transform) };
            mat4s node_transform;
            glm_translate_make(node_transform.raw, trs.translation.data());
            glm_quat_rotate(node_transform.raw,
                            versor{ trs.rotation.x(),  // Quat copy uses SIMD which conflicts
                                    trs.rotation.y(),  // memory-wise with the fastgltf setup.
                                    trs.rotation.z(),
                                    trs.rotation.w() },
                            node_transform.raw);
            glm_scale(node_transform.raw, trs.scale.data());

            glm_mat4_mul(parent_transform, node_transform.raw, node_transform.raw);
            // glm_mat4_mul(node_transform.raw, parent_transform, node_transform.raw);

            node_idx_to_global_transform_map.emplace(process_node.my_idx,
                                                     std::move(node_transform));

            // Add children as process nodes.
            for (size_t child_node_idx : asset.nodes[process_node.my_idx].children)
                process_nodes.emplace_back(process_node.my_idx, child_node_idx);
        }
    }

    // Load skins.
    std::unordered_map<size_t, size_t> node_idx_to_model_joint_idx_map;  // @NOTE: Need for rest of loading procs.
    std::unordered_map<size_t, size_t> gltf_asset_joint_idx_to_insert_order_map;  // @NOTE: For remapping joint indices.
    std::vector<size_t> node_index_insert_order;  // @NOTE: Need for animation creation.

    if (asset.skins.size() > 1)
    {
        logger::printef(logger::WARN,
                        "glTF asset has more than 1 skin when only 1 skin is supported. Skins: %lld",
                        asset.skins.size());
        assert(false);  // For debug purposes.
    }

    bool first_skin_w_joints{ true };
    for (auto& skin : asset.skins)
    if (!skin.joints.empty())
    {   // Ensure that only one skin is processed.
        assert(first_skin_w_joints);
        first_skin_w_joints = false;

        // Load all joint data.
        std::vector<mat4s> inv_bind_mats;
        {   // Load all inverse bind matrices.
            auto& inv_bind_mats_accessor{
                asset.accessors[skin.inverseBindMatrices.value()] };
            for (auto element :
                     fastgltf::iterateAccessor<fastgltf::math::fmat4x4>(asset,
                                                                        inv_bind_mats_accessor))
            {
                // @NOTE: `_ucopy()` is used instead of the normal copy to force no SIMD.
                mat4s new_inv_bind_mat;
                glm_mat4_ucopy(reinterpret_cast<vec4*>(element.data()), new_inv_bind_mat.raw);
                inv_bind_mats.emplace_back(std::move(new_inv_bind_mat));
            }
        }

        size_t root_joint_node_idx;
        std::unordered_map<size_t, size_t> node_idx_to_inv_bind_mat_idx_map;
        {   // Build child-to-parent map and node-to-inverse_bind_matrices map.
            std::unordered_map<size_t, size_t> child_to_parent_map;
            size_t inv_bind_mat_idx{ 0 };
            for (auto joint_node_idx : skin.joints)
            {
                node_idx_to_inv_bind_mat_idx_map.emplace(joint_node_idx, inv_bind_mat_idx);
                for (auto child_node_idx : asset.nodes[joint_node_idx].children)
                {
                    if (std::find(skin.joints.begin(), skin.joints.end(), child_node_idx)
                        == skin.joints.end())
                    {   // Child of the joint node is not a joint node.
                        logger::printe(logger::ERROR, "Child of joint node is not a joint node.");
                        assert(false);
                        return;
                    }
                    child_to_parent_map.emplace(child_node_idx, joint_node_idx);
                }
                inv_bind_mat_idx++;
            }

            // Find joint root node.
            root_joint_node_idx = skin.joints.front();
            while (child_to_parent_map.find(root_joint_node_idx) != child_to_parent_map.end())
                root_joint_node_idx = child_to_parent_map.at(root_joint_node_idx);
        }

        {   // Calc root node inverse global transform.
            // Find skin node idx.
            size_t skin_idx{ (size_t)-1 };
            for (size_t i = 0; i < asset.skins.size(); i++)
                if (&asset.skins[i] == &skin)
                {
                    skin_idx = i;
                    break;
                }

            size_t skin_node_idx{ (size_t)-1 };
            for (size_t i = 0; i < asset.nodes.size(); i++)
                if (asset.nodes[i].skinIndex.has_value() &&
                    asset.nodes[i].skinIndex.value() == skin_idx)
                {
                    skin_node_idx = i;
                    break;
                }

            // Spit out skin node inverse transform.
            glm_mat4_inv_precise(node_idx_to_global_transform_map.at(skin_node_idx).raw,
                                 m_model_skin.inverse_global_transform);
        }

        {   // Calc full child to parent node map.
            std::unordered_map<size_t, size_t> child_to_parent_node_idx_map;
            for (size_t parent_node_idx = 0; parent_node_idx < asset.nodes.size(); parent_node_idx++)
                for (size_t child_node_idx : asset.nodes[parent_node_idx].children)
                {
                    child_to_parent_node_idx_map.emplace(child_node_idx, parent_node_idx);
                }

            // Calc baseline transform.
            if (child_to_parent_node_idx_map.find(root_joint_node_idx) != child_to_parent_node_idx_map.end())
            {
                glm_mat4_copy(
                    node_idx_to_global_transform_map.at(
                        child_to_parent_node_idx_map.at(root_joint_node_idx)).raw,
                    m_model_skin.baseline_transform);
            }
            else
            {
                glm_mat4_identity(m_model_skin.baseline_transform);
            }
        }

        std::unordered_map<size_t, size_t> node_idx_to_gltf_joint_idx_map;
        for (size_t gji = 0; gji < skin.joints.size(); gji++)
        {   // Create mapping from node idx to gltf joint idx.
            node_idx_to_gltf_joint_idx_map.emplace(skin.joints[gji], gji);
        }

        {   // Write model joints into model skin.
            struct Node_process_job
            {
                size_t node_idx;
                size_t inv_bind_mat_idx;
            };
            std::list<Node_process_job> process_jobs;

            // Enter root job.
            process_jobs.emplace_back(root_joint_node_idx,
                                      node_idx_to_inv_bind_mat_idx_map.at(root_joint_node_idx));

            // Process jobs while adding more in a breadth-first way.
            node_idx_to_model_joint_idx_map.clear();
            gltf_asset_joint_idx_to_insert_order_map.clear();
            node_index_insert_order.clear();
            while (!process_jobs.empty())
            {
                auto job{ process_jobs.front() };
                process_jobs.pop_front();

                node_idx_to_model_joint_idx_map.emplace(
                    job.node_idx,
                    m_model_skin.joints_sorted_breadth_first.size());

                Model_joint new_model_joint{
                    std::string(asset.nodes[job.node_idx].name), };
                glm_mat4_copy(inv_bind_mats[job.inv_bind_mat_idx].raw,
                              new_model_joint.inverse_bind_matrix);
                // @NOTE: Add parent-child relation later.

                m_model_skin.joints_sorted_breadth_first.emplace_back(new_model_joint);
                gltf_asset_joint_idx_to_insert_order_map.emplace(
                    node_idx_to_gltf_joint_idx_map.at(job.node_idx),
                    node_index_insert_order.size());
                node_index_insert_order.emplace_back(job.node_idx);

                for (auto child_node_idx : asset.nodes[job.node_idx].children)
                {
                    process_jobs.emplace_back(child_node_idx,
                                              node_idx_to_inv_bind_mat_idx_map.at(
                                                  child_node_idx));
                }
            }
            // Emplace for zero case (if zero case already exists then nothing happens with `emplace()`).
            gltf_asset_joint_idx_to_insert_order_map.emplace(0, 0);

            // Add parent-child relationships.
            assert(m_model_skin.joints_sorted_breadth_first.size() == node_index_insert_order.size());
            for (size_t node_idx : node_index_insert_order)
                for (size_t child_idx : asset.nodes[node_idx].children)
                {
                    // Establish parent-child relation.
                    size_t parent_model_joint_idx{ node_idx_to_model_joint_idx_map.at(node_idx) };
                    size_t child_model_joint_idx{ node_idx_to_model_joint_idx_map.at(child_idx) };

                    auto& parent_joint{ m_model_skin.joints_sorted_breadth_first[parent_model_joint_idx] };
                    auto& child_joint{ m_model_skin.joints_sorted_breadth_first[child_model_joint_idx] };

                    child_joint.parent_idx = parent_model_joint_idx;
                    parent_joint.children.emplace_back(&child_joint);
                }
        }

        // @NOTE: Ignore the `skeleton` property in the `Skin` struct.
    }

    // Load meshes.
    bool overall_has_skin{ !asset.skins.empty() };
    m_vertices.clear();
    m_model_aabb.reset();

    size_t num_meshes{ 0 };
    for (auto& mesh : asset.meshes)
        num_meshes += mesh.primitives.size();

    m_meshes.clear();
    m_meshes.reserve(num_meshes);  // @NOTE: Reserve prevents calling dtor() which messes up the meshes.

    for (auto& mesh : asset.meshes)
        for (auto& primitive : mesh.primitives)
        {   // Load vertices.
            // Find all wanted accessors.
            auto pos_attribute{ primitive.findAttribute("POSITION") };
            auto norm_attribute{ primitive.findAttribute("NORMAL") };
            auto tex_coord_attribute{ primitive.findAttribute("TEXCOORD_0") };
            auto joints_attribute{ primitive.findAttribute("JOINTS_0") };
            auto weights_attribute{ primitive.findAttribute("WEIGHTS_0") };

            assert(pos_attribute != nullptr);  // POSITION is definitely required.
            assert(norm_attribute != nullptr);
            assert(tex_coord_attribute != nullptr);
            assert((joints_attribute != nullptr) == (weights_attribute != nullptr));

            auto& pos_accessor{ asset.accessors[pos_attribute->accessorIndex] };
            auto& norm_accessor{ asset.accessors[norm_attribute->accessorIndex] };
            auto& tex_coord_accessor{ asset.accessors[tex_coord_attribute->accessorIndex] };
            fastgltf::Accessor* joints_accessor{ nullptr };
            fastgltf::Accessor* weights_accessor{ nullptr };
            bool has_skin{ false };

            if (joints_attribute != nullptr && weights_attribute != nullptr)
            {   // Include skinning accessors.
                joints_accessor = &asset.accessors[joints_attribute->accessorIndex];
                weights_accessor = &asset.accessors[weights_attribute->accessorIndex];
                has_skin = true;
            }

            // Either all meshes must have/not have a skin, with the exception of
            // overall meshes having skins but this one in particular doesn't.
            // A dummy set of skin weights will be applied later for this exception.
            assert(overall_has_skin == has_skin ||
                   (overall_has_skin && !has_skin));

            // Resize to include new vertices.
            auto base_vertex_idx{ m_vertices.size() };
            m_vertices.resize(base_vertex_idx + pos_accessor.count);
            if (overall_has_skin)
            {   // Include vertex skin data even if this mesh does not have skin.
                m_vert_skin_datas.resize(base_vertex_idx + pos_accessor.count);
            }

            // Load data for new vertices.
            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, pos_accessor,
                [this, base_vertex_idx](fastgltf::math::fvec3 v, size_t index) {
                    m_model_aabb.feed_position(v.data());
                    glm_vec3_copy(v.data(),
                                  m_vertices[base_vertex_idx + index].position);
                });

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec3>(asset, norm_accessor,
                [this, base_vertex_idx](fastgltf::math::fvec3 v, size_t index) {
                    glm_vec3_copy(v.data(),
                                  m_vertices[base_vertex_idx + index].normal);
                });

            fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec2>(asset, tex_coord_accessor,
                [this, base_vertex_idx](fastgltf::math::fvec2 v, size_t index) {
                    glm_vec2_copy(v.data(),
                                  m_vertices[base_vertex_idx + index].tex_coord);
                });

            if (has_skin)
            {   // Joint indices.
                switch (joints_accessor->componentType)
                {
                    case fastgltf::ComponentType::UnsignedByte:
                        fastgltf::iterateAccessorWithIndex<fastgltf::math::u8vec4>(asset, *joints_accessor,
                            [this, base_vertex_idx, &gltf_asset_joint_idx_to_insert_order_map]
                            (fastgltf::math::u8vec4 v, size_t index) {
                                m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[0] =
                                    gltf_asset_joint_idx_to_insert_order_map.at(v.x());
                                m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[1] =
                                    gltf_asset_joint_idx_to_insert_order_map.at(v.y());
                                m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[2] =
                                    gltf_asset_joint_idx_to_insert_order_map.at(v.z());
                                m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[3] =
                                    gltf_asset_joint_idx_to_insert_order_map.at(v.w());
                            });
                        break;

                    case fastgltf::ComponentType::UnsignedShort:
                        fastgltf::iterateAccessorWithIndex<fastgltf::math::u16vec4>(asset, *joints_accessor,
                            [this, base_vertex_idx, &gltf_asset_joint_idx_to_insert_order_map]
                            (fastgltf::math::u16vec4 v, size_t index) {
                                m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[0] =
                                    gltf_asset_joint_idx_to_insert_order_map.at(v.x());
                                m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[1] =
                                    gltf_asset_joint_idx_to_insert_order_map.at(v.y());
                                m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[2] =
                                    gltf_asset_joint_idx_to_insert_order_map.at(v.z());
                                m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[3] =
                                    gltf_asset_joint_idx_to_insert_order_map.at(v.w());
                            });
                        break;

                    default:
                        // Component type for joint indices not supported.
                        assert(false);
                        return;
                }

                // Weights.
                fastgltf::iterateAccessorWithIndex<fastgltf::math::fvec4>(asset, *weights_accessor,
                    [this, base_vertex_idx](fastgltf::math::fvec4 v, size_t index) {
                        glm_vec4_copy(v.data(),
                                      m_vert_skin_datas[base_vertex_idx + index].weights);
                    });
            }
            else if (overall_has_skin && !has_skin)
            {   // Joint indices (dummy).
                for (size_t index = 0; index < pos_accessor.count; index++)
                {
                    m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[0] = 0;
                    m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[1] = 0;
                    m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[2] = 0;
                    m_vert_skin_datas[base_vertex_idx + index].joint_mat_idxs[3] = 0;
                }

                // Weights (dummy).
                for (size_t index = 0; index < pos_accessor.count; index++)
                {
                    glm_vec4_zero(m_vert_skin_datas[base_vertex_idx + index].weights);
                }
            }

            // Load all indices.
            assert(primitive.indicesAccessor.has_value());
            auto& indices_accessor{ asset.accessors[primitive.indicesAccessor.value()] };

            std::vector<uint32_t> indices;
            indices.reserve(indices_accessor.count);

            for (uint32_t ind :
                     fastgltf::iterateAccessor<uint32_t>(asset, indices_accessor))
            {
                // Offset indices to ensure they're referencing the correct
                // mesh.
                indices.emplace_back(base_vertex_idx + ind);
            }

            // Create mesh in model.
            m_meshes.emplace_back(std::move(indices), material_name);
        }

    {   // Upload vertices to GPU.
        glGenVertexArrays(1, &m_model_vertex_vao);
        glGenBuffers(1, &m_model_vertex_vbo);

        glBindVertexArray(m_model_vertex_vao);
        glBindBuffer(GL_ARRAY_BUFFER, m_model_vertex_vbo);
        glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_STATIC_DRAW);

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

        if (overall_has_skin)
        {   // Upload vertex skin datas to GPU as well.
            glGenBuffers(1, &m_model_vertex_skin_datas_buffer);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_model_vertex_skin_datas_buffer);
            glBufferData(GL_SHADER_STORAGE_BUFFER,
                         m_vert_skin_datas.size() * sizeof(Vertex_skin_data),
                         m_vert_skin_datas.data(),
                         GL_STATIC_READ);
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        }
    }

    // Load animations.
    m_animations.clear();
    m_animations.reserve(asset.animations.size());
    for (auto& anim : asset.animations)
    {
        // Set anim name.
        std::string anim_name{ anim.name };
        if (anim_name.empty())
        {
            anim_name = std::to_string(m_animations.size());
        }

        // Extract glTF-style animation data.
        struct Sampler_extracted_data
        {
            fastgltf::AnimationInterpolation interp_type;  // @NOTE: 永久に使われないかも。
            std::vector<float_t> times;
            std::vector<vec4s> trs_fragments;  // Could be trans, rot, or sca.
        };
        std::unordered_map<size_t, Sampler_extracted_data> sampler_idx_to_data_map;

        struct Channel_extracted_data
        {
            Sampler_extracted_data* sampler{ nullptr };
            size_t target_joint_idx;  // Idx in `joints_sorted_breadth_first`.
            fastgltf::AnimationPath trs_type;
        };
        std::vector<Channel_extracted_data> channel_datas;

        bool animation_data_invalid{ false };

        {   // Organize samplers.
            for (size_t i = 0; i < anim.samplers.size(); i++)
            {
                auto& sampler{ anim.samplers[i] };

                Sampler_extracted_data new_data;

                new_data.interp_type = sampler.interpolation;
                if (new_data.interp_type == fastgltf::AnimationInterpolation::Step)
                {
                    logger::printe(logger::WARN,
                                   "`Step` animation interpolation type not supported. May be "
                                   "supported in the future but for now it will just be imported as `Linear`.");
                    assert(false);
                }
                if (new_data.interp_type == fastgltf::AnimationInterpolation::CubicSpline)
                {
                    logger::printe(logger::ERROR,
                                   "`CubicSpline` animation interpolation type not supported.");
                    assert(false);
                    return;
                }

                {   // Get `.times` (sampler input).
                    auto& input_accessor{ asset.accessors[sampler.inputAccessor] };
                    assert(input_accessor.componentType == fastgltf::ComponentType::Float);
                    for (float_t element :
                         fastgltf::iterateAccessor<float_t>(asset, input_accessor))
                    {
                        new_data.times.emplace_back(element);
                    }
                }

                {   // Get `.trs_fragments` (sampler output).
                    auto& output_accessor{ asset.accessors[sampler.outputAccessor] };
                    assert(output_accessor.componentType == fastgltf::ComponentType::Float);

                    switch (output_accessor.type)
                    {
                        case fastgltf::AccessorType::Vec3:
                            for (fastgltf::math::fvec3 element :
                                 fastgltf::iterateAccessor<fastgltf::math::fvec3>(asset,
                                                                                  output_accessor))
                            {
                                new_data.trs_fragments.emplace_back(vec4s{ element.x(),
                                                                           element.y(),
                                                                           element.z(),
                                                                           0.0f });
                            }
                            break;

                        case fastgltf::AccessorType::Vec4:
                            for (fastgltf::math::fvec4 element :
                                 fastgltf::iterateAccessor<fastgltf::math::fvec4>(asset,
                                                                                  output_accessor))
                            {
                                new_data.trs_fragments.emplace_back(vec4s{ element.x(),
                                                                           element.y(),
                                                                           element.z(),
                                                                           element.w() });
                            }
                            break;

                        default:
                            // Huh???
                            assert(false);
                            return;
                    }
                    // @NOTE: Please don't call me lazy but I just didn't want a big branch between
                    //   two `for` loops that would look pretty much the same  >.<
                    //     -Thea 2025/07/12 (I turned in my passport app w/ the correct gender today!
                    //                       Hopefully I don't get struck down by the tyrants)
                    // @AMEND: Turns out that I had to split it between two branches bc `iterateAccessor`
                    //   didn't like the `float_t` type for the accessor.  -Thea 2025/07/14
                }

                // Emplace.
                sampler_idx_to_data_map.emplace(i, new_data);
            }

            // Check for sampler times sorted.
            for (auto& sampler : sampler_idx_to_data_map)
            {
                float_t prev_time{ std::numeric_limits<float_t>::lowest() };
                for (float_t time : sampler.second.times)
                {
                    if (prev_time >= time)
                    {
                        logger::printef(logger::ERROR,
                                        "Times are not sorted asc! prev: %.6f curr: %.6f",
                                        prev_time,
                                        time);
                        assert(false);
                        return;  // Abort loading.
                    }
                    prev_time = time;
                }
            }

            // Organize channels.
            for (auto& channel : anim.channels)
            {
                if (node_idx_to_model_joint_idx_map.find(channel.nodeIndex.value())
                    == node_idx_to_model_joint_idx_map.end())
                {   // This channel is found to reference an invalid node. Handle by skipping this animation.
                    animation_data_invalid = true;
                }
                else
                {   // Add channel information to data extraction struct.
                    channel_datas.emplace_back(&sampler_idx_to_data_map.at(channel.samplerIndex),
                                               node_idx_to_model_joint_idx_map.at(
                                                   channel.nodeIndex.value()),
                                               channel.path);
                }

            }
        }

        if (animation_data_invalid)
        {   // Abort trying to import this animation and skip to next one.
            logger::printef(logger::WARN,
                            "Animation data for anim \"%s\" is invalid. Skipping.",
                            anim_name.c_str());
            continue;
        }

        // Convert glTF-style data to `Model_animator` data.
        constexpr float_t k_anim_frame_time{
            1.0f / Model_joint_animation::k_frames_per_second };
        float_t start_time{ std::numeric_limits<float_t>::max() };
        float_t end_time{ std::numeric_limits<float_t>::lowest() };
        uint32_t perceived_frames{ 0 };
        {   // Find start/end times of all samplers.
            for (auto& elem : sampler_idx_to_data_map)
            {
                auto& sampler{ elem.second };
                for (float_t time : sampler.times)
                {
                    start_time = std::min(start_time, time);
                    end_time = std::max(end_time, time);
                }
            }

            // Calculate the frames between the times.
            float_t num_frames_raw{ ((end_time - start_time) / k_anim_frame_time) + 1.0f };
            float_t deviation{ num_frames_raw - std::roundf(num_frames_raw) };
            if (abs(deviation) > 1e-6f)
            {
                logger::printef(logger::WARN,
                                "Animation length does not match the %.3f hz animation cutting "
                                "requirement (deviation: %0.6f). Will extend animation clip until "
                                "the cutting requirement is fulfilled.",
                                Model_joint_animation::k_frames_per_second,
                                deviation);
                // @NOCHECKIN: Don't assert on this warning for now.
                // assert(false);  // Idk if you want an assert on this, but it's a heavier warning.
            }

            // Turn into perceived frames.
            perceived_frames = std::ceilf(num_frames_raw);
        }

        std::vector<Model_joint_animation_frame> new_anim_frames;
        new_anim_frames.reserve(perceived_frames);
        {   // Record animation frames.
            float_t curr_time{ start_time };
            for (size_t _ = 0; _ < perceived_frames; _++)
            {   // Get interpolation of current frame.
                std::unordered_map<size_t, Model_joint_animation_frame::Joint_local_transform>
                    joint_idx_to_local_trans_map;
                for (auto const& channel : channel_datas)
                {
                    if (joint_idx_to_local_trans_map.find(channel.target_joint_idx)
                            == joint_idx_to_local_trans_map.end())
                    {   // @NOTE: Emplace in the beginning since channels only
                        // sample one part of the TRS.
                        joint_idx_to_local_trans_map.emplace(
                            channel.target_joint_idx,
                            Model_joint_animation_frame::Joint_local_transform{});
                    }

                    Model_joint_animation_frame::Joint_local_transform& joint_trans{
                        joint_idx_to_local_trans_map.at(channel.target_joint_idx) };

                    bool found_sample{ false };
                    assert(channel.sampler->times.size() >= 2);
                    for (size_t i = 0; i < channel.sampler->times.size() - 1; i++)
                        if (curr_time >= channel.sampler->times[i] &&
                            curr_time <= channel.sampler->times[i + 1])
                        {
                            float_t interp_t{ std::max(0.0f, curr_time - channel.sampler->times[i])
                                                  / (channel.sampler->times[i + 1] - channel.sampler->times[i]) };
                            auto const& output0{ channel.sampler->trs_fragments[i] };
                            auto const& output1{ channel.sampler->trs_fragments[i + 1] };
                            switch (channel.trs_type)
                            {
                                case fastgltf::AnimationPath::Translation:
                                    glm_vec3_lerp(const_cast<float_t*>(output0.raw),
                                                  const_cast<float_t*>(output1.raw),
                                                  interp_t,
                                                  joint_trans.position);
                                    break;

                                case fastgltf::AnimationPath::Rotation:
                                    // @NOTE: Using `slerp()` for better accuracy than `nlerp()`.
                                    //   This is fine since it's just the import stage, not actual
                                    //   animation.
                                    glm_quat_slerp(const_cast<float_t*>(output0.raw),
                                                   const_cast<float_t*>(output1.raw),
                                                   interp_t,
                                                   joint_trans.rotation);
                                    break;

                                case fastgltf::AnimationPath::Scale:
                                    glm_vec3_lerp(const_cast<float_t*>(output0.raw),
                                                  const_cast<float_t*>(output1.raw),
                                                  interp_t,
                                                  joint_trans.scale);
                                    break;

                                case fastgltf::AnimationPath::Weights:
                                    // Not supported.
                                    assert(false);
                                    break;

                                default:
                                    // Huh?
                                    assert(false);
                                    return;
                            }

                            found_sample = true;
                            break;
                        }
                    assert(found_sample);
                }

                // Create animation frame from pose.
                Model_joint_animation_frame new_frame;
                new_frame.joint_transforms_in_order.reserve(
                    m_model_skin.joints_sorted_breadth_first.size());

                // @NOTE: This uses `i` the index of the model joint list (sorted breadth first)
                //   to access the local trans map (instead of contents of `node_index_insert_order`
                //   which is WRONG)  -Thea 2025/07/20
                for (size_t i = 0; i < m_model_skin.joints_sorted_breadth_first.size(); i++)
                {
                    new_frame.joint_transforms_in_order.emplace_back(
                        std::move(joint_idx_to_local_trans_map.at(i)));
                }
                new_anim_frames.emplace_back(std::move(new_frame));

                // Tick next frame (and clamp to end time for any possible extra frames).
                curr_time = std::min(curr_time + k_anim_frame_time,
                                     end_time);
            }
        }

        // Create animation.
        m_animations.emplace_back(std::ref(m_model_skin),
                                  anim_name,
                                  std::move(new_anim_frames));
    }
}


// @NOTE: vvv `Deformed_model` vvv
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

    // Create ssbo for storing joint matrices.
    glGenBuffers(1, &m_mesh_joint_deform_data_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_mesh_joint_deform_data_ssbo);
    glBufferData(GL_SHADER_STORAGE_BUFFER, k_max_num_joints * sizeof(mat4), nullptr, GL_DYNAMIC_COPY);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

BT::Deformed_model::~Deformed_model()
{
    glDeleteBuffers(1, &m_deform_vertex_vbo);
    glDeleteVertexArrays(1, &m_deform_vertex_vao);
}

void BT::Deformed_model::dispatch_compute_deform(vector<mat4s>&& joint_matrices)
{
    // Upload joint matrices to GPU.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_mesh_joint_deform_data_ssbo);
    glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, joint_matrices.size() * sizeof(mat4), joint_matrices.data());
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // Dispatch compute.
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_model.m_model_vertex_vbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_model.m_model_vertex_vbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_model.m_model_vertex_skin_datas_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, m_model.m_model_vertex_skin_datas_buffer);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_mesh_joint_deform_data_ssbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, m_mesh_joint_deform_data_ssbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, m_deform_vertex_vbo);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, m_deform_vertex_vbo);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    static auto& s_shader{ *Shader_bank::get_shader("skinned_mesh_compute") };
    s_shader.bind();
    size_t num_vertices{ m_model.m_vertices.size() };
    s_shader.set_uint("num_vertices", num_vertices);
    glDispatchCompute(std::ceilf(static_cast<float_t>(num_vertices)
                                 / 256.0f),  // @NOTE: Must match compute shader `local_size_x`.
                      1,
                      1);
    s_shader.unbind();

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, 0);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
}

std::string BT::Deformed_model::get_model_name() const
{
    return Model_bank::get_model_name(&m_model);
}

void BT::Deformed_model::render(mat4 transform, Material_ifc* override_material /*= nullptr*/) const
{
    // @NOTE: All meshes share the same vertices, just use different indices.
    glBindVertexArray(m_deform_vertex_vao);

    for (auto& mesh : m_model.m_meshes)  // Use the regular model index buffers.
    {
        mesh.render_mesh(transform, override_material);
    }

    glBindVertexArray(0);
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

vector<string> BT::Model_bank::get_all_model_names()
{
    vector<string> model_names;
    model_names.reserve(s_models.size());

    for (auto& model : s_models)
    {
        model_names.emplace_back(model.first);
    }

    return model_names;
}
