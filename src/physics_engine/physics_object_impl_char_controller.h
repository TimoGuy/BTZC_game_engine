#pragma once

#include "Jolt/Jolt.h"
#include "Jolt/Core/TempAllocator.h"
#include "Jolt/Math/Real.h"
#include "Jolt/Physics/Character/CharacterVirtual.h"
#include "Jolt/Physics/PhysicsSystem.h"
#include "physics_engine.h"
#include "physics_object.h"
#include <vector>

using std::vector;


namespace BT
{

class Phys_obj_impl_char_controller : public Physics_object_type_impl_ifc, public JPH::CharacterContactListener
{
public:
    Phys_obj_impl_char_controller(Physics_engine& phys_engine,
                                  float_t radius,
                                  float_t height,
                                  float_t crouch_height,
                                  Physics_transform&& init_transform);
    ~Phys_obj_impl_char_controller();

    // Phys obj impl ifc.
    Physics_object_type get_type() override { return PHYSICS_OBJECT_TYPE_CHARACTER_CONTROLLER; }
    void move_kinematic(Physics_transform&& new_transform) override;
    void tick_fetch_cc_status(JPH::Vec3& out_ground_velocity,
                              JPH::Vec3& out_linear_velocity,
                              JPH::Vec3& out_up_direction,
                              JPH::Quat& out_up_rotation,
                              bool& out_is_supported,
                              JPH::CharacterVirtual::EGroundState& out_ground_state,
                              JPH::Vec3& out_ground_normal,
                              bool& out_is_crouched) override;
    bool is_cc_slope_too_steep(JPH::Vec3 normal) override;
    void set_cc_allow_sliding(bool allow) override;
    void set_cc_velocity(JPH::Vec3Arg velocity) override;
    bool set_cc_stance(bool is_crouching) override;
    void on_pre_update(float_t physics_delta_time) override;
    Physics_transform read_transform() override;

    // Character contact listener.
    void OnAdjustBodyVelocity(JPH::CharacterVirtual const* inCharacter,
                              JPH::Body const& in_body2,
                              JPH::Vec3& io_linear_velocity,
                              JPH::Vec3& io_angular_velocity) override;
    void OnContactAdded(JPH::CharacterVirtual const* in_character,
                        JPH::BodyID const& in_body_id2,
                        JPH::SubShapeID const& in_sub_shape_id2,
                        JPH::RVec3Arg in_contact_position,
                        JPH::Vec3Arg in_contact_normal,
                        JPH::CharacterContactSettings& io_settings) override;
    void OnCharacterContactAdded(JPH::CharacterVirtual const* in_character,
                                 JPH::CharacterVirtual const* in_other_character,
                                 JPH::SubShapeID const& in_sub_shape_id2,
                                 JPH::RVec3Arg in_contact_position,
                                 JPH::Vec3Arg in_contact_normal,
                                 JPH::CharacterContactSettings& io_settings) override;
    void OnContactSolve(JPH::CharacterVirtual const* in_character,
                        JPH::BodyID const& in_body_id2,
                        JPH::SubShapeID const& in_sub_shape_id2,
                        JPH::RVec3Arg in_contact_position,
                        JPH::Vec3Arg in_contact_normal,
                        JPH::Vec3Arg in_contact_velocity,
                        JPH::PhysicsMaterial const* in_contact_material,
                        JPH::Vec3Arg in_character_velocity,
                        JPH::Vec3& io_new_character_velocity) override;


private:
    Physics_engine& m_phys_engine;
    JPH::PhysicsSystem& m_phys_system;
    JPH::TempAllocator& m_phys_temp_allocator;
    float_t m_radius;
    float_t m_height;
    float_t m_crouch_height;

    // Config values.
    // @NOTE: I set the back face mode to ignore because there were times when
    //   rounding over a 90 degree angle will cause a contact on the back face edge
    //   of the 90 degree wall. This causes the solver to not know what to do, and
    //   ignoring back faces solves this issue.  -Thea 2025/05/29
    static inline JPH::EBackFaceMode s_back_face_mode                   = JPH::EBackFaceMode::IgnoreBackFaces;
    static inline float_t            s_up_rotation_x                    = 0;
    static inline float_t            s_up_rotation_z                    = 0;
    static inline float_t            s_max_slope_angle                  = JPH::DegreesToRadians(46.0f);
    static inline float_t            s_max_strength                     = 100.0f;
    static inline float_t            s_character_padding                = 0.02f;
    static inline float_t            s_penetration_recovery_speed       = 1.0f;
    static inline float_t            s_predictive_contact_distance      = 0.1f;
    static inline bool               s_enable_walk_stairs               = true;
    static inline bool               s_enable_stick_to_floor            = true;
    static inline bool               s_enhanced_internal_edge_removal   = false;
    static inline bool               s_create_inner_body                = false;
    static inline bool               s_player_can_push_other_characters = true;
    static inline bool               s_other_characters_can_push_player = true;

    JPH::RefConst<JPH::Shape>        m_standing_shape;
    JPH::RefConst<JPH::Shape>        m_crouching_shape;

    JPH::Ref<JPH::CharacterVirtual>  m_character;
    bool m_is_crouched;
    bool m_allow_sliding{ false };  // True when want to move.
};

}  // namespace BT
