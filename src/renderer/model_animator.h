#pragma once

#include "../animation_frame_action_tool/runtime_data.h"
#include "btglm.h"
#include <atomic>
#include <limits>
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

    std::string get_name() const { return m_name; }
    size_t get_num_frames() const { return m_frames.size(); }

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

    Model_skin const& get_model_skin() const;

    struct Animator_state
    {
        std::string state_name;
        uint32_t animation_idx;
        float_t speed{ 1.0f };
        bool loop{ true };
    };
    void configure_animator_states(
        std::vector<Animator_state>&& animator_states);
    void configure_anim_frame_action_controls(
        anim_frame_action::Runtime_data_controls const* anim_frame_action_controls);

    std::vector<Animator_state> const& get_animator_states() const;
    Animator_state const& get_animator_state(size_t idx) const;

    Animator_state& get_animator_state_write_handle(size_t idx);

    void change_state_idx(uint32_t to_state);

    size_t get_model_animation_idx(std::string anim_name) const;
    Model_joint_animation const& get_model_animation(size_t idx) const;

    /// Sets time for all timer profiles of the animator.
    void set_time(float_t time);

    /// Profile enum for which timing of the animator to base calculations off of.
    enum Animator_timer_profile
    {
        SIMULATION_PROFILE,
        RENDERER_PROFILE,
    };

    /// Updates the animator, supplying a deltatime.
    /// There are two animator timers, so you need to give which timer to update.
    void update(Animator_timer_profile profile, float_t delta_time);

    /// Calculates the set of joint matrices, interpolated.
    void calc_anim_pose(Animator_timer_profile profile,
                        std::vector<mat4s>& out_joint_matrices) const;

    /// Calculates the set of joint matrices, floored. Note this one will be faster.
    void get_anim_floored_frame_pose(Animator_timer_profile profile,
                                     std::vector<mat4s>& out_joint_matrices) const;

    anim_frame_action::Runtime_controllable_data& get_anim_frame_action_data_handle();

private:
    std::vector<Model_joint_animation> const& m_model_animations;
    Model_skin const& m_model_skin;

    // @TEMP: Super simple animator right here for now.
    std::atomic_uint32_t m_current_state_idx{ 0 };

    // @NOTE: Times need to be atomic since `change_state_idx()` and `set_time()` can be called from
    //        any thread.
    using animator_time_t = typename std::atomic<float_t>;

    animator_time_t& get_profile_time_handle(Animator_timer_profile profile) const;
    animator_time_t& get_profile_prev_time_handle(Animator_timer_profile profile) const;

    animator_time_t m_sim_time{ 0.0f };
    animator_time_t m_prev_sim_time{ std::numeric_limits<float_t>::lowest() };  // For rising edge events.
    animator_time_t m_rend_time{ 0.0f };
    ///////////////////////////////////////////////////

    std::vector<Animator_state> m_animator_states;
    anim_frame_action::Runtime_data_controls const* m_anim_frame_action_controls{ nullptr };
    anim_frame_action::Runtime_controllable_data m_anim_frame_action_data;
};

}  // namespace BT
