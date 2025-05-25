#pragma once

#include "Jolt/Jolt.h"
#include "Jolt/Math/Real.h"
#include "physics_object.h"
#include <vector>

using std::vector;


namespace BT
{

class Phys_obj_impl_kine_tri_mesh : public Physics_object_type_impl_ifc
{
public:
    Phys_obj_impl_kine_tri_mesh(vector<uint32_t> indices,
                                vector<JPH::RVec3> vertices,  // @INCOMPLETE @NOCHECKIN.
                                Physics_transform&& init_transform);
    ~Phys_obj_impl_kine_tri_mesh();

    Physics_object_type get_type() override { return PHYSICS_OBJECT_TYPE_KINEMATIC_TRIANGLE_MESH; }
    void move_kinematic(Physics_transform&& new_transform) override;
    void set_linear_velocity(JPH::Vec3Arg velocity) override;
    Physics_transform read_transform() override;
};

}  // namespace BT