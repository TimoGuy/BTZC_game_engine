#include "mesh.h"

#include "cglm/mat4.h"
#include "cglm/vec2-ext.h"
#include "cglm/vec3-ext.h"
#include "cglm/vec3.h"
#include "fastgltf/core.hpp"
#include "fastgltf/types.hpp"
#include "fastgltf/tools.hpp"
#include "glad/glad.h"
#include "logger.h"
#include "material.h"
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

    // Load meshes.
    for (auto& mesh : asset.meshes)
    {
        // Position.
        // Normal.
        // Tex coord.

        // If there is a skin associated with the model:
            // Joints.
            // Weights.
        assert(false);
    }

    // Load skins.
    std::unordered_map<size_t, size_t> node_idx_to_model_joint_idx_map;  // @NOTE: Need for rest of loading procs.

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
                // @HERE @TODO Copy in the inverse bind matrices.
                assert(false);
            }
        }

        size_t root_node_idx;
        std::unordered_map<size_t, size_t> node_idx_to_inv_bind_mat_idx_map;
        {   // Build child-to-parent map and node-to-inverse_bind_matrices map.
            std::unordered_map<size_t, size_t> child_to_parent_map;
            size_t inv_bind_mat_idx{ 0 };
            for (auto joint_node_idx : skin.joints)
            {
                node_idx_to_inv_bind_mat_idx_map.emplace(joint_node_idx, inv_bind_mat_idx);
                for (auto child_node_idx : asset.nodes[joint_node_idx].children)
                    child_to_parent_map.emplace(child_node_idx, joint_node_idx);
            }

            // Find root node.
            root_node_idx = skin.joints.front();
            while (child_to_parent_map.find(root_node_idx) != child_to_parent_map.end())
                root_node_idx = child_to_parent_map.at(root_node_idx);
        }

        {   // Calc root node inverse global transform.
            auto global_transform{
                std::get<fastgltf::math::fmat4x4>(asset.nodes[root_node_idx].transform) };
            glm_mat4_inv_precise(reinterpret_cast<vec4*>(global_transform.data()),
                                 m_model_skin.inverse_global_transform);
            // @TODO: Check if using fastgltf mat4 like this^^ works.
            assert(false);
        }

        {   // Write model joints into model skin.
            struct Node_process_job
            {
                size_t node_idx;
                size_t inv_bind_mat_idx;
            };
            std::list<Node_process_job> process_jobs;

            // Enter root job.
            process_jobs.emplace_back(root_node_idx,
                                      node_idx_to_inv_bind_mat_idx_map.at(root_node_idx));
            
            // Process jobs while adding more in a breadth-first way.
            node_idx_to_model_joint_idx_map.clear();
            std::vector<size_t> node_index_insert_order;
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
                node_index_insert_order.emplace_back(job.node_idx);
            }

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

        // @TODO: @CHECK: That the above^^ is actually doing what it says it is.
        assert(false);
    }

    // Load animations.
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
            fastgltf::AnimationInterpolation interp_type;
            std::vector<float_t> times;
            std::vector<vec4s> trs_fragments;  // Could be trans, rot, or sca.
        };
        std::unordered_map<size_t, Sampler_extracted_data> sampler_idx_to_data_map;

        struct Channel_extracted_data
        {
            Sampler_extracted_data* sampler{ nullptr };
            Model_joint* target_joint{ nullptr };
            fastgltf::AnimationPath trs_type;
        };
        std::vector<Channel_extracted_data> channel_datas;

        {   // Organize samplers.
            for (size_t i = 0; i < anim.samplers.size(); i++)
            {
                auto& sampler{ anim.samplers[i] };

                Sampler_extracted_data new_data;

                new_data.interp_type = sampler.interpolation;
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
                    uint32_t xyzw_count{ 0 };
                    uint32_t vec_len{ output_accessor.type == fastgltf::AccessorType::Vec4 ? 4u : 3u };
                    for (float_t element :
                         fastgltf::iterateAccessor<float_t>(asset, output_accessor))
                    {
                        if (xyzw_count == 0)
                        {   // Create new vec4.
                            new_data.trs_fragments.emplace_back();
                        }

                        new_data.trs_fragments.back().raw[xyzw_count] = element;

                        if (vec_len == 3 && xyzw_count == 2)
                        {   // Also insert `.w` but as 0.0f.
                            new_data.trs_fragments.back().w = 0.0f;
                        }

                        xyzw_count = (xyzw_count + 1) % vec_len;
                    }

                    // Should be back to 0 by the end.
                    // @NOTE: Please don't call me lazy but I just didn't want a big branch between
                    //   two `for` loops that would look pretty much the same  >.<
                    //     -Thea 2025/07/12 (I turned in my passport app w/ the correct gender today!
                    //                       Hopefully I don't get struck down by the tyrants)
                    assert(xyzw_count == 0);
                }

                // Emplace.
                sampler_idx_to_data_map.emplace(i, new_data);
            }

            // Check for sampler times sorted.
            for (auto& sampler : sampler_idx_to_data_map)
            {
                float_t prev_time{ std::numeric_limits<float_t>::min() };
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
                channel_datas.emplace_back(&sampler_idx_to_data_map.at(channel.samplerIndex),
                                           &m_model_skin.joints_sorted_breadth_first[
                                               node_idx_to_model_joint_idx_map.at(
                                                   channel.nodeIndex.value())],
                                           channel.path);
            }
        }

        // Convert glTF-style data to `Model_animator` data.
        // @TODO: @HERE
    }
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
