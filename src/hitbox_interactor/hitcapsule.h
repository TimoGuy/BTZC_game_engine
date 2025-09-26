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
    size_t  calcd_bone_mat_idx{ (size_t)-1 };
    size_t  calcd_bone_mat_idx_2{ (size_t)-1 };  // -1 means same as `calcd_bone_mat_idx`.
    float_t calcd_orig_pts_dist;

    void init_calc_info(Model_animator* animator);
    void calc_orig_pt_distance();
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

private:
    bool m_enabled;
    Type m_type;
    std::vector<Hitcapsule> m_capsules;

    void copy_into_me();
};

class Hitcapsule_group_set : public Scene_serialization_ifc
{
public:
    void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

private:
    std::vector<Hitcapsule_group> m_hitcapsule_grps;
};

class Hitcapsule_group_overlap_solver
{
public:
    Hitcapsule_group_overlap_solver();

    bool add_group_set(Hitcapsule_group_set& group_set);
    void remove_group_set(Hitcapsule_group_set& group_set);

    void update_overlaps();

private:
    std::unordered_set<Hitcapsule_group_set const*> m_group_sets;
};

}  // namespace BT
