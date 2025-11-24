#pragma once

#include "animator_template_types.h"
#include "btjson.h"

#include <array>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


namespace BT
{

class Model_animator;

struct Animator_template
{
    struct Animator_state
    {
        std::string state_name;
        std::string anim_name;
        float_t     speed;
        bool        loop;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Animator_state,
                                       state_name,
                                       anim_name,
                                       speed,
                                       loop);
    };
    std::vector<Animator_state> animator_states;

    struct Transition_intermediate_state
    {
        std::string                         trans_state_name;
        std::pair<std::string, std::string> from_to_state;
        std::string                         anim_name;
        float_t                             speed;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(Transition_intermediate_state,
                                       trans_state_name,
                                       from_to_state,
                                       anim_name,
                                       speed);
    };
    std::vector<Transition_intermediate_state> transition_intermediate_states;

    /// Variables to be used in state transition conditions.
    std::vector<std::string> variables;

    /// DO NOT INCLUDE IN SERIALIZATION.
    std::vector<anim_tmpl_types::Animator_variable> variables_cooked;

    struct State_transition
    {
        std::array<std::string, 2> from_to_state;
        std::string condition;

        NLOHMANN_DEFINE_TYPE_INTRUSIVE(State_transition,
                                       from_to_state,
                                       condition);

        /// DO NOT INCLUDE IN SERIALIZATION.
        anim_tmpl_types::Animator_state_transition cooked;
    };
    std::vector<State_transition> state_transitions;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE(Animator_template,
                                   animator_states,
                                   transition_intermediate_states,
                                   variables,
                                   state_transitions);
};

class Animator_template_bank
{
public:
    Animator_template_bank();

    Animator_template const& load_animator_template(std::string const& anim_template_name);
    void load_animator_template_into_animator(Model_animator& animator,
                                              std::string const& anim_template_name);

private:
    std::unordered_map<std::string, Animator_template> m_anim_template_cache;
};

}  // namespace BT
