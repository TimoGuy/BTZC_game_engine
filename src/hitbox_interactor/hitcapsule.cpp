#include "hitcapsule.h"

#include "../renderer/model_animator.h"
#include "../service_finder/service_finder.h"
#include "cglm/vec3.h"

#include <cassert>


// Hitcapsule.
void BT::Hitcapsule::init_calc_info(Model_animator* animator)
{
    assert(animator != nullptr);
    auto& joint_name_to_idx{ animator->get_model_skin().joint_name_to_idx };

    if (!connecting_bone_name.empty())
        calcd_bone_mat_idx = joint_name_to_idx.at(connecting_bone_name);
    if (!connecting_bone_name_2.empty())
        calcd_bone_mat_idx_2 = joint_name_to_idx.at(connecting_bone_name_2);

    calc_orig_pt_distance();
}

void BT::Hitcapsule::calc_orig_pt_distance()
{
    calcd_orig_pts_dist = glm_vec3_distance(origin_a.raw, origin_b.raw);
}


// Hitcapsule_group.
BT::Hitcapsule_group::Hitcapsule_group(bool enabled,
                    Type type,
                    std::vector<Hitcapsule>&& capsules)
    : m_enabled{ enabled }
    , m_type{ type }
    , m_capsules{ std::move(capsules) }
{
}

void BT::Hitcapsule_group::set_enabled(bool enabled)
{
    m_enabled = enabled;
}

bool BT::Hitcapsule_group::is_enabled()
{
    return m_enabled;
}

BT::Hitcapsule_group::Type BT::Hitcapsule_group::get_type()
{
    return m_type;
}

std::vector<BT::Hitcapsule>& BT::Hitcapsule_group::get_capsules()
{
    return m_capsules;
}


// Hitcapsule_group_set.
void BT::Hitcapsule_group_set::scene_serialize(Scene_serialization_mode mode, json& node_ref)
{
    if (mode == SCENE_SERIAL_MODE_DESERIALIZE)
    {   // Access list of hitcapsule groups.
        m_hitcapsule_grps.reserve(node_ref["hitcapsule_groups"].size());
        for (auto& capsule_group_json : node_ref["hitcapsule_groups"])
        {   // Deserialize capsules.
            std::vector<Hitcapsule> capsules;
            capsules.reserve(capsule_group_json["capsules"].size());
            for (auto& capsule_json : capsule_group_json["capsules"])
            {
                capsules.emplace_back(vec3s{ capsule_json["origin_a"][0].get<float_t>(),
                                             capsule_json["origin_a"][1].get<float_t>(),
                                             capsule_json["origin_a"][2].get<float_t>() },
                                      vec3s{ capsule_json["origin_b"][0].get<float_t>(),
                                             capsule_json["origin_b"][1].get<float_t>(),
                                             capsule_json["origin_b"][2].get<float_t>() },
                                      capsule_json["radius"].get<float_t>(),
                                      capsule_json["connecting_bone_name"].get<std::string>(),
                                      capsule_json["connecting_bone_name_2"].get<std::string>());
            }

            m_hitcapsule_grps.emplace_back(capsule_group_json["enabled"].get<bool>(),
                                           capsule_group_json["type"].get<Hitcapsule_group::Type>(),
                                           std::move(capsules));
        }
    }
    else if (mode == SCENE_SERIAL_MODE_SERIALIZE)
    {   // Access list of hitcapsule groups.
        for (auto& capsule_group : m_hitcapsule_grps)
        {   // Serialize capsules.
            json capsule_group_json = {};
            for (auto& capsule : capsule_group.get_capsules())
            {
                json capsule_json = {};
                capsule_json["origin_a"][0]            = capsule.origin_a.x;
                capsule_json["origin_a"][1]            = capsule.origin_a.y;
                capsule_json["origin_a"][2]            = capsule.origin_a.z;
                capsule_json["origin_b"][0]            = capsule.origin_b.x;
                capsule_json["origin_b"][1]            = capsule.origin_b.y;
                capsule_json["origin_b"][2]            = capsule.origin_b.z;
                capsule_json["radius"]                 = capsule.radius;
                capsule_json["connecting_bone_name"]   = capsule.connecting_bone_name;
                capsule_json["connecting_bone_name_2"] = capsule.connecting_bone_name_2;

                capsule_group_json["capsules"].push_back(capsule_json);
            }

            capsule_group_json["enabled"] = capsule_group.is_enabled();
            capsule_group_json["type"] = capsule_group.get_type();

            node_ref["hitcapsule_groups"].push_back(capsule_group_json);
        }
    }
}

void BT::Hitcapsule_group_set::deep_clone(Hitcapsule_group_set& dest) const
{   // @TODO: IMPLEMENT.
    assert(false);
}


// Hitcapsule_group_overlap_solver.
BT::Hitcapsule_group_overlap_solver::Hitcapsule_group_overlap_solver()
{
    // Add self as service.
    BT_SERVICE_FINDER_ADD_SERVICE(Hitcapsule_group_overlap_solver, this);
}

BT::UUID BT::Hitcapsule_group_overlap_solver::add_group_set(Hitcapsule_group& group)
{
    // @TODO
    assert(false);
    return UUID();
}

void BT::Hitcapsule_group_overlap_solver::remove_group_set(Hitcapsule_group& group)
{
    // @TODO
    assert(false);

}

void BT::Hitcapsule_group_overlap_solver::update_overlaps()
{
    // @TODO
    assert(false);
}
