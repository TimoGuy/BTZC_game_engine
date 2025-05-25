#pragma once

#include "cglm/cglm.h"
#include "Jolt/Jolt.h"
#include "Jolt/Math/MathTypes.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Math/Real.h"
#include <atomic>
#include <memory>
#include <utility>

using JPH::Real;
using std::atomic_size_t;
using std::unique_ptr;
using std::pair;


namespace BT
{

using rvec3 = JPH::Real[3];

enum Physics_object_type
{
    PHYSICS_OBJECT_TYPE_KINEMATIC_TRIANGLE_MESH = 0,
    PHYSICS_OBJECT_TYPE_CHARACTER_CONTROLLER,
    NUM_PHYSICS_OBJECT_TYPES
};

class Physics_object_type_impl_ifc
{
public:
    virtual ~Physics_object_type_impl_ifc() = default;
    virtual Physics_object_type get_type() = 0;
    virtual void move_kinematic() = 0;  // @TODO
    virtual void set_velocity() = 0;  // @TODO
    virtual pair<JPH::RVec3Arg, JPH::QuatArg> read_transform() = 0;
};

struct Physics_transform
{
    JPH::RVec3 position = JPH::RVec3::sZero();
    JPH::Quat rotation = JPH::Quat::sIdentity();
};

class Physics_engine;

class Physics_object
{
public:
    Physics_object(Physics_object_type type, bool interpolate_transform);
    Physics_object(const Physics_object&)            = delete;
    Physics_object(Physics_object&&)                 = delete;
    Physics_object& operator=(const Physics_object&) = delete;
    Physics_object& operator=(Physics_object&&)      = delete;
    
    void set_physics_engine_reference(Physics_engine const* phys_engine) { m_phys_engine = phys_engine; }
    void deposit_new_transform(JPH::RVec3Arg position, JPH::QuatArg rotation);

    void get_transform_for_rendering(rvec3& out_position, versor& out_rotation);

private:
    bool m_interpolate;
    Physics_engine const* m_phys_engine;

    unique_ptr<Physics_object_type_impl_ifc> m_type_pimpl;

    Physics_transform m_transform_triple_buffer[3];

    static constexpr size_t k_trip_buf_read_a{ 0 };
    static constexpr size_t k_trip_buf_read_b{ 1 };
    static constexpr size_t k_trip_buf_write{ 2 };
    atomic_size_t m_trip_buf_offset{ 0 };
};

}  // namespace BT
