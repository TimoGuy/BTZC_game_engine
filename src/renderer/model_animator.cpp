#include "model_animator.h"

#include "cglm/affine.h"
#include "cglm/mat4.h"
#include "cglm/quat.h"
#include "cglm/vec3.h"
#include "logger.h"
#include "mesh.h"
#include <algorithm>
#include <cmath>


BT::Model_joint_animation_frame::Joint_local_transform
BT::Model_joint_animation_frame::Joint_local_transform::interpolate_fast(
    Joint_local_transform const& other,
    float_t t) const
{
    Joint_local_transform ret_trans;
#if 0  /* I removed the `STEP` interpolation type since I figured that at least for skeletal animations I'm not gonna include it  -Thea 2025/07/13 */
    switch (interp_type)
    {
        case INTERP_TYPE_LINEAR:
#endif  // 0
            glm_vec3_lerp(const_cast<float_t*>(position),
                          const_cast<float_t*>(other.position),
                          t,
                          ret_trans.position);
            glm_quat_nlerp(const_cast<float_t*>(rotation),
                           const_cast<float_t*>(other.rotation),
                           t,
                           ret_trans.rotation);
            glm_vec3_lerp(const_cast<float_t*>(scale),
                          const_cast<float_t*>(other.scale),
                          t,
                          ret_trans.scale);
#if 0
            ret_trans.interp_type = INTERP_TYPE_LINEAR;
            break;

        case INTERP_TYPE_STEP:
            glm_vec3_copy(const_cast<float_t*>(position),
                          ret_trans.position);
            glm_quat_copy(const_cast<float_t*>(rotation),
                          ret_trans.rotation);
            glm_vec3_copy(const_cast<float_t*>(scale),
                          ret_trans.scale);
            ret_trans.interp_type = INTERP_TYPE_STEP;
            break;

        default:
            assert(false);
            break;
    }
#endif  // 0
    return ret_trans;
}


BT::Model_joint_animation::Model_joint_animation(
    Model_skin const& skin,
    std::string name,
    std::vector<Model_joint_animation_frame>&& animation_frames)
    : m_model_skin{ skin }
    , m_name{ name }
    , m_frames{ std::move(animation_frames) }
{
}

uint32_t BT::Model_joint_animation::calc_frame_idx(float_t time,
                                                   bool loop,
                                                   Rounding_func rounding) const
{
    assert(time >= 0.0f);
    assert(m_frames.size() >= 1);

    uint32_t frame_idx;
    switch (rounding)
    {
        case FLOOR:
            frame_idx = std::floor(time * k_frames_per_second);
            break;

        case CEIL:
            frame_idx = std::ceil(time * k_frames_per_second);
            break;

        default:
            // Incorrect value entered.
            assert(false);
            break;
    }

    if (loop)
    {
        // Loop keyframes.
        frame_idx = (frame_idx % m_frames.size());
    }
    else
    {
        // Clamp keyframes to base frame.
        frame_idx = std::min(frame_idx, static_cast<uint32_t>(m_frames.size()) - 1);
    }

    return frame_idx;
}

void BT::Model_joint_animation::calc_joint_matrices(float_t time,
                                                    bool loop,
                                                    std::vector<mat4s>& out_joint_matrices) const
{
    uint32_t frame_idx_a{ calc_frame_idx(time, loop, FLOOR) };
    uint32_t frame_idx_b{ calc_frame_idx(time, loop, CEIL) };

    float_t interp_t{ (time / k_frames_per_second)
                      - std::floor(time / k_frames_per_second) };

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

        // Calculate global transform (relative to parent bone -> model space).
        auto local_joint_transform{
            m_frames[frame_idx_a].joint_transforms_in_order[i].interpolate_fast(
                m_frames[frame_idx_b].joint_transforms_in_order[i],
                interp_t) };

        mat4 global_joint_transform;
        glm_translate_make(global_joint_transform, local_joint_transform.position);
        glm_quat_rotate(global_joint_transform, local_joint_transform.rotation, global_joint_transform);
        glm_scale(global_joint_transform, local_joint_transform.scale);

        if (joint.parent_idx == (uint32_t)-1)
        {   // Use skin baseline transform.
            glm_mat4_mul(const_cast<vec4*>(m_model_skin.baseline_transform),
                 global_joint_transform,
                 global_joint_transform);
        }
        else
        {   // Use cached parent global trans to make global trans.
            glm_mat4_mul(joint_global_transform_cache[joint.parent_idx].raw,
                         global_joint_transform,
                         global_joint_transform);
        }

        // Insert global transform into cache.
        glm_mat4_copy(global_joint_transform, joint_global_transform_cache[i].raw);

        // Calculate joint matrix.
        // @RANT: I hate how all the glm functions don't mark the params as const,
        //   and also since they're not getting mutated! Aaaaggghhhh
        // @RANT: I hate how the rant above was a rant!!! The amount of strenuous
        //   work to get this whole shitshow working was insane!!!! Hahahahahahaha  -Thea 2025/07/20
        mat4 joint_matrix;
        glm_mat4_mul(const_cast<vec4*>(m_model_skin.inverse_global_transform),
                     global_joint_transform,
                     joint_matrix);
        glm_mat4_mul(joint_matrix,
                     const_cast<vec4*>(joint.inverse_bind_matrix),
                     out_joint_matrices[i].raw);
    }

    // @DEBUG.
    static float_t s_target_matrix_as_flt{ 0.0f };
    size_t keep_idx{ static_cast<size_t>(std::floorf(s_target_matrix_as_flt)) % out_joint_matrices.size() };
    static size_t prev_keep_idx{ (size_t)-1 };
    for (size_t i = 0; i < out_joint_matrices.size(); i++)
    {
        // // if (m_model_skin.joints_sorted_breadth_first[i].name == "Neck")
        // // if (i == keep_idx)
        // if (i == 78)
        //     glm_translate_make(out_joint_matrices[i].raw, vec3{ 0.0f, 0.0f, 10.0f });
        // else
        //     glm_mat4_identity(out_joint_matrices[i].raw);
    }
    s_target_matrix_as_flt += 0.01f;
    if (keep_idx != prev_keep_idx)
    {
        logger::printef(logger::TRACE, "New keep idx: %d", keep_idx);
        prev_keep_idx = keep_idx;
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

    assert(false);
}


BT::Model_animator::Model_animator(Model const& model)
    : m_model_animations{ model.m_animations }
{
}

void BT::Model_animator::configure_animator(std::vector<Animator_state>&& animator_states)
{
    // Idk why I put this into a separate method instead of in the constructor but hey, here we are.
    m_animator_states = std::move(animator_states);
}

void BT::Model_animator::change_state_idx(uint32_t to_state)
{
    uint32_t from_state_copy{ m_current_state_idx.load() };
    if (m_current_state_idx.compare_exchange_strong(from_state_copy,
                                                    to_state))
    {
        m_time = 0.0f;
    }
}

void BT::Model_animator::update(float_t delta_time)
{
    m_time += delta_time * m_animator_states[m_current_state_idx].speed;
}

void BT::Model_animator::calc_anim_pose(std::vector<mat4s>& out_joint_matrices) const
{
    auto& anim_state{ m_animator_states[m_current_state_idx] };
    m_model_animations[anim_state.animation_idx]
        .calc_joint_matrices(m_time, anim_state.loop, out_joint_matrices);
}

void BT::Model_animator::get_anim_floored_frame_pose(std::vector<mat4s>& out_joint_matrices) const
{
    auto& anim_state{ m_animator_states[m_current_state_idx] };
    uint32_t frame_idx{
        m_model_animations[anim_state.animation_idx]
            .calc_frame_idx(m_time,
                            anim_state.loop,
                            Model_joint_animation::FLOOR) };
    m_model_animations[anim_state.animation_idx]
        .get_joint_matrices_at_frame(frame_idx, out_joint_matrices);
}
