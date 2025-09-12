#include "runtime_data.h"

#include "../renderer/mesh.h"
#include "../renderer/model_animator.h"
#include <fstream>
#include <string>
#include <unordered_map>


namespace
{

std::unordered_map<std::string, size_t> compile_anim_name_to_idx_map(
    std::vector<BT::Model_joint_animation> const& model_animations)
{
    std::unordered_map<std::string, size_t> rei_no_map;
    size_t idx{ 0 };
    for (auto& model_joint_anim : model_animations)
    {
        rei_no_map.emplace(model_joint_anim.get_name(), idx);
        idx++;
    }
    return rei_no_map;
}

}  // namespace

BT::anim_frame_action::Runtime_data::Runtime_data(std::string const& fname)
{   // @COPYPASTA: See `scene_serialization.cpp` vvvv
    static auto load_to_json_fn = [](std::string const& fname) {
        std::ifstream f{ fname };
        return json::parse(f);
    };
    json root = load_to_json_fn(fname);
    serialize(SERIAL_MODE_DESERIALIZE, root);
}

void BT::anim_frame_action::Runtime_data::serialize(
    Serialization_mode mode,
    json& node_ref)
{
    if (mode == SERIAL_MODE_DESERIALIZE)
    {   // Load model from bank.
        model = Model_bank::get_model(node_ref["animated_model_name"]);
        assert(model != nullptr);

        // Control items.
        control_items.clear();
        auto& nr_control_items{ node_ref["control_items"] };
        if (!nr_control_items.is_null() && nr_control_items.is_array())
        {
            control_items.reserve(nr_control_items.size());
            for (size_t i = 0; i < nr_control_items.size(); i++)
            {
                control_items.emplace_back(nr_control_items[i]["name"]);
                assert(false);  // @TODO: ^^ Add ctrl item type into the emplace right here!
            }
        }

        // Animations (use anim names as key).
        anim_frame_action_timelines.clear();
        auto& nr_anims{ node_ref["anim_frame_action_timelines"] };
        if (!nr_anims.is_null() && nr_anims.is_array())
        {
            auto anim_name_to_idx_map{ compile_anim_name_to_idx_map(model->get_joint_animations()) };
            anim_frame_action_timelines.resize(anim_name_to_idx_map.size());
            for (auto& nr_anim_entry : nr_anims)
            {   // Animations level.
                size_t anim_idx{ anim_name_to_idx_map.at(nr_anim_entry["anim_name"]) };
                auto& nr_anim_regions{ nr_anim_entry["regions"] };
                if (!nr_anim_regions.is_null() && nr_anim_regions.is_array())
                {   // Insert anim action regions.
                    anim_frame_action_timelines[anim_idx]
                        .regions.reserve(nr_anim_regions.size());
                    for (auto& nr_region : nr_anim_regions)
                        anim_frame_action_timelines[anim_idx].regions
                            .emplace_back(nr_region["ctrl_item_idx"].get<uint32_t>(),
                                          nr_region["start_frame"].get<int32_t>(),
                                          nr_region["end_frame"].get<int32_t>());
                }
            }
        }
    }
    else if (mode == SERIAL_MODE_SERIALIZE)
    {   // Save model from bank.
        node_ref["animated_model_name"] = Model_bank::get_model_name(model);

        // Control items.
        auto& nr_control_items{ node_ref["control_items"] };
        for (size_t i = 0; i < control_items.size(); i++)
        {
            nr_control_items[i]["name"] = control_items[i].name;
        }

        // Animations.
        auto& nr_anims{ node_ref["anim_frame_action_timelines"] };
        nr_anims = json::array();
        for (size_t anim_idx = 0;
             anim_idx < anim_frame_action_timelines.size();
             anim_idx++)
        {   // Animations level.
            json nr_anim_entry = {};
            nr_anim_entry["anim_name"] = model->get_joint_animations()[anim_idx].get_name();
            auto& nr_anim_regions{ nr_anim_entry["regions"] };
            nr_anim_regions = json::array();

            for (size_t reg_idx = 0;
                 reg_idx < anim_frame_action_timelines[anim_idx].regions.size();
                 reg_idx++)
            {   // Insert anim action regions.
                auto const& region{ anim_frame_action_timelines[anim_idx].regions[reg_idx] };
                json nr_region = {};
                nr_region["ctrl_item_idx"] = region.ctrl_item_idx;
                nr_region["start_frame"]   = region.start_frame;
                nr_region["end_frame"]     = region.end_frame;
                nr_anim_regions.emplace_back(nr_region);
            }

            nr_anims.emplace_back(nr_anim_entry);
        }
    }
}

void BT::anim_frame_action::Bank::emplace(std::string const& name,
                                          Runtime_data&& runtime_state)
{
    s_runtime_states.emplace(name, std::move(runtime_state));
}

void BT::anim_frame_action::Bank::replace(std::string const& name,
                                          Runtime_data&& runtime_state)
{
    s_runtime_states.at(name) = std::move(runtime_state);
}

BT::anim_frame_action::Runtime_data const&
BT::anim_frame_action::Bank::get(std::string const& name)
{
    return s_runtime_states.at(name);
}

std::vector<std::string> BT::anim_frame_action::Bank::get_all_names()
{
    std::vector<std::string> all_names;
    all_names.reserve(s_runtime_states.size());

    for (auto& [key, data] : s_runtime_states)
    {
        all_names.emplace_back(key);
    }

    return all_names;
}
