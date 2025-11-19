#include "physics_object.h"

#include "../renderer/mesh.h"
#include "Jolt/Jolt.h"
#include "Jolt/Math/MathTypes.h"
#include "Jolt/Math/Real.h"
#include "btglm.h"
#include "btlogger.h"
#include "physics_engine.h"
#include "physics_object_impl_char_controller.h"
#include "physics_object_impl_tri_mesh.h"
#include "service_finder/service_finder.h"

#include <memory>
#include <string>

using std::make_unique;
using std::string;


namespace
{

JPH::Vec3 json_to_vec3(json& node_ref)
{
    return JPH::Vec3(node_ref[0], node_ref[1], node_ref[2]);
}

JPH::Quat json_to_quat(json& node_ref)
{
    return JPH::Quat(node_ref[0], node_ref[1], node_ref[2], node_ref[3]);
}

void vec3_to_json(JPH::Vec3 position, json& node_ref)
{
    node_ref[0] = position.GetX();
    node_ref[1] = position.GetY();
    node_ref[2] = position.GetZ();
}

void quat_to_json(JPH::Quat rotation, json& node_ref)
{
    node_ref[0] = rotation.GetX();
    node_ref[1] = rotation.GetY();
    node_ref[2] = rotation.GetZ();
    node_ref[3] = rotation.GetW();
}

}  // namespace


BT::Physics_transform BT::Physics_transform::make_phys_trans(rvec3s pos, versors rot)
{
    return { JPH::RVec3(pos.x, pos.y, pos.z), JPH::Quat(rot.x, rot.y, rot.z, rot.w) };
}

unique_ptr<BT::Physics_object> BT::Physics_object::create_triangle_mesh(
    bool interpolate_transform,
    Model const* model,
    JPH::EMotionType motion_type,
    Physics_transform&& init_transform)
{
    auto tri_mesh =
        make_unique<Phys_obj_impl_tri_mesh>(model,
                                            motion_type,
                                            std::move(init_transform));
    return unique_ptr<Physics_object>(
        new Physics_object(interpolate_transform, std::move(tri_mesh)));
}

unique_ptr<BT::Physics_object> BT::Physics_object::create_character_controller(
    bool interpolate_transform,
    float_t radius,
    float_t height,
    float_t crouch_height,
    Physics_transform&& init_transform)
{
    auto cc = make_unique<Phys_obj_impl_char_controller>(radius,
                                                         height,
                                                         crouch_height,
                                                         std::move(init_transform));
    return unique_ptr<Physics_object>(new Physics_object(interpolate_transform, std::move(cc)));
}

BT::Physics_object::Physics_object(bool interpolate_transform,
                                   unique_ptr<Physics_object_type_impl_ifc>&& impl_type)
    : m_interpolate{ interpolate_transform }
    , m_type_pimpl{ std::move(impl_type) }
{
}

void BT::Physics_object::read_and_store_new_transform()
{
    auto phys_transform{ m_type_pimpl->read_transform() };
    m_type_pimpl->update_debug_mesh();

    // Write to triple buffer then increment offset.
    size_t trip_buf_offset{ m_trip_buf_offset.load() };
    m_transform_triple_buffer[(trip_buf_offset + k_trip_buf_write) % 3] = { phys_transform.position,
                                                                            phys_transform.rotation };
    m_trip_buf_offset.store(trip_buf_offset + 1);
}

void BT::Physics_object::get_transform_for_entity(rvec3& out_position, versor& out_rotation)
{
    size_t trip_buf_offset{ m_trip_buf_offset.load() };

    static auto const s_convert_transform_fn =
        [](JPH::RVec3Arg position, JPH::QuatArg rotation, rvec3& out_position, versor& out_rotation) {
            out_position[0] = position.GetX();
            out_position[1] = position.GetY();
            out_position[2] = position.GetZ();

            out_rotation[0] = rotation.GetX();
            out_rotation[1] = rotation.GetY();
            out_rotation[2] = rotation.GetZ();
            out_rotation[3] = rotation.GetW();
        };

    if (m_interpolate)
    {
        // Interpolate A and B transforms.
        float_t lerp_val{ service_finder::find_service<Physics_engine>().get_interpolation_alpha() };  // @TODO: @FIXME: I want this gone since there's not gonna be interpolation between physics and simulation/game thread. They're locked together now.  -Thea 2025/10/31
        auto& trans_a{ m_transform_triple_buffer[(trip_buf_offset + k_trip_buf_read_a) % 3] };
        auto& trans_b{ m_transform_triple_buffer[(trip_buf_offset + k_trip_buf_read_b) % 3] };

        rvec3 pos_a;
        rvec3 pos_b;
        versor rot_a;
        versor rot_b;
        s_convert_transform_fn(trans_a.position, trans_a.rotation, pos_a, rot_a);
        s_convert_transform_fn(trans_b.position, trans_b.rotation, pos_b, rot_b);

        out_position[0] = pos_a[0] + lerp_val * (pos_b[0] - pos_a[0]);
        out_position[1] = pos_a[1] + lerp_val * (pos_b[1] - pos_a[1]);
        out_position[2] = pos_a[2] + lerp_val * (pos_b[2] - pos_a[2]);
        glm_quat_nlerp(rot_a, rot_b, lerp_val, out_rotation);
    }
    else
    {
        // Send only B transform.
        auto& trans_b{ m_transform_triple_buffer[(trip_buf_offset + k_trip_buf_read_b) % 3] };
        s_convert_transform_fn(trans_b.position, trans_b.rotation, out_position, out_rotation);
    }
}
