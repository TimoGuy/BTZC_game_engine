#pragma once

#include "../uuid/uuid.h"
#include "Jolt/Jolt.h"
#include "Jolt/Math/Float3.h"
#include "Jolt/Physics/Body/BodyID.h"
#include "Jolt/Physics/Body/BodyInterface.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "physics_object.h"
#include <vector>

using std::vector;


namespace BT
{

class Model;

class Phys_obj_impl_tri_mesh : public Physics_object_type_impl_ifc
{
public:
    Phys_obj_impl_tri_mesh(Physics_engine& phys_engine,
                           Model const* model,
                           JPH::EMotionType motion_type,
                           Physics_transform&& init_transform);
    Phys_obj_impl_tri_mesh(const Phys_obj_impl_tri_mesh&)            = delete;
    Phys_obj_impl_tri_mesh(Phys_obj_impl_tri_mesh&&)                 = delete;
    Phys_obj_impl_tri_mesh& operator=(const Phys_obj_impl_tri_mesh&) = delete;
    Phys_obj_impl_tri_mesh& operator=(Phys_obj_impl_tri_mesh&&)      = delete;
    ~Phys_obj_impl_tri_mesh();

    Physics_object_type get_type() override { return PHYSICS_OBJECT_TYPE_TRIANGLE_MESH; }
    void move_kinematic(Physics_transform&& new_transform) override;
    Physics_transform read_transform() override;
    void update_debug_mesh() override;

    // Scene_serialization_ifc.
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;
private:
    JPH::BodyInterface& m_phys_body_ifc;
    Model const* m_model;  // Save for serialization purposes, and debug rendering purposes.
    JPH::BodyID m_body_id;
    bool m_can_move;

    UUID m_debug_mesh_id;
};

}  // namespace BT
