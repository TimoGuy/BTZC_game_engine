#pragma once

#include "cglm/types-struct.h"
#include "cglm/types.h"
#include <string>
#include <vector>


namespace BT
{

class Model_joint_animation
{
public:
    void calc_joint_matrices(float_t time, std::vector<mat4s>& out_joint_matrices) const;
    void get_joint_matrices_at_frame(uint32_t frame_idx, std::vector<mat4s>& out_joint_matrices) const;

private:
    std::string m_name;

    enum Interpolation_type
    {
        INTERP_TYPE_LINEAR = 0,
        INTERP_TYPE_STEP,
        NUM_INTERP_TYPES
    } m_interp_type;

    struct Animation_frame
    {
        vec3 position;
        versor rotation;
        vec3 scale;
    };
    std::vector<Animation_frame> m_frames;

    float_t m_frames_per_second{ 50.0f };
};

class Model;

class Model_animator
{
public:
    Model_animator(Model const& model);

    struct Animator_state
    {
        uint32_t animation_idx;
        float_t speed{ 1.0f };
        bool loop{ true };
    };
    void configure_animator(std::vector<Animator_state>&& animator_states);

    void update(float_t delta_time);
    void calc_anim_pose(std::vector<mat4s>& out_joint_matrices) const;
    void get_anim_frame_floored_pose(std::vector<mat4s>& out_joint_matrices) const;

private:
    // @TEMP: Super simple animator right here for now.
    uint32_t m_current_state_idx{ 0 };
    float_t m_time{ 0.0f };
    ///////////////////////////////////////////////////

    std::vector<Animator_state> m_animator_states;
    std::vector<Model_joint_animation> const& m_model_animations;
};

}  // namespace BT
