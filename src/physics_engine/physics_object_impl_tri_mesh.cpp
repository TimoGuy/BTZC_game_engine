#include "physics_object_impl_tri_mesh.h"

#include "../renderer/mesh.h"
#include "Jolt/Geometry/IndexedTriangle.h"
#include "Jolt/Geometry/Triangle.h"
#include "Jolt/Jolt.h"
#include "Jolt/Math/Float3.h"
#include "Jolt/Physics/Body/BodyCreationSettings.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Collision/Shape/MeshShape.h"
#include "Jolt/Physics/EActivation.h"
#include "logger.h"
#include "physics_engine.h"
#include "physics_engine_impl_layers.h"
#include <cassert>
#include <iostream> // @NOCHECKIN!!!


BT::Phys_obj_impl_tri_mesh::Phys_obj_impl_tri_mesh(Physics_engine& phys_engine,
                                                   Model const* model,
                                                   JPH::EMotionType motion_type,
                                                   Physics_transform&& init_transform)
    : m_phys_body_ifc{ *reinterpret_cast<JPH::BodyInterface*>(phys_engine.get_physics_body_ifc()) }
    , m_can_move{ motion_type == JPH::EMotionType::Kinematic }
{
    if (motion_type == JPH::EMotionType::Dynamic)
    {
        logger::printe(logger::ERROR, "Dynamic motion type not allowed.");
        assert(false);
        return;
    }

    auto verts_indices{ model->get_all_vertices_and_indices() };

    // @NOTE: I think there might be some extra stuff Jolt is doing in the behind
    //   that makes the triangle list better than using the inefficient-for-physics
    //   indexed triangle list from the obj file (or placebo hehe), so I'm gonna
    //   stick to the triangle list below for now.  -Thea 2025/05/29
#define INDEXED_TRIANGLE_LIST 0
#if INDEXED_TRIANGLE_LIST
    JPH::VertexList vertex_list;
    vertex_list.reserve(verts_indices.first.size());
    for (auto& vertex : verts_indices.first)
    {
        vertex_list.emplace_back(vertex.position[0], vertex.position[1], vertex.position[2]);
    }

    assert(verts_indices.second.size() % 3 == 0);
    JPH::IndexedTriangleList indexed_tris_list;
    indexed_tris_list.reserve(verts_indices.second.size() / 3);
    for (size_t i = 0 ; i < verts_indices.second.size(); i += 3)
    {
        indexed_tris_list.emplace_back(verts_indices.second[i + 0],
                                       verts_indices.second[i + 1],
                                       verts_indices.second[i + 2],
                                       0);
    }
    JPH::MeshShapeSettings mesh_settings(vertex_list, indexed_tris_list);
#else
    JPH::TriangleList tri_list;
    for (size_t i = 0; i < verts_indices.second.size(); i += 3)
    {
        JPH::Float3 p0{ verts_indices.first[verts_indices.second[i + 0]].position[0],
                        verts_indices.first[verts_indices.second[i + 0]].position[1],
                        verts_indices.first[verts_indices.second[i + 0]].position[2] };
        JPH::Float3 p1{ verts_indices.first[verts_indices.second[i + 1]].position[0],
                        verts_indices.first[verts_indices.second[i + 1]].position[1],
                        verts_indices.first[verts_indices.second[i + 1]].position[2] };
        JPH::Float3 p2{ verts_indices.first[verts_indices.second[i + 2]].position[0],
                        verts_indices.first[verts_indices.second[i + 2]].position[1],
                        verts_indices.first[verts_indices.second[i + 2]].position[2] };
        tri_list.emplace_back(p0, p1, p2);
    }
    JPH::MeshShapeSettings mesh_settings(tri_list);
#endif  // INDEXED_TRIANGLE_LIST

    mesh_settings.SetEmbedded();
    JPH::BodyCreationSettings mesh_body_settings(&mesh_settings,
                                                 init_transform.position,
                                                 init_transform.rotation,
                                                 motion_type,
                                                 (m_can_move ? Layers::MOVING : Layers::NON_MOVING));
    m_body_id = m_phys_body_ifc.CreateAndAddBody(mesh_body_settings, JPH::EActivation::DontActivate);
}

BT::Phys_obj_impl_tri_mesh::~Phys_obj_impl_tri_mesh()
{
    m_phys_body_ifc.RemoveBody(m_body_id);
}

void BT::Phys_obj_impl_tri_mesh::move_kinematic(Physics_transform&& new_transform)
{
    if (!m_can_move)
    {
        logger::printe(logger::ERROR, "Object marked as unmovable.");
        assert(false);
    }
    m_phys_body_ifc.MoveKinematic(m_body_id,
                                  new_transform.position,
                                  new_transform.rotation,
                                  Physics_engine::k_simulation_delta_time);
}

BT::Physics_transform BT::Phys_obj_impl_tri_mesh::read_transform()
{
    return { m_phys_body_ifc.GetCenterOfMassPosition(m_body_id),
             m_phys_body_ifc.GetRotation(m_body_id) };
}
