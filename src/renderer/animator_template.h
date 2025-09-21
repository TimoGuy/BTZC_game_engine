#pragma once

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>


namespace BT
{

class Model_animator;

class Animator_template_bank
{
public:
    Animator_template_bank();

    void load_animator_template_into_animator(Model_animator& animator,
                                              std::string const& anim_template_name);

private:
    struct Animator_template
    {
        struct Animator_state
        {
            std::string state_name;
            std::string anim_name;
            float_t     speed;
            bool        loop;
        };
        std::vector<Animator_state> animator_states;

        struct Transition_state
        {
            std::string                         trans_state_name;
            std::pair<std::string, std::string> from_to_state;
            std::string                         anim_name;
            float_t                             speed;
        };
        std::vector<Transition_state> transition_states;
    };

    std::unordered_map<std::string, Animator_template> m_anim_template_cache;
};

}  // namespace BT
