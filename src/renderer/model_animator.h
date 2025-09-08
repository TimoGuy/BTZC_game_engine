#pragma once

#include "cglm/mat4.h"
#include "cglm/types-struct.h"
#include "cglm/types.h"
#include "cglm/util.h"
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>


namespace BT
{

namespace anim_frame_action { struct Runtime_data; }
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

    Model_joint_animation const& get_model_animation_by_idx(size_t idx);
    size_t get_num_model_animations();

    struct Animator_state
    {
        uint32_t animation_idx;
        float_t speed{ 1.0f };
        bool loop{ true };
    };
    void configure_animator(
        std::vector<Animator_state>&& animator_states,
        anim_frame_action::Runtime_data const* anim_frame_action_runtime_state);

    void change_state_idx(uint32_t to_state);
    void set_time(float_t time);
    void update(float_t delta_time);
    void calc_anim_pose(std::vector<mat4s>& out_joint_matrices) const;
    void get_anim_floored_frame_pose(std::vector<mat4s>& out_joint_matrices) const;

private:
    // @TEMP: Super simple animator right here for now.
    std::atomic_uint32_t m_current_state_idx{ 0 };
    std::atomic<float_t> m_time{ 0.0f };
    ///////////////////////////////////////////////////

    std::vector<Model_joint_animation> const& m_model_animations;
    std::vector<Animator_state> m_animator_states;
    anim_frame_action::Runtime_data const* m_anim_frame_action_runtime_state{ nullptr };




    ////////////////////////////////////////////////////////////////////////////////////////////////
    /// @NOTE: vvvBELOWvvv: Move into its own class and have the model animator use it instead of
    ///        this abomination lol.
    ////////////////////////////////////////////////////////////////////////////////////////////////

    // Controllable data.
    #define BT_MODEL_ANIMATOR_CONTROLLABLE_DATA_LIST    \
        /* Floats */                                    \
        X_float(model_opacity,     1.0f)                \
        X_float(turn_speed,        0.0f)                \
        X_float(move_speed,        0.0f)                \
        X_float(gravity_magnitude, 1.0f)                \
        /* Bools */                                     \
        X__bool(is_parry_active,       false)           \
        X__bool(can_move_exit,         true)            \
        X__bool(can_guard_exit,        true)            \
        X__bool(can_attack_exit,       true)            \
        X__bool(blade_has_mizunokata,  false)           \
        X__bool(blade_has_honoonokata, false)           \
        X__bool(show_hurtbox_bicep_r,  false)           \
        X__bool(hide_hitbox_leg_l,     false)           \
        /* Rising edge events */                        \
        X_reeve(play_sfx_footstep)                      \
        X_reeve(play_sfx_ready_guard)                   \
        X_reeve(play_sfx_blade_swing)                   \
        X_reeve(play_sfx_hurt_vocalize_human_male_mc)   \
        X_reeve(play_sfx_guard_receive_hit)             \
        X_reeve(play_sfx_deflect_receive_hit)

public:
    // Enum table.
    enum Controllable_data_label : std::uint32_t
    {
    INTERNAL__CTRL_DATA_LABEL_MARKER_BEGIN_FLOAT = 0,

    #define X_float(name, def_val)  CTRL_DATA_LABEL_##name,
    #define X__bool(name, def_val)
    #define X_reeve(name)
    BT_MODEL_ANIMATOR_CONTROLLABLE_DATA_LIST
    #undef X_float
    #undef X__bool
    #undef X_reeve

    INTERNAL__CTRL_DATA_LABEL_MARKER_END_FLOAT_BEGIN_BOOL,

    #define X_float(name, def_val)
    #define X__bool(name, def_val)  CTRL_DATA_LABEL_##name,
    #define X_reeve(name)
    BT_MODEL_ANIMATOR_CONTROLLABLE_DATA_LIST
    #undef X_float
    #undef X__bool
    #undef X_reeve

    INTERNAL__CTRL_DATA_LABEL_MARKER_END_BOOL_BEGIN_REEVE,

    #define X_float(name, def_val)
    #define X__bool(name, def_val)
    #define X_reeve(name)           CTRL_DATA_LABEL_##name,
    BT_MODEL_ANIMATOR_CONTROLLABLE_DATA_LIST
    #undef X_float
    #undef X__bool
    #undef X_reeve

    INTERNAL__CTRL_DATA_LABEL_MARKER_END_REEVE
    };

    // Get list of str labels.
    static std::vector<std::string> const& get_all_str_labels()
    {
        static std::vector<std::string> s_all_labels{
            #define X_float(name, def_val)  #name,
            #define X__bool(name, def_val)  #name,
            #define X_reeve(name)           #name,
            BT_MODEL_ANIMATOR_CONTROLLABLE_DATA_LIST
            #undef X_float
            #undef X__bool
            #undef X_reeve
        };
        return s_all_labels;
    }

    // Lookup str->enum.
    static Controllable_data_label str_label_to_enum(std::string const& str_label)
    {
        static std::unordered_map<std::string, Controllable_data_label> s_label_to_enum_map{
            #define X_float(name, def_val)  { #name, CTRL_DATA_LABEL_##name },
            #define X__bool(name, def_val)  { #name, CTRL_DATA_LABEL_##name },
            #define X_reeve(name)           { #name, CTRL_DATA_LABEL_##name },
            BT_MODEL_ANIMATOR_CONTROLLABLE_DATA_LIST
            #undef X_float
            #undef X__bool
            #undef X_reeve
        };
        return s_label_to_enum_map.at(str_label);
    }

    // Lookup data type.
    enum Controllable_data_type
    {
        CTRL_DATA_TYPE_FLOAT,
        CTRL_DATA_TYPE_BOOL,
        CTRL_DATA_TYPE_RISING_EDGE_EVENT,
        CTRL_DATA_TYPE_UNKNOWN,
    };
    static Controllable_data_type get_data_type(Controllable_data_label label)
    {
        Controllable_data_type ctrl_data_type;
        if (label > INTERNAL__CTRL_DATA_LABEL_MARKER_BEGIN_FLOAT &&
            label < INTERNAL__CTRL_DATA_LABEL_MARKER_END_FLOAT_BEGIN_BOOL)
        {
            ctrl_data_type = CTRL_DATA_TYPE_FLOAT;
        }
        else if (label > INTERNAL__CTRL_DATA_LABEL_MARKER_END_FLOAT_BEGIN_BOOL &&
                 label < INTERNAL__CTRL_DATA_LABEL_MARKER_END_BOOL_BEGIN_REEVE)
        {
            ctrl_data_type = CTRL_DATA_TYPE_BOOL;
        }
        else if (label > INTERNAL__CTRL_DATA_LABEL_MARKER_END_BOOL_BEGIN_REEVE &&
                 label < INTERNAL__CTRL_DATA_LABEL_MARKER_END_REEVE)
        {
            ctrl_data_type = CTRL_DATA_TYPE_RISING_EDGE_EVENT;
        }
        else
        {   // Unknown data type.
            assert(false);
            ctrl_data_type = CTRL_DATA_TYPE_UNKNOWN;
        }
        return ctrl_data_type;
    }

private:
    // Lookup reading/writing handle for data.
    std::unordered_map<Controllable_data_label, float_t> m_data_floats;  // @TODO: private.
    std::unordered_map<Controllable_data_label, bool> m_data_bools;  // @TODO: private.

public:
    class Rising_edge_event  // @TODO: public.
    {
    public:
        void mark_rising_edge() { m_rising_edge_count++; }
        bool check_if_rising_edge_occurred()
        {
            if (m_rising_edge_count > 0)
            {
                m_rising_edge_count--;
                m__dev_re_ocurred_cooldown = 1.0f;
                return true;
            }
            else
                return false;
        }
        float_t update_cooldown_and_fetch_val(float_t delta_time)
        {
            auto cooldown_prev_copy{ m__dev_re_ocurred_cooldown };
            m__dev_re_ocurred_cooldown = glm_max(0.0f,
                                                 m__dev_re_ocurred_cooldown
                                                 - delta_time);
            return cooldown_prev_copy;
        }
    private:
        uint32_t m_rising_edge_count{ 0 };
        float_t m__dev_re_ocurred_cooldown{ 0.0f };
    };
private:
    std::unordered_map<Controllable_data_label, Rising_edge_event> m_data_reeves;  // @TODO: private.

public:
    float_t& get_float_data_handle(Controllable_data_label label)
    {
        assert(get_data_type(label) == CTRL_DATA_TYPE_FLOAT);
        return m_data_floats.at(label);
    }

    bool& get_bool_data_handle(Controllable_data_label label)
    {
        assert(get_data_type(label) == CTRL_DATA_TYPE_BOOL);
        return m_data_bools.at(label);
    }

    Rising_edge_event& get_reeve_data_handle(Controllable_data_label label)
    {
        assert(get_data_type(label) == CTRL_DATA_TYPE_RISING_EDGE_EVENT);
        return m_data_reeves.at(label);
    }

    #undef BT_MODEL_ANIMATOR_CONTROLLABLE_DATA_LIST
};

}  // namespace BT
