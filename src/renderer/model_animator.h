#pragma once

#include "cglm/mat4.h"
#include "cglm/types-struct.h"
#include "cglm/types.h"
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>


namespace BT
{

struct Model_joint;

struct Model_skin
{
    mat4 baseline_transform = GLM_MAT4_IDENTITY_INIT;
    mat4 inverse_global_transform = GLM_MAT4_IDENTITY_INIT;
    std::unordered_map<std::string, uint32_t> joint_name_to_idx;
    std::vector<Model_joint> joints_sorted_breadth_first;
};

struct Model_joint
{
    std::string name;
    mat4 inverse_bind_matrix;
    uint32_t parent_idx{ (uint32_t)-1 };  // @NOTE: Idx instead of pointer for cache lookup.  -Thea 2025/07/10
    std::vector<Model_joint*> children;
};

struct Model_joint_animation_frame
{
    struct Joint_local_transform
    {
        vec3 position;
        versor rotation;
        vec3 scale;

#if 0  /* @NOTE: I really don't think I can handle step interpolation. It's too 細かい from gltf to use here. */
        // (FOR NOW OR MAYBE FOREVER) interpolation type is ignored and only linear is used
        // at least for skeletal animation.
        enum Interpolation_type
        {
            INTERP_TYPE_LINEAR = 0,
            INTERP_TYPE_STEP,
            NUM_INTERP_TYPES
        } interp_type;
#endif  // 0

        Joint_local_transform interpolate_fast(Joint_local_transform const& other,
                                               float_t t) const;
    };
    std::vector<Joint_local_transform> joint_transforms_in_order;
};

class Model_joint_animation
{
public:
    Model_joint_animation(Model_skin const& skin,
                          std::string name,
                          std::vector<Model_joint_animation_frame>&& animation_frames);

    enum Rounding_func{ FLOOR, CEIL };
    uint32_t calc_frame_idx(float_t time, bool loop, Rounding_func rounding) const;
    void calc_joint_matrices(float_t time, bool loop, std::vector<mat4s>& out_joint_matrices) const;
    void get_joint_matrices_at_frame(uint32_t frame_idx, std::vector<mat4s>& out_joint_matrices) const;

    static constexpr float_t k_frames_per_second{ 60.0f };

private:
    Model_skin const& m_model_skin;

    std::string m_name;
    std::vector<Model_joint_animation_frame> m_frames;
};

class Model;

class Model_animator
{
public:
    Model_animator(Model const& model);

    size_t get_num_model_animations();

    struct Animator_state
    {
        uint32_t animation_idx;
        float_t speed{ 1.0f };
        bool loop{ true };
    };
    void configure_animator(std::vector<Animator_state>&& animator_states);

    void change_state_idx(uint32_t to_state);
    void update(float_t delta_time);
    void calc_anim_pose(std::vector<mat4s>& out_joint_matrices) const;
    void get_anim_floored_frame_pose(std::vector<mat4s>& out_joint_matrices) const;

private:
    // @TEMP: Super simple animator right here for now.
    std::atomic_uint32_t m_current_state_idx{ 0 };
    std::atomic<float_t> m_time{ 0.0f };
    ///////////////////////////////////////////////////

    std::vector<Animator_state> m_animator_states;
    std::vector<Model_joint_animation> const& m_model_animations;
};

}  // namespace BT
