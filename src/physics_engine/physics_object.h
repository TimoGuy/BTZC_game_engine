#pragma once

#include "../scene/scene_serialization_ifc.h"
#include "../uuid/uuid_ifc.h"
#include "cglm/cglm.h"
#include "Jolt/Jolt.h"
#include "Jolt/Math/MathTypes.h"
#include "Jolt/Math/Quat.h"
#include "Jolt/Physics/Body/MotionType.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "rvec3.h"
#include <atomic>
#include <cassert>
#include <memory>
#include <utility>
#include <vector>

using std::atomic_size_t;
using std::pair;
using std::unique_ptr;
using std::vector;


namespace BT
{

class Game_object;

enum Physics_object_type
{
    PHYSICS_OBJECT_TYPE_TRIANGLE_MESH = 0,
    PHYSICS_OBJECT_TYPE_CHARACTER_CONTROLLER,
    NUM_PHYSICS_OBJECT_TYPES
};

inline static const vector<pair<string, BT::Physics_object_type>> k_phys_obj_str_type_pairs{
    { "triangle_mesh", PHYSICS_OBJECT_TYPE_TRIANGLE_MESH },
    { "character_controller", PHYSICS_OBJECT_TYPE_CHARACTER_CONTROLLER },
};

struct Physics_transform
{
    JPH::RVec3 position = JPH::RVec3::sZero();
    JPH::Quat rotation = JPH::Quat::sIdentity();
};

class Physics_object_type_impl_ifc : public Scene_serialization_ifc
{
public:
    virtual ~Physics_object_type_impl_ifc() = default;
    virtual Physics_object_type get_type() = 0;
    virtual void move_kinematic(Physics_transform&& new_transform) { assert(false); }
    virtual void tick_fetch_cc_status(JPH::Vec3& out_ground_velocity,
                                      JPH::Vec3& out_linear_velocity,
                                      JPH::Vec3& out_up_direction,
                                      JPH::Quat& out_up_rotation,
                                      bool& out_is_supported,
                                      JPH::CharacterVirtual::EGroundState& out_ground_state,
                                      JPH::Vec3& out_ground_normal,
                                      bool& out_is_crouched) { assert(false); }
    virtual bool is_cc_slope_too_steep(JPH::Vec3 normal) { assert(false); return false; }
    virtual void set_cc_allow_sliding(bool allow) { assert(false); }
    virtual void set_cc_velocity(JPH::Vec3Arg velocity) { assert(false); }
    virtual bool set_cc_stance(bool is_crouching) { assert(false); return false; }
    virtual bool get_cc_stance() { assert(false); return false; }
    virtual bool has_cc_wall_contact() { assert(false); return false; }
    virtual float_t get_cc_radius() { assert(false); return 0.0f; }
    virtual float_t get_cc_height() { assert(false); return 0.0f; }
    virtual void on_pre_update(float_t physics_delta_time) { }
    virtual Physics_transform read_transform() = 0;
    virtual void update_debug_mesh() = 0;
};

class Physics_engine;
class Model;

class Physics_object : public Scene_serialization_ifc, public UUID_ifc
{
public:
    static unique_ptr<Physics_object> create_physics_object_from_serialization(
        Game_object& game_obj,
        Physics_engine& phys_engine,
        json& node_ref);
    static unique_ptr<Physics_object> create_triangle_mesh(Game_object& game_obj,
                                                           Physics_engine& phys_engine,
                                                           bool interpolate_transform,
                                                           Model const* model,
                                                           JPH::EMotionType motion_type,
                                                           Physics_transform&& init_transform);
    static unique_ptr<Physics_object> create_character_controller(Game_object& game_obj,
                                                                  Physics_engine& phys_engine,
                                                                  bool interpolate_transform,
                                                                  float_t radius,
                                                                  float_t height,
                                                                  float_t crouch_height,
                                                                  Physics_transform&& init_transform);

private:
    // Required to use a factory function to init.
    Physics_object(Game_object& game_obj,
                   Physics_engine const* phys_engine,
                   bool interpolate_transform,
                   unique_ptr<Physics_object_type_impl_ifc>&& impl_type);
public:
    Physics_object(Physics_object const&)            = delete;
    Physics_object(Physics_object&&)                 = delete;
    Physics_object& operator=(Physics_object const&) = delete;
    Physics_object& operator=(Physics_object&&)      = delete;

    Physics_object_type_impl_ifc* get_impl() { return m_type_pimpl.get(); }
    void read_and_store_new_transform();

    void get_transform_for_game_obj(rvec3& out_position, versor& out_rotation);

    // Scene_serialization_ifc.
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

private:
    Game_object& m_game_obj;
    Physics_engine const* m_phys_engine;
    bool m_interpolate;

    unique_ptr<Physics_object_type_impl_ifc> m_type_pimpl;

    Physics_transform m_transform_triple_buffer[3];

    static constexpr size_t k_trip_buf_read_a{ 0 };
    static constexpr size_t k_trip_buf_read_b{ 1 };
    static constexpr size_t k_trip_buf_write{ 2 };
    atomic_size_t m_trip_buf_offset{ 0 };
};

}  // namespace BT
