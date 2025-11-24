#include "animator_template.h"

#include "../btzc_game_engine.h"
#include "../renderer/model_animator.h"
#include "../service_finder/service_finder.h"
#include "animator_template_types.h"
#include "btjson.h"

#include <cassert>
#include <sstream>
#include <iterator>
#include <string>


BT::Animator_template_bank::Animator_template_bank()
{
    // Add self as service.
    BT_SERVICE_FINDER_ADD_SERVICE(Animator_template_bank, this);
}

BT::Animator_template const& BT::Animator_template_bank::load_animator_template(
    std::string const& anim_template_name)
{
    if (m_anim_template_cache.find(anim_template_name) == m_anim_template_cache.end())
    {   // Load from disk.
        json root = json_load_from_disk(BTZC_GAME_ENGINE_ASSET_ANIMATOR_TEMPLATES_PATH +
                                        anim_template_name);

        // Fill in new struct.
        Animator_template new_template = root;

        // Cook animator template variables.
        new_template.variables_cooked.resize(new_template.variables.size());
        for (size_t i = 0; i < new_template.variables.size(); i++)
        {   // Get tokens of variable string.
            std::vector<std::string> tokens;
            {
                std::istringstream iss(new_template.variables[i]);
                std::copy(std::istream_iterator<std::string>(iss),
                          std::istream_iterator<std::string>(),
                          std::back_inserter(tokens));
                assert(tokens.size() == 2);
            }

            // Start cookin'
            auto& ckd{ new_template.variables_cooked[i] };

            // Convert type.
            if      (tokens[0] == "bool")  ckd.type = anim_tmpl_types::Animator_variable::TYPE_BOOL;
            else if (tokens[0] == "int")   ckd.type = anim_tmpl_types::Animator_variable::TYPE_INT;
            else if (tokens[0] == "float") ckd.type = anim_tmpl_types::Animator_variable::TYPE_FLOAT;
            else if (tokens[0] == "trig")  ckd.type = anim_tmpl_types::Animator_variable::TYPE_TRIGGER;
            else assert(false);

            // Convert name.
            ckd.var_name = tokens[1];
        }

        // Cook animator template state transitions.
        for (auto& state_trans : new_template.state_transitions)
        {
            auto& ckd{ state_trans.cooked };

            // Convert from-to-state.
            for (size_t i = 0; i < state_trans.from_to_state.size(); i++)
            for (size_t j = 0; j < new_template.animator_states.size(); j++)
            if (state_trans.from_to_state[i] == new_template.animator_states[j].state_name)
            {
                ckd.from_to_state[i] = j;
                break;
            }

            // Get tokens of condition string.
            std::vector<std::string> tokens;
            {
                std::istringstream iss(state_trans.condition);
                std::copy(std::istream_iterator<std::string>(iss),
                          std::istream_iterator<std::string>(),
                          std::back_inserter(tokens));
                assert(tokens.size() == 1 || tokens.size() == 3);
            }

            // Convert var idx.
            auto var_type{ anim_tmpl_types::Animator_variable::TYPE_INVALID };

            if (tokens[0] == "ON_ANIM_END")
            {   // Special case.
                ckd.condition_var_idx = anim_tmpl_types::k_on_anim_end_idx;
                var_type = anim_tmpl_types::Animator_variable::TYPE_TRIGGER;
            }
            else
            {
                bool found{ false };
                for (size_t i = 0; i < new_template.variables_cooked.size(); i++)
                    if (tokens[0] == new_template.variables_cooked[i].var_name)
                    {   // Found var name!
                        ckd.condition_var_idx = i;
                        var_type = new_template.variables_cooked[i].type;
                        found = true;
                        break;
                    }

                if (!found)
                {
                    BT_ERRORF("Var name not found: %s", tokens[0].c_str());
                    assert(false);
                }
            }

            // Convert compare op and value.
            switch (var_type)
            {
            case anim_tmpl_types::Animator_variable::TYPE_BOOL:
                if      (tokens[1] == "eq")  ckd.compare_operator = ckd.COMP_EQ;
                else if (tokens[1] == "neq") ckd.compare_operator = ckd.COMP_NEQ;
                else assert(false);

                if      (tokens[2] == "false") ckd.compare_value = anim_tmpl_types::k_bool_false;
                else if (tokens[2] == "true")  ckd.compare_value = anim_tmpl_types::k_bool_true;
                else assert(false);
                break;

            case anim_tmpl_types::Animator_variable::TYPE_INT:
                if      (tokens[1] == "eq")      ckd.compare_operator = ckd.COMP_EQ;
                else if (tokens[1] == "neq")     ckd.compare_operator = ckd.COMP_NEQ;
                else if (tokens[1] == "less")    ckd.compare_operator = ckd.COMP_LESS;
                else if (tokens[1] == "leq")     ckd.compare_operator = ckd.COMP_LEQ;
                else if (tokens[1] == "greater") ckd.compare_operator = ckd.COMP_GREATER;
                else if (tokens[1] == "geq")     ckd.compare_operator = ckd.COMP_GEQ;
                else assert(false);

                assert(false);  // @TODO: Implement str-to-int here!
                break;

            case anim_tmpl_types::Animator_variable::TYPE_FLOAT:
                if      (tokens[1] == "eq")      ckd.compare_operator = ckd.COMP_EQ;
                else if (tokens[1] == "neq")     ckd.compare_operator = ckd.COMP_NEQ;
                else if (tokens[1] == "less")    ckd.compare_operator = ckd.COMP_LESS;
                else if (tokens[1] == "leq")     ckd.compare_operator = ckd.COMP_LEQ;
                else if (tokens[1] == "greater") ckd.compare_operator = ckd.COMP_GREATER;
                else if (tokens[1] == "geq")     ckd.compare_operator = ckd.COMP_GEQ;
                else assert(false);

                assert(false);  // @TODO: Implement str-to-float here!
                break;

            case anim_tmpl_types::Animator_variable::TYPE_TRIGGER:
                ckd.compare_operator = ckd.COMP_EQ;
                ckd.compare_value = anim_tmpl_types::k_trig_triggered;
                break;

            default: assert(false); break;
            }
        }

        m_anim_template_cache.emplace(anim_template_name, std::move(new_template));
    }


    // Grab template from cache.
    return m_anim_template_cache.at(anim_template_name);
}

void BT::Animator_template_bank::load_animator_template_into_animator(
    Model_animator& animator,
    std::string const& anim_template_name)
{
    auto const& anim_temp{ load_animator_template(anim_template_name) };

    // Write to model animator.
    std::vector<anim_tmpl_types::Animator_state> anim_states;
    anim_states.reserve(anim_temp.animator_states.size());
    for (auto const& temp_anim_state : anim_temp.animator_states)
    {
        anim_states.emplace_back(temp_anim_state.state_name,
                                 animator.get_model_animation_idx(
                                     temp_anim_state.anim_name),
                                 temp_anim_state.speed,
                                 temp_anim_state.loop);
    }
    
    // @TODO: Also include transition states in model animator.

    std::vector<anim_tmpl_types::Animator_state_transition> anim_state_transitions;
    anim_state_transitions.reserve(anim_temp.state_transitions.size());
    for (auto const& temp_state_trans : anim_temp.state_transitions)
    {
        anim_state_transitions.emplace_back(temp_state_trans.cooked);
    }

    animator.configure_animator_states(anim_states,
                                       anim_temp.variables_cooked,
                                       anim_state_transitions);
}
