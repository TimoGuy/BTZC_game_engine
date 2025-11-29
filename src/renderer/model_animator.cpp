#include "model_animator.h"

#include "../animation_frame_action_tool/runtime_data.h"
#include "animator_template_types.h"
#include "btglm.h"
#include "btlogger.h"
#include "mesh.h"
#include "uuid/uuid.h"

#include <algorithm>
#include <cmath>
#include <cstddef>


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
                                                    bool root_motion_zeroing,
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
        
        if (i == 0 && root_motion_zeroing)
        {   // Delete root motion (for XZ axes).
            local_joint_transform.position[0] = local_joint_transform.position[2] = 0;
        }

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
}

void BT::Model_joint_animation::get_joint_matrices_at_frame(
    uint32_t frame_idx,
    std::vector<mat4s>& out_joint_matrices) const
{   ////////////////////////////////////////////////////////////////////////////////////////////////
    // @COPYPASTA below from `calc_joint_matrices()`
    //   Note the commented out sections of code. This marks the parts that are removed from the
    //   original source material. And then, lines with `// +` at the end are marked for showing
    //   addition of the source material.
    //
    // @NOTE: So I think that the best thing to do here is just copy the contents of the
    //   `calc_joint_matrices()` function and then instead of using `interpolate_fast()`, just
    //   return the first one essentially. It'll just be copypasta for this run around but maybe in
    //   the future a more efficient way for `get_joint_matrices_at_frame()` can be concocted when
    //   the program demands the performance.  -Thea 2025/09/27
    ////////////////////////////////////////////////////////////////////////////////////////////////

    // uint32_t frame_idx_a{ calc_frame_idx(time, loop, FLOOR) };
    // uint32_t frame_idx_b{ calc_frame_idx(time, loop, CEIL) };

    // float_t interp_t{ (time / k_frames_per_second)
    //                   - std::floor(time / k_frames_per_second) };

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

        // auto local_joint_transform{
        //     m_frames[frame_idx_a].joint_transforms_in_order[i].interpolate_fast(
        //         m_frames[frame_idx_b].joint_transforms_in_order[i],
        //         interp_t) };
        auto& local_joint_transform{                                                            // +
            const_cast<Model_joint_animation_frame::Joint_local_transform&>(                    // +
                m_frames[frame_idx].joint_transforms_in_order[i])                               // +
        };                                                                                      // +

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
}

void BT::Model_joint_animation::get_joint_matrices_at_frame_with_root_motion(
    uint32_t frame_idx,
    std::vector<mat4s>& out_joint_matrices) const
{   ////////////////////////////////////////////////////////////////////////////////////////////////
    // @COPYPASTA below from `calc_joint_matrices()` (and then `get_joint_matrices_at_frame()`)
    //   Note the commented out sections of code. This marks the parts that are removed from the
    //   original source material. And then, lines with `// +` at the end are marked for showing
    //   addition of the source material.
    //
    // @NOTE: So I think that the best thing to do here is just copy the contents of the
    //   `calc_joint_matrices()` function and then instead of using `interpolate_fast()`, just
    //   return the first one essentially. It'll just be copypasta for this run around but maybe in
    //   the future a more efficient way for `get_joint_matrices_at_frame()` can be concocted when
    //   the program demands the performance.  -Thea 2025/09/27
    //
    // @NOTE: Hey, I really don't like these copypastas. These really need some kind of
    //   consolidation/abstraction for this!!!  -Thea 2025/11/26
    ////////////////////////////////////////////////////////////////////////////////////////////////

    // uint32_t frame_idx_a{ calc_frame_idx(time, loop, FLOOR) };
    // uint32_t frame_idx_b{ calc_frame_idx(time, loop, CEIL) };

    // float_t interp_t{ (time / k_frames_per_second)
    //                   - std::floor(time / k_frames_per_second) };

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

        // auto local_joint_transform{
        //     m_frames[frame_idx_a].joint_transforms_in_order[i].interpolate_fast(
        //         m_frames[frame_idx_b].joint_transforms_in_order[i],
        //         interp_t) };
        auto& local_joint_transform{                                                            // +
            const_cast<Model_joint_animation_frame::Joint_local_transform&>(                    // +
                m_frames[frame_idx].joint_transforms_in_order[i])                               // +
        };                                                                                      // +

        if (i == 0)                                                                             // +
        {   // Delete root motion (for XZ axes).                                                // +
            local_joint_transform.position[0] = local_joint_transform.position[2] = 0;          // +
        }                                                                                       // +

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
}

void BT::Model_joint_animation::get_root_motion_delta_pos_at_frame(
    uint32_t frame_idx,
    vec3& out_root_motion_delta_pos) const
{   // Include root motion delta position (Just XZ axes).
    glm_vec3_copy(const_cast<float_t*>(m_frames[frame_idx].root_motion_delta_pos),
                  out_root_motion_delta_pos);
    out_root_motion_delta_pos[1] = 0;
}


BT::Model_animator::Model_animator(Model const& model, bool use_root_motion)
    : m_model_animations{ model.m_animations }
    , m_model_skin{ model.m_model_skin }
    , m_is_using_root_motion{ use_root_motion }
{
}

BT::Model_skin const& BT::Model_animator::get_model_skin() const
{
    return m_model_skin;
}

void BT::Model_animator::configure_animator_states(
    std::vector<anim_tmpl_types::Animator_state> animator_states,
    std::vector<anim_tmpl_types::Animator_variable> animator_variables,
    std::vector<anim_tmpl_types::Animator_state_transition> animator_state_transitions)
{
    // Idk why I put this into a separate method instead of in the constructor but hey, here we are.
    // @NOTE: A lot of copying, but I'm @TEMP temporarily doing this for a looser interface.
    m_animator_states = animator_states;
    m_animator_variables = animator_variables;
    m_animator_state_transitions = animator_state_transitions;
}

void BT::Model_animator::configure_anim_frame_action_controls(
    anim_frame_action::Runtime_data_controls const* anim_frame_action_controls,
    UUID resp_entity_uuid)
{
    // Idk why I put this into a separate method instead of in the constructor but hey, here we are.
    m_anim_frame_action_controls = anim_frame_action_controls;

    m_anim_frame_action_data.map_animator_to_control_regions(*this, *m_anim_frame_action_controls);

    m_anim_frame_action_data.hitcapsule_group_set.replace_and_reregister(
        m_anim_frame_action_controls->data.hitcapsule_group_set_template,
        resp_entity_uuid);
    m_anim_frame_action_data.hitcapsule_group_set.connect_animator(*this);
}

std::vector<BT::anim_tmpl_types::Animator_state> const&
BT::Model_animator::get_animator_states() const
{
    return m_animator_states;
}

BT::anim_tmpl_types::Animator_state const&
BT::Model_animator::get_animator_state(size_t idx) const
{
    return m_animator_states[idx];
}

BT::anim_tmpl_types::Animator_state&
BT::Model_animator::get_animator_state_write_handle(size_t idx)
{
    return m_animator_states[idx];
}

size_t BT::Model_animator::get_num_animator_variables() const
{
    return m_animator_variables.size();
}

BT::anim_tmpl_types::Animator_variable const& BT::Model_animator::get_animator_variable(size_t idx) const
{
    return m_animator_variables[idx];
}

BT::anim_tmpl_types::Animator_variable& BT::Model_animator::get_animator_variable_write_handle(size_t idx)
{
    return m_animator_variables[idx];
}

void BT::Model_animator::change_state_idx(uint32_t to_state)
{
    uint32_t from_state_copy{ m_current_state_idx.load() };
    if (m_current_state_idx.compare_exchange_strong(from_state_copy,
                                                    to_state))
    {   // Reset all time profiles.
        set_time(0.0f);
        m_prev_sim_time = std::numeric_limits<float_t>::lowest();  // @TODO: Make abstract?
    }
}

size_t BT::Model_animator::get_model_animation_idx(std::string anim_name) const
{
    size_t idx{ (size_t)-1 };

    for (size_t i = 0; i < m_model_animations.size(); i++)
        if (m_model_animations[i].get_name() == anim_name)
        {   // Found name.
            idx = i;
            break;
        }

    return idx;
}

BT::Model_joint_animation const& BT::Model_animator::get_model_animation(size_t idx) const
{
    return m_model_animations[idx];
}

void BT::Model_animator::set_bool_variable(std::string const& var_name, bool value)
{
    auto& var_handle{ find_animator_variable(var_name) };

    if (var_handle.type != anim_tmpl_types::Animator_variable::TYPE_BOOL)
    {
        assert(false);
        return;
    }

    var_handle.var_value = (value ? anim_tmpl_types::k_bool_true
                                  : anim_tmpl_types::k_bool_false);
}

void BT::Model_animator::set_int_variable(std::string const& var_name, int32_t value)
{
    auto& var_handle{ find_animator_variable(var_name) };

    if (var_handle.type != anim_tmpl_types::Animator_variable::TYPE_INT)
    {
        assert(false);
        return;
    }

    var_handle.var_value = value;
}

void BT::Model_animator::set_float_variable(std::string const& var_name, float_t value)
{
    auto& var_handle{ find_animator_variable(var_name) };

    if (var_handle.type != anim_tmpl_types::Animator_variable::TYPE_FLOAT)
    {
        assert(false);
        return;
    }

    var_handle.var_value = value;
}

void BT::Model_animator::set_trigger_variable(std::string const& var_name)
{
    auto& var_handle{ find_animator_variable(var_name) };

    if (var_handle.type != anim_tmpl_types::Animator_variable::TYPE_TRIGGER)
    {
        assert(false);
        return;
    }

    var_handle.var_value = anim_tmpl_types::k_trig_triggered;
}

void BT::Model_animator::set_time(float_t time)
{
    m_sim_time  = time;
    m_rend_time = time;
}

void BT::Model_animator::update(Animator_timer_profile profile, float_t delta_time)
{   // Get timer to work with.
    animator_time_t& time_handle{ get_profile_time_handle(profile) };

    // @TODO: There needs to be some kind of time syncing between timers. Since the creation of
    //        setting triggers and variables to switch states, there will be issues when changing
    //        states.
    // @THOUGHT: Well, ig since `set_time()` will be setting all the timers, then it will start out
    //           synced up enough? Only the simulation loop is going to be changing states inside
    //           the animator.

    // Tick forward.
    auto const& anim_state{ m_animator_states[m_current_state_idx] };
    time_handle += delta_time * anim_state.speed;

    // Process animator state transitions.
    if (profile == SIMULATION_PROFILE)
    {
        bool state_changed{ false };
        uint32_t prev_state_idx;
        uint32_t curr_state_idx{ m_current_state_idx };
        do
        {   // Keep track of whether state changes.
            prev_state_idx = curr_state_idx;

            // Look for possible state transitions.
            for (auto const& state_trans : m_animator_state_transitions)
            for (auto from_state_idx : state_trans.from_to_state.first)
            if (from_state_idx == curr_state_idx)
            {   // Possible state transition.
                bool do_transition{ false };
                if (state_trans.condition_var_idx == anim_tmpl_types::k_on_anim_end_var_idx)
                {
                    if (!state_changed)
                    {   // Special ON_ANIM_END case.
                        // @NOTE: Since this is frame dependent, we need to make sure that we're
                        //        using the correct animation idx (hence `!state_changed` check).
                        //        -Thea 2025/11/23
                        auto const& model_anim{ m_model_animations[anim_state.animation_idx] };
                        if (model_anim.calc_frame_idx(time_handle.load(),
                                                      false,
                                                      Model_joint_animation::FLOOR) ==
                            model_anim.get_num_frames() - 1)
                        {
                            do_transition = true;
                        }
                    }
                }
                else
                {   // Normal condition var.
                    auto const& anim_var{ m_animator_variables[state_trans.condition_var_idx] };

                    switch (state_trans.compare_operator)
                    {
                    case anim_tmpl_types::Animator_state_transition::COMP_EQ:
                        do_transition = glm_eq(anim_var.var_value, state_trans.compare_value);
                        break;

                    case anim_tmpl_types::Animator_state_transition::COMP_NEQ:
                        do_transition = !glm_eq(anim_var.var_value, state_trans.compare_value);
                        break;

                    case anim_tmpl_types::Animator_state_transition::COMP_LESS:
                        do_transition = (anim_var.var_value < state_trans.compare_value);
                        break;

                    case anim_tmpl_types::Animator_state_transition::COMP_LEQ:
                        do_transition = (anim_var.var_value <= state_trans.compare_value);
                        break;

                    case anim_tmpl_types::Animator_state_transition::COMP_GREATER:
                        do_transition = (anim_var.var_value > state_trans.compare_value);
                        break;

                    case anim_tmpl_types::Animator_state_transition::COMP_GEQ:
                        do_transition = (anim_var.var_value >= state_trans.compare_value);
                        break;

                    default: assert(false); break;
                    }
                }

                if (do_transition)
                {   // Transition states!
                    curr_state_idx = state_trans.from_to_state.second;
                    state_changed = true;
                    break;
                }
            }
        } while (prev_state_idx != curr_state_idx);

        // Erase all trigger activations!
        for (auto& anim_var : m_animator_variables)
        if (anim_var.type == anim_tmpl_types::Animator_variable::TYPE_TRIGGER)
        {
            anim_var.var_value = 0;
        }

        // Perform actual state change!
        if (state_changed)
            change_state_idx(curr_state_idx);
    }

    // Process anim frame action controls.
    if (profile == SIMULATION_PROFILE &&
        m_anim_frame_action_controls != nullptr)
    {   // Get prev timer to work with.
        animator_time_t& prev_time_handle{ get_profile_prev_time_handle(profile) };

        // Copy current and previous times.
        float_t prev_time{ prev_time_handle };
        float_t curr_time{ time_handle };

        // Process anim frame action runtime.
        auto current_action_timeline_idx{
            m_anim_frame_action_data.anim_state_idx_to_timeline_idx_map.at(m_current_state_idx)
        };
        auto& afa_timeline{ m_anim_frame_action_controls->data
                                .anim_frame_action_timelines[current_action_timeline_idx] };

        m_anim_frame_action_data.clear_all_data_overrides();

        for (auto const& region : afa_timeline.regions)
        {
            auto& ctrl_item{
                m_anim_frame_action_controls->data.control_items[region.ctrl_item_idx] };
            if (ctrl_item.type == anim_frame_action::CTRL_ITEM_TYPE_EVENT_TRIGGER)
            {   // Check if rising edge (start_frame) of event is within prev_time/curr_time.
                float_t rising_edge_time = region.start_frame
                                           / Model_joint_animation::k_frames_per_second
                                           ;//* state_speed;  <-- @TODO: Include this when it's not editor mode (I think is the best decision)!!!!
                if (prev_time < rising_edge_time && rising_edge_time <= curr_time)
                {   // Add rising edge event trigger mark.
                    m_anim_frame_action_data
                        .get_reeve_data_handle(ctrl_item.affecting_data_label)
                        .mark_rising_edge();
                }
            }
            else
            {   // Check if time within frame bounds.
                auto frame_idx = m_model_animations[m_animator_states[m_current_state_idx].animation_idx]
                                 .calc_frame_idx(curr_time,
                                                 m_animator_states[m_current_state_idx].loop,
                                                 Model_joint_animation::FLOOR);
                if (frame_idx >= region.start_frame && frame_idx < region.end_frame)
                {   // Add override/write mutation.
                    bool is_bool_type{ anim_frame_action::Runtime_controllable_data
                                           ::get_data_type(ctrl_item.affecting_data_label)
                                       == anim_frame_action::Runtime_controllable_data
                                           ::CTRL_DATA_TYPE_BOOL };
                    switch (ctrl_item.type)
                    {   // @TODO: @NOCHECKIN: @IMPROVE: I wish this logic section would improve. It kinda sucks ngl.  -Thea 2025/09/15
                        case anim_frame_action::CTRL_ITEM_TYPE_DATA_OVERRIDE:
                            if (!is_bool_type)
                            {
                                m_anim_frame_action_data
                                    .get_float_data_handle(ctrl_item.affecting_data_label)
                                    .override_val(*reinterpret_cast<float_t const*>(&ctrl_item.data_point0));
                            }
                            else
                            {
                                m_anim_frame_action_data
                                    .get_bool_data_handle(ctrl_item.affecting_data_label)
                                    .override_val(*reinterpret_cast<bool const*>(&ctrl_item.data_point0));
                            }
                            break;

                        case anim_frame_action::CTRL_ITEM_TYPE_DATA_WRITE:
                            if (!is_bool_type)
                            {
                                m_anim_frame_action_data
                                    .get_float_data_handle(ctrl_item.affecting_data_label)
                                    .write_val(*reinterpret_cast<float_t const*>(&ctrl_item.data_point0));
                            }
                            else
                            {
                                m_anim_frame_action_data
                                    .get_bool_data_handle(ctrl_item.affecting_data_label)
                                    .write_val(*reinterpret_cast<bool const*>(&ctrl_item.data_point0));
                            }
                            break;

                        default:
                            // Unsupported ctrl item type.
                            assert(false);
                            break;
                    }
                }
            }
        }

        // Update prev time.
        prev_time_handle = time_handle.load();
    }
}

void BT::Model_animator::calc_anim_pose(Animator_timer_profile profile,
                                        std::vector<mat4s>& out_joint_matrices) const
{
    auto& anim_state{ m_animator_states[m_current_state_idx] };
    m_model_animations[anim_state.animation_idx]
        .calc_joint_matrices(get_profile_time_handle(profile).load(),
                             anim_state.loop,
                             false,
                             out_joint_matrices);
}

void BT::Model_animator::calc_anim_pose_with_root_motion_zeroing(
    Animator_timer_profile profile,
    std::vector<mat4s>& out_joint_matrices) const
{
    auto& anim_state{ m_animator_states[m_current_state_idx] };
    m_model_animations[anim_state.animation_idx]
        .calc_joint_matrices(get_profile_time_handle(profile).load(),
                             anim_state.loop,
                             true,
                             out_joint_matrices);
}

bool BT::Model_animator::get_is_using_root_motion() const
{
    return m_is_using_root_motion;
}

void BT::Model_animator::get_anim_floored_frame_pose(Animator_timer_profile profile,
                                                     std::vector<mat4s>& out_joint_matrices) const
{
    auto& anim_state{ m_animator_states[m_current_state_idx] };
    uint32_t frame_idx{
        m_model_animations[anim_state.animation_idx]
            .calc_frame_idx(get_profile_time_handle(profile).load(),
                            anim_state.loop,
                            Model_joint_animation::FLOOR) };
    m_model_animations[anim_state.animation_idx].get_joint_matrices_at_frame(frame_idx,
                                                                             out_joint_matrices);
}

void BT::Model_animator::get_anim_floored_frame_pose_with_root_motion_zeroing(
    Animator_timer_profile profile,
    std::vector<mat4s>& out_joint_matrices) const
{
    auto& anim_state{ m_animator_states[m_current_state_idx] };
    uint32_t frame_idx{
        m_model_animations[anim_state.animation_idx]
            .calc_frame_idx(get_profile_time_handle(profile).load(),
                            anim_state.loop,
                            Model_joint_animation::FLOOR) };
    m_model_animations[anim_state.animation_idx].get_joint_matrices_at_frame_with_root_motion(
        frame_idx,
        out_joint_matrices);
}

void BT::Model_animator::get_anim_root_motion_delta_pos(Animator_timer_profile profile,
                                                        vec3& out_root_motion_delta_pos) const
{
    auto& anim_state{ m_animator_states[m_current_state_idx] };
    uint32_t frame_idx{
        m_model_animations[anim_state.animation_idx]
            .calc_frame_idx(get_profile_time_handle(profile).load(),
                            anim_state.loop,
                            Model_joint_animation::FLOOR) };
    m_model_animations[anim_state.animation_idx].get_root_motion_delta_pos_at_frame(
        frame_idx,
        out_root_motion_delta_pos);
}

BT::anim_frame_action::Runtime_controllable_data&
BT::Model_animator::get_anim_frame_action_data_handle()
{
    return m_anim_frame_action_data;
}

// Please ignore the const_cast's below!! (^_^;)

BT::Model_animator::animator_time_t& BT::Model_animator::get_profile_time_handle(
    Animator_timer_profile profile) const
{
    switch (profile)
    {
    case SIMULATION_PROFILE: return const_cast<animator_time_t&>(m_sim_time);
    case RENDERER_PROFILE:   return const_cast<animator_time_t&>(m_rend_time);

    default:
        assert(false);
        return *reinterpret_cast<animator_time_t*>(0xDEADBEEF);
        break;
    }
}

BT::Model_animator::animator_time_t& BT::Model_animator::get_profile_prev_time_handle(
    Animator_timer_profile profile) const
{
    switch (profile)
    {
    case SIMULATION_PROFILE: return const_cast<animator_time_t&>(m_prev_sim_time);

    default:
        assert(false);
        return *reinterpret_cast<animator_time_t*>(0xDEADBEEF);
        break;
    }
}

BT::anim_tmpl_types::Animator_variable& BT::Model_animator::find_animator_variable(
    std::string const& var_name)
{
    for (auto& anim_var : m_animator_variables)
        if (anim_var.var_name == var_name)
        {   // Found variable!
            return anim_var;
        }

    // Crash the program when don't find the var.
    assert(false);
    throw new std::exception(("Did not find var name: " + var_name).c_str());
}
