#pragma once

#include <array>
#include <cmath>
#include <cstdint>
#include <limits>
#include <string>
#include <vector>


namespace BT
{
namespace anim_tmpl_types
{

struct Animator_state
{
    std::string state_name;
    uint32_t animation_idx;
    float_t speed{ 1.0f };
    bool loop{ true };
};

struct Animator_variable
{
    enum Type : int32_t
    {
        TYPE_INVALID = -1,
        TYPE_BOOL,
        TYPE_INT,
        TYPE_FLOAT,
        TYPE_TRIGGER
    } type;

    std::string var_name;

    float_t var_value{ std::numeric_limits<float_t>::lowest() };
};

struct Animator_state_transition
{
    std::pair<std::vector<size_t>, size_t> from_to_state;  // Many "from" states to one "to" state.

    size_t condition_var_idx;

    enum Compare_op : int32_t
    {
        COMP_EQ,       // equal ==
        COMP_NEQ,      // not equal !=
        COMP_LESS,     // less than <
        COMP_LEQ,      // less than or equal to <=
        COMP_GREATER,  // greater than >
        COMP_GEQ,      // greater than or equal to >=
    } compare_operator;

    float_t compare_value;
};

/// Special condition var indexes.
static constexpr size_t k_on_anim_end_var_idx{ (size_t)-2 };

/// Var values for special types.
static constexpr float_t k_bool_false     = 0;
static constexpr float_t k_bool_true      = 1;
static constexpr float_t k_trig_triggered = 1;

}  // namespace animator_template_types
}  // namespace BT
