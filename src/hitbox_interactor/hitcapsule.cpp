#include "hitcapsule.h"

#include "../renderer/debug_render_job.h"
#include "../renderer/model_animator.h"
#include "../service_finder/service_finder.h"
#include "btglm.h"
#include "btlogger.h"

#include <cassert>


// Hitcapsule.
void BT::Hitcapsule::init_calc_info(Model_animator const& animator)
{
    auto& joint_name_to_idx{ animator.get_model_skin().joint_name_to_idx };

    if (!connecting_bone_name.empty())
        calcd_bone_mat_idx = joint_name_to_idx.at(connecting_bone_name);
    if (!connecting_bone_name_2.empty())
        calcd_bone_mat_idx_2 = joint_name_to_idx.at(connecting_bone_name_2);

    glm_vec3_copy(origin_a.raw, calcd_origin_a);
    glm_vec3_copy(origin_b.raw, calcd_origin_b);

    calc_orig_pt_distance();
}

void BT::Hitcapsule::update_transform(mat4 base_transform, std::vector<mat4s> const& joint_matrices)
{
    static auto s_eval_combined_transforms_fn = [](size_t connecting_bone_mat_idx,
                                                   size_t connecting_bone_mat_idx_2,
                                                   mat4 base_transform,
                                                   std::vector<mat4s> const& joint_matrices,
                                                   mat4& out_combined_trans,
                                                   mat4& out_combined_trans_2) {
        if (connecting_bone_mat_idx == (size_t)-1)
        {
            glm_mat4_copy(base_transform, out_combined_trans);
            glm_mat4_copy(out_combined_trans, out_combined_trans_2);
        }
        else
        {
            glm_mat4_mul(base_transform,
                         const_cast<vec4*>(joint_matrices[connecting_bone_mat_idx].raw),
                         out_combined_trans);

            // Second connecting bone.
            if (connecting_bone_mat_idx_2 == (size_t)-1)
            {
                glm_mat4_copy(out_combined_trans, out_combined_trans_2);
            }
            else
            {
                glm_mat4_mul(base_transform,
                             const_cast<vec4*>(joint_matrices[connecting_bone_mat_idx_2].raw),
                             out_combined_trans_2);
            }
        }
    };

    // Update capsule origins with combined transform(s).
    mat4 combined_trans;
    mat4 combined_trans_2;
    s_eval_combined_transforms_fn(calcd_bone_mat_idx,
                                  calcd_bone_mat_idx_2,
                                  base_transform,
                                  joint_matrices,
                                  combined_trans,
                                  combined_trans_2);

    glm_mat4_mulv3(combined_trans, origin_a.raw, 1.0f, calcd_origin_a);
    glm_mat4_mulv3(combined_trans_2, origin_b.raw, 1.0f, calcd_origin_b);

    // Recalc origin point dependent vars.
    calc_orig_pt_distance();
}

void BT::Hitcapsule::calc_orig_pt_distance()
{
    calcd_orig_pts_dist = glm_vec3_distance(calcd_origin_a, calcd_origin_b);
}

void BT::Hitcapsule::emplace_debug_render_repr(vec4 color) const
{
    get_main_debug_line_pool().emplace_debug_line_based_capsule(
        const_cast<float_t*>(calcd_origin_a),
        const_cast<float_t*>(calcd_origin_b),
        radius,
        color,
#define BT_HITCAPSULE_DEBUG_RENDER_REPRESENTATION_LONG_HOLD 0
#if BT_HITCAPSULE_DEBUG_RENDER_REPRESENTATION_LONG_HOLD
        0.5f);
#else
        0.03f);
#endif
}


// Hitcapsule_group.
void BT::Hitcapsule_group::set_enabled(bool enabled)
{
    m_enabled = enabled;
}

bool BT::Hitcapsule_group::is_enabled() const
{
    return m_enabled;
}

BT::Hitcapsule_group::Type BT::Hitcapsule_group::get_type() const
{
    return m_type;
}

std::vector<BT::Hitcapsule>& BT::Hitcapsule_group::get_capsules()
{
    return m_capsules;
}

void BT::Hitcapsule_group::emplace_debug_render_repr() const
{   // Generate color.
    vec4s color;
    switch (m_type)
    {
        case HITBOX_TYPE_RECEIVE_HURT:
            color = (m_enabled
                     ? vec4s{ 0.509, 0.465, 0.990 }
                     : vec4s{ 0.201, 0.183, 0.390 });
            break;

        case HITBOX_TYPE_GIVE_HURT:
            color = (m_enabled
                     ? vec4s{ 0.950, 0.427, 0.00 }
                     : vec4s{ 0.530, 0.288, 0.0901 });
            break;
    }

    // "Draw" hitcapsules.
    for (auto& hitcapsule : m_capsules)
    {
        hitcapsule.emplace_debug_render_repr(color.raw);
    }
}


// Hitcapsule_group_set.
BT::Hitcapsule_group_set::~Hitcapsule_group_set()
{
    unregister_from_overlap_solver();
}

void BT::Hitcapsule_group_set::replace_and_reregister(Hitcapsule_group_set const& other)
{
    auto& overlap_solver{ service_finder::find_service<Hitcapsule_group_overlap_solver>() };

    if (m_is_registered_in_overlap_solver)
    {   // Unregister self if currently registered.  @COPYPASTA with `unregister_from_overlap_solver()`
        m_is_registered_in_overlap_solver =
            !overlap_solver.remove_group_set(*this);
        assert(!m_is_registered_in_overlap_solver);
    }

    *this = other;

    m_is_registered_in_overlap_solver = overlap_solver.add_group_set(*this);
    assert(m_is_registered_in_overlap_solver);
}

void BT::Hitcapsule_group_set::unregister_from_overlap_solver()
{
    if (m_is_registered_in_overlap_solver)
    {
        // Unregister to cleanup.
        m_is_registered_in_overlap_solver =
            !service_finder::find_service<Hitcapsule_group_overlap_solver>().remove_group_set(*this);
    }
}

void BT::Hitcapsule_group_set::connect_animator(Model_animator const& animator)
{
    for (auto& group : m_hitcapsule_grps)
    for (auto& capsule : group.get_capsules())
    {
        capsule.init_calc_info(animator);
    }

    m_is_connected_to_animator = true;
}

std::vector<BT::Hitcapsule_group>& BT::Hitcapsule_group_set::get_hitcapsule_groups()
{
    return m_hitcapsule_grps;
}

void BT::Hitcapsule_group_set::emplace_debug_render_repr() const
{
    for (auto& group : m_hitcapsule_grps)
    {
        group.emplace_debug_render_repr();
    }
}

std::vector<BT::Hitcapsule const*> BT::Hitcapsule_group_set::fetch_all_enabled_hitcapsules(
    Hitcapsule_group::Type hitcapsule_type) const
{
    std::vector<Hitcapsule const*> result_hitcapsules;

    for (auto& group : m_hitcapsule_grps)
        if (group.is_enabled() && group.get_type() == hitcapsule_type)
        {
            auto const& hitcapsules{ const_cast<Hitcapsule_group&>(group).get_capsules() };  // @HACK.

            for (auto& hitcapsule : hitcapsules)
                result_hitcapsules.emplace_back(&hitcapsule);
        }

    result_hitcapsules.shrink_to_fit();
    return result_hitcapsules;    
}


// Hitcapsule_group_overlap_solver.
BT::Hitcapsule_group_overlap_solver::Hitcapsule_group_overlap_solver()
{
    // Add self as service.
    BT_SERVICE_FINDER_ADD_SERVICE(Hitcapsule_group_overlap_solver, this);
}

namespace
{
constexpr char const* const k_add_log_str =
    "(+) Added hitcapsule group set to overlap solver:     ";
constexpr char const* const k_remove_log_str =
    "(-) Removed hitcapsule group set from overlap solver: ";
}  // namespace

bool BT::Hitcapsule_group_overlap_solver::add_group_set(Hitcapsule_group_set& group_set)
{
    bool success{ false };
    if (m_group_sets.emplace(&group_set).second)
    {
        logger::printef(logger::TRACE,
                        "%s%p",
                        k_add_log_str,
                        (void*)&group_set);
        success = true;
    }
    else
        logger::printe(logger::ERROR, "Emplacing hitcapsule group set failed.");

    assert(success);
    return success;
}

bool BT::Hitcapsule_group_overlap_solver::remove_group_set(Hitcapsule_group_set& group_set)
{
    bool success{ false };
    auto iter{ m_group_sets.find(&group_set) };
    if (iter != m_group_sets.end())
    {
        m_group_sets.erase(iter);
        logger::printef(logger::TRACE,
                        "%s%p",
                        k_remove_log_str,
                        (void*)&group_set);
        success = true;
    }

    return success;
}

size_t BT::Hitcapsule_group_overlap_solver::get_num_group_sets() const
{
    return m_group_sets.size();
}

void BT::Hitcapsule_group_overlap_solver::update_overlaps()
{
    for (auto grp_set_ptr_a : m_group_sets)
    for (auto grp_set_ptr_b : m_group_sets)
    if (grp_set_ptr_a != grp_set_ptr_b)
    {   // Collect capsules for check.
        auto give_hurt_caps_a{ grp_set_ptr_a->fetch_all_enabled_hitcapsules(
            Hitcapsule_group::HITBOX_TYPE_GIVE_HURT) };
        auto rece_hurt_caps_b{ grp_set_ptr_b->fetch_all_enabled_hitcapsules(
            Hitcapsule_group::HITBOX_TYPE_RECEIVE_HURT) };

        // Broad phase check for overlaps (A hurts B).
        // @NOTE: Only A hurts B is necessary, since the n^2 loops. The reverse case will eventually
        //        be covered with the loops.
        bool found_overlap{ false };
        {
            for (size_t a_i = 0; a_i < give_hurt_caps_a.size() && !found_overlap; a_i++)
            for (size_t b_i = 0; b_i < rece_hurt_caps_b.size() && !found_overlap; b_i++)
            {
                auto& ghc{ *give_hurt_caps_a[a_i] };
                auto& rhc{ *rece_hurt_caps_b[b_i] };

                // @NOTE: `ghc` is "give hurt capsule", and `rhc` is "receive hurt capsule".
                vec3 ghc_mdpt_origin = GLM_VEC3_ZERO_INIT;
                glm_vec3_muladds(const_cast<float_t*>(ghc.calcd_origin_a), 0.5f, ghc_mdpt_origin);
                glm_vec3_muladds(const_cast<float_t*>(ghc.calcd_origin_b), 0.5f, ghc_mdpt_origin);
                float_t ghc_bp_sphere_rad{ ghc.calcd_orig_pts_dist * 0.5f + ghc.radius };

                vec3 rhc_mdpt_origin = GLM_VEC3_ZERO_INIT;
                glm_vec3_muladds(const_cast<float_t*>(rhc.calcd_origin_a), 0.5f, rhc_mdpt_origin);
                glm_vec3_muladds(const_cast<float_t*>(rhc.calcd_origin_b), 0.5f, rhc_mdpt_origin);
                float_t rhc_bp_sphere_rad{ rhc.calcd_orig_pts_dist * 0.5f + rhc.radius };

                // Check for overlap.
                float_t combined_rads{ ghc_bp_sphere_rad + rhc_bp_sphere_rad };
                if (glm_vec3_distance2(ghc_mdpt_origin, rhc_mdpt_origin) <
                    combined_rads * combined_rads)
                {   // Found overlap! No more need to look.
                    found_overlap = true;
                    break;
                }
            }
        }

        if (found_overlap)
        {   // Continue to narrow phase check!
            found_overlap = false;
            assert(false);  // @TODO: Implement!
        }

        if (found_overlap)
        {   // Report that A hurt B!
            assert(false);  // @TODO: Implement!
        }
    }

    // Submit debug drawing for all hitcapsules.
    for (auto group_set_ptr : m_group_sets)
    {
        group_set_ptr->emplace_debug_render_repr();
    }
}
