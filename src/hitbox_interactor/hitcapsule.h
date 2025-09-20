#pragma once

#include "../uuid/uuid.h"
#include "cglm/cglm.h"
#include "cglm/types-struct.h"

#include <string>
#include <vector>
#include <unordered_map>


namespace BT
{

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
    std::vector<Hitcapsule> const& get_capsules();

private:
    bool m_enabled;
    Type const m_type;
    std::vector<Hitcapsule> m_capsules;
};

class Hitcapsule_group_overlap_solver
{
public:
    Hitcapsule_group_overlap_solver();

    UUID add_group(Hitcapsule_group const& group);
    void remove_group(UUID group_id);
    void update_overlaps();

private:
    std::unordered_map<UUID, Hitcapsule_group const*> m_groups;
};

}  // namespace BT
