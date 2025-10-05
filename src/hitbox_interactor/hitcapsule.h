#pragma once

#include "../scene/scene_serialization_ifc.h"
#include "cglm/types-struct.h"

#include <string>
#include <vector>
#include <unordered_set>


namespace BT
{

class Model_animator;

struct Hitcapsule
{
    vec3s   origin_a;  // Origins of both ends of the spheres making up the capsule.
    vec3s   origin_b;
    float_t radius;

    std::string connecting_bone_name{ "" };  // Leave empty str for no bone connection.
    std::string connecting_bone_name_2{ "" };  // Leave empty str for same as `connecting_bone_name`.

    // Calculated info.
    vec3    calcd_origin_a;       // Set to `origin_a` on init. Updated to connect to joint mat in
                                  // `update_transform`.
    vec3    calcd_origin_b;       // Similar to `calcd_origin_a`.
    float_t calcd_orig_pts_dist;  // Calculated from `calcd_origin_a/b`.

    size_t  calcd_bone_mat_idx{ (size_t)-1 };
    size_t  calcd_bone_mat_idx_2{ (size_t)-1 };  // -1 means same as `calcd_bone_mat_idx`.

    void init_calc_info(Model_animator const& animator);
    void update_transform(mat4 base_transform, std::vector<mat4s> const& joint_matrices);
    void calc_orig_pt_distance();

    // Submits debug render representation of this capsule, with provided `color`.
    void emplace_debug_render_repr(vec4 color) const;
};

class Hitcapsule_group
{
public:
    enum Type : uint8_t
    {
        HITBOX_TYPE_RECEIVE_HURT,
        HITBOX_TYPE_GIVE_HURT,
    };

    Hitcapsule_group(bool enabled,
                     Type type,
                     std::vector<Hitcapsule>&& capsules);

    void set_enabled(bool enabled);
    bool is_enabled();
    Type get_type();
    std::vector<Hitcapsule>& get_capsules();

    // Submits debug render representation of hitcapsules, with different colors depending on type
    // of group and whether this group is enabled.
    void emplace_debug_render_repr() const;

private:
    bool m_enabled;
    Type m_type;
    std::vector<Hitcapsule> m_capsules;

    void copy_into_me();
};

class Hitcapsule_group_set : public Scene_serialization_ifc
{
public:
    ~Hitcapsule_group_set();

    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

    void replace_and_reregister(Hitcapsule_group_set const& other);
    void connect_animator(Model_animator const& animator);

    std::vector<Hitcapsule_group>& get_hitcapsule_groups();

    // Submits debug render representation of hitcapsule groups.
    void emplace_debug_render_repr() const;

private:
    std::vector<Hitcapsule_group> m_hitcapsule_grps;
    bool m_is_registered_in_overlap_solver{ false };
};

class Hitcapsule_group_overlap_solver
{
public:
    Hitcapsule_group_overlap_solver();

    bool add_group_set(Hitcapsule_group_set& group_set);
    bool remove_group_set(Hitcapsule_group_set& group_set);

    void update_overlaps();

private:
    std::unordered_set<Hitcapsule_group_set const*> m_group_sets;
};

}  // namespace BT
