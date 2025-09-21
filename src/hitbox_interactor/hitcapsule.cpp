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


// Hitcapsule_group_overlap_solver.
BT::Hitcapsule_group_overlap_solver::Hitcapsule_group_overlap_solver()
{
    // Add self as service.
    BT_SERVICE_FINDER_ADD_SERVICE(Hitcapsule_group_overlap_solver, this);
}

BT::UUID BT::Hitcapsule_group_overlap_solver::add_group(Hitcapsule_group const& group)
{
    // @TODO
    assert(false);
    return UUID();
}

void BT::Hitcapsule_group_overlap_solver::remove_group(UUID group_id)
{}

void BT::Hitcapsule_group_overlap_solver::update_overlaps()
{}
