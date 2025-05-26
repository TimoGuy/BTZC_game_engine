#include "physics_object_impl_tri_mesh.h"

#include "../renderer/mesh.h"
#include "Jolt/Geometry/IndexedTriangle.h"
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

void BT::Phys_obj_impl_tri_mesh::set_linear_velocity(JPH::Vec3Arg velocity)
{
    (void)velocity;
    logger::printe(logger::ERROR, "Cannot set linear velocity for tri mesh.");
    assert(false);
}

BT::Physics_transform BT::Phys_obj_impl_tri_mesh::read_transform()
{
    return { m_phys_body_ifc.GetCenterOfMassPosition(m_body_id),
             m_phys_body_ifc.GetRotation(m_body_id) };
}
