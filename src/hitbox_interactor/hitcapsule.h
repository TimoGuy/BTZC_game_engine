#pragma once

// #include "../scene/scene_serialization_ifc.h"
#include "btglm.h"
#include "btjson.h"

#include <string>
#include <vector>
#include <unordered_set>


// -------------------------------------------------------------------------------------------------
template<
    typename BasicJsonType,
    nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0>
void to_json(BasicJsonType& nlohmann_json_j, const vec3s& nlohmann_json_t)
{
    nlohmann_json_j["x"] = nlohmann_json_t.x;
    nlohmann_json_j["y"] = nlohmann_json_t.y;
    nlohmann_json_j["z"] = nlohmann_json_t.z;
}

template<
    typename BasicJsonType,
    nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0>
void from_json(const BasicJsonType& nlohmann_json_j, vec3s& nlohmann_json_t)
{
    nlohmann_json_j.at("x").get_to(nlohmann_json_t.x);
    nlohmann_json_j.at("y").get_to(nlohmann_json_t.y);
    nlohmann_json_j.at("z").get_to(nlohmann_json_t.z);
}
// -------------------------------------------------------------------------------------------------


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

    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Hitcapsule,
                                   origin_a,
                                   origin_b,
                                   radius,
                                   connecting_bone_name,
                                   connecting_bone_name_2);

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

    // Hitcapsule_group(bool enabled,
    //                  Type type,
    //                  std::vector<Hitcapsule>&& capsules);

    void set_enabled(bool enabled);
    bool is_enabled();
    Type get_type();
    std::vector<Hitcapsule>& get_capsules();

    // Submits debug render representation of hitcapsules, with different colors depending on type
    // of group and whether this group is enabled.
    void emplace_debug_render_repr() const;

private:
    bool m_enabled{ false };
    Type m_type{ 0 };
    std::vector<Hitcapsule> m_capsules;

public:
    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Hitcapsule_group, m_enabled, m_type, m_capsules);
};

class Hitcapsule_group_set // : public Scene_serialization_ifc
{
public:
    ~Hitcapsule_group_set();

    // void scene_serialize(Scene_serialization_mode mode, json& node_ref) override;

    void replace_and_reregister(Hitcapsule_group_set const& other);
    void connect_animator(Model_animator const& animator);

    std::vector<Hitcapsule_group>& get_hitcapsule_groups();

    // Submits debug render representation of hitcapsule groups.
    void emplace_debug_render_repr() const;

private:
    std::vector<Hitcapsule_group> m_hitcapsule_grps;
    bool m_is_connected_to_animator{ false };
    bool m_is_registered_in_overlap_solver{ false };

public:
    /// Serialization/deserialization.
    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Hitcapsule_group_set, m_hitcapsule_grps);
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
