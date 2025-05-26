#pragma once

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

    Physics_object_type get_type() override { return PHYSICS_OBJECT_TYPE_KINEMATIC_TRIANGLE_MESH; }
    void move_kinematic(Physics_transform&& new_transform) override;
    void set_linear_velocity(JPH::Vec3Arg velocity) override;
    Physics_transform read_transform() override;

private:
    JPH::BodyInterface& m_phys_body_ifc;
    JPH::BodyID m_body_id;
    bool m_can_move;
};

}  // namespace BT
