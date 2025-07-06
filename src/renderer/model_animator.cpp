#include "model_animator.h"

#include "cglm/affine.h"
#include "cglm/mat4.h"
#include "cglm/quat.h"
#include "logger.h"
#include <cmath>


BT::Model_joint_animation::Model_joint_animation(
    Model_skin const& skin,
    std::string name,
    std::vector<Model_joint_animation_frame>&& animation_frames)
    : m_model_skin{ skin }
    , m_name{ name }
    , m_frames{ std::move(animation_frames) }
{
}

void BT::Model_joint_animation::calc_joint_matrices(float_t time,
                                                    std::vector<mat4s>& out_joint_matrices) const
{
    uint32_t frame_idx_a = std::floor(time / k_frames_per_second);
    uint32_t frame_idx_b = std::ceil(time / k_frames_per_second);

    float_t interp_t{ (time / k_frames_per_second) - frame_idx_a };

    frame_idx_a = (frame_idx_a % m_frames.size());
    frame_idx_b = (frame_idx_b % m_frames.size());

    if (m_interp_type == INTERP_TYPE_STEP ||
        frame_idx_a == frame_idx_b)
    {
        // Just get current frame since no interp.
        get_joint_matrices_at_frame(frame_idx_a, out_joint_matrices);
        return;
    }

    // Allocate calculation cache.
    std::vector<mat4s> joint_global_transform_cache;
    joint_global_transform_cache.resize(m_model_skin.joints_sorted_breadth_first.size());

    out_joint_matrices.resize(m_model_skin.joints_sorted_breadth_first.size());

    // Calculate joint matrices.
    for (size_t i = 0; i < m_model_skin.joints_sorted_breadth_first.size(); i++)
    {
        auto& joint{ m_model_skin.joints_sorted_breadth_first[i] };
        if (i == 0 && joint.parent_idx != (uint32_t)-1)
        {
            logger::printe(logger::ERROR,
                           "First joint parent is not null. Joint list probably not sorted. Aborting.");
            assert(false);
            return;
        }

        // Calculate global transform (relative to root bone).
        auto local_joint_transform{
            m_frames[frame_idx_a].joint_transforms_in_order[i].interpolate_fast(
                m_frames[frame_idx_b].joint_transforms_in_order[i],
                interp_t) };

        mat4 global_joint_transform;
        glm_translate_make(global_joint_transform, local_joint_transform.position);
        glm_quat_rotate(global_joint_transform, local_joint_transform.rotation, global_joint_transform);
        glm_scale(global_joint_transform, local_joint_transform.scale);
        if (joint.parent_idx != (uint32_t)-1)
        {
            // Use cached parent global trans to make global trans.
            glm_mat4_mul(joint_global_transform_cache[joint.parent_idx].raw,
                         global_joint_transform,
                         global_joint_transform);
        }

        // Insert global transform into cache.
        glm_mat4_copy(global_joint_transform, joint_global_transform_cache[i].raw);

        // Calculate joint matrix.
        // @RANT: I hate how all the glm functions don't mark the params as const,
        //   and also since they're not getting mutated! Aaaaggghhhh
        mat4 joint_matrix;
        glm_mat4_mul(const_cast<vec4*>(m_model_skin.inverse_global_transform),
                     global_joint_transform,
                     joint_matrix);
        glm_mat4_mul(joint_matrix,
                     const_cast<vec4*>(joint.inverse_bind_matrix),
                     out_joint_matrices[i].raw);
    }
}

void BT::Model_joint_animation::get_joint_matrices_at_frame(
    uint32_t frame_idx,
    std::vector<mat4s>& out_joint_matrices) const
{
    // @FOR NOW: Just return t pose.
    // @INCOMPLETE: @NOCHECKIN
    out_joint_matrices.clear();
    out_joint_matrices.resize(m_model_skin.joints_sorted_breadth_first.size());
    for (auto& joint_matrix : out_joint_matrices)
    {
        glm_mat4_identity(joint_matrix.raw);
    }
}
