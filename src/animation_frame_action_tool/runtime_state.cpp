#include "runtime_state.h"

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

BT::anim_frame_action::Runtime_state::Runtime_state(std::string const& fname,
                                                    Model const* model)
{   // @COPYPASTA: See `scene_serialization.cpp` vvvv
    static auto load_to_json_fn = [](std::string const& fname) {
        std::ifstream f{ fname };
        return json::parse(f);
    };
    json root = load_to_json_fn(fname);
    serialize(SERIAL_MODE_DESERIALIZE, model, root);
}

void BT::anim_frame_action::Runtime_state::serialize(
    Serialization_mode mode,
    Model const* model,
    json& node_ref)
{
    if (mode == SERIAL_MODE_DESERIALIZE)
    {   // Control items.
        control_items.clear();
        auto& nr_control_items{ node_ref["control_items"] };
        if (!nr_control_items.is_null() && nr_control_items.is_array())
        {
            control_items.reserve(nr_control_items.size());
            for (size_t i = 0; i < nr_control_items.size(); i++)
            {
                control_items.emplace_back(nr_control_items[i]["name"]);
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
    {

    }
}

void BT::anim_frame_action::Bank::emplace(std::string const& name,
                                          Runtime_state&& runtime_state)
{
    s_runtime_states.emplace(name, std::move(runtime_state));
}

BT::anim_frame_action::Runtime_state const&
BT::anim_frame_action::Bank::get(std::string const& name)
{
    return s_runtime_states.at(name);
}
