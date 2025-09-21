#include "animator_template.h"

#include "../btzc_game_engine.h"
#include "../renderer/model_animator.h"
#include "../service_finder/service_finder.h"

#include "nlohmann/json.hpp"
using json = nlohmann::json;

#include <fstream>


BT::Animator_template_bank::Animator_template_bank()
{
    // Add self as service.
    BT_SERVICE_FINDER_ADD_SERVICE(Animator_template_bank, this);
}

void BT::Animator_template_bank::load_animator_template_into_animator(
    Model_animator& animator,
    std::string const& anim_template_name)
{
    if (m_anim_template_cache.find(anim_template_name) == m_anim_template_cache.end())
    {   // Load from disk.
        static auto load_to_json_fn = [](std::string const& fname) {  // @COPYPASTA.
            std::ifstream f{ fname };
            return json::parse(f);
        };
        json root = load_to_json_fn(BTZC_GAME_ENGINE_ASSET_ANIMATOR_TEMPLATES_PATH
                                    + anim_template_name);

        // Fill in new struct.
        Animator_template new_template;

        new_template.animator_states.reserve(root["animator_states"].size());
        for (auto& anim_state_json : root["animator_states"])
        {
            new_template.animator_states.emplace_back(
                anim_state_json["state_name"].get<std::string>(),
                anim_state_json["anim_name"].get<std::string>(),
                anim_state_json["speed"].get<float_t>(),
                anim_state_json["loop"].get<bool>());
        }

        new_template.transition_states.reserve(root["transition_states"].size());
        for (auto& trans_state_json : root["transition_states"])
        {
            new_template.transition_states.emplace_back(
                trans_state_json["trans_state_name"].get<std::string>(),
                std::make_pair(trans_state_json["from_to_state"][0].get<std::string>(),
                               trans_state_json["from_to_state"][1].get<std::string>()),
                trans_state_json["anim_name"].get<std::string>(),
                trans_state_json["speed"].get<float_t>());
        }

        m_anim_template_cache.emplace(anim_template_name, std::move(new_template));
    }

    // Grab template from cache.
    auto& anim_temp{ m_anim_template_cache.at(anim_template_name) };

    // Write to model animator.
    std::vector<Model_animator::Animator_state> anim_states;
    anim_states.reserve(anim_temp.animator_states.size());
    for (auto& temp_anim_state : anim_temp.animator_states)
    {
        anim_states.emplace_back(animator.get_model_animation_idx(
                                     temp_anim_state.anim_name),
                                 temp_anim_state.speed,
                                 temp_anim_state.loop);
    }
    // @TODO: Also include transition states in model animator.

    animator.configure_animator_states(std::move(anim_states));
}
