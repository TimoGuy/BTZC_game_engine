#include "runtime_data.h"

#include "../hitbox_interactor/hitcapsule.h"
#include "../renderer/animator_template.h"
#include "../renderer/mesh.h"
#include "../renderer/model_animator.h"
#include "../service_finder/service_finder.h"
#include "btjson.h"
#include <sstream>
#include <string>
#include <unordered_map>


// Controllable data.
void BT::anim_frame_action::Runtime_controllable_data::Rising_edge_event
    ::mark_rising_edge()
{
    m_rising_edge_count++;
}

bool BT::anim_frame_action::Runtime_controllable_data::Rising_edge_event
    ::check_if_rising_edge_occurred()
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

float_t BT::anim_frame_action::Runtime_controllable_data::Rising_edge_event
    ::update_cooldown_and_fetch_val(float_t delta_time)
{
    auto cooldown_prev_copy{ m__dev_re_ocurred_cooldown };
    m__dev_re_ocurred_cooldown = glm_max(0.0f,
                                         m__dev_re_ocurred_cooldown
                                         - delta_time);
    return cooldown_prev_copy;
}

BT::anim_frame_action::Runtime_controllable_data::Controllable_data_type
BT::anim_frame_action::Runtime_controllable_data::get_data_type(Controllable_data_label label)
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

BT::anim_frame_action::Runtime_controllable_data::Overridable_data<float_t>&
BT::anim_frame_action::Runtime_controllable_data
    ::get_float_data_handle(Controllable_data_label label)
{
    assert(get_data_type(label) == CTRL_DATA_TYPE_FLOAT);
    return data_floats.at(label);
}

BT::anim_frame_action::Runtime_controllable_data::Overridable_data<bool>&
BT::anim_frame_action::Runtime_controllable_data
    ::get_bool_data_handle(Controllable_data_label label)
{
    assert(get_data_type(label) == CTRL_DATA_TYPE_BOOL);
    return data_bools.at(label);
}

BT::anim_frame_action::Runtime_controllable_data::Rising_edge_event&
BT::anim_frame_action::Runtime_controllable_data
    ::get_reeve_data_handle(Controllable_data_label label)
{
    assert(get_data_type(label) == CTRL_DATA_TYPE_RISING_EDGE_EVENT);
    return data_reeves.at(label);
}

void BT::anim_frame_action::Runtime_controllable_data
    ::clear_all_data_overrides()
{   // Populate the overridable data labels.
    static auto s_get_overridable_data_labels_fn = []() {
        std::vector<Controllable_data_label> overridable_data_labels;

        auto const& str_labels{ get_all_str_labels() };
        for (auto const& str_label : str_labels)
        {
            auto data_label{ str_label_to_enum(str_label) };
            switch (get_data_type(data_label))
            {
                case CTRL_DATA_TYPE_FLOAT:
                case CTRL_DATA_TYPE_BOOL:
                    overridable_data_labels.emplace_back(data_label);
                    break;

                default:
                    // Ignore since not overridable label.
                    break;
            }
        }

        return overridable_data_labels;
    };

    static auto s_overridable_data_labels{ s_get_overridable_data_labels_fn() };

    // Clear the data labels.
    for (auto data_label : s_overridable_data_labels)
    {
        switch (get_data_type(data_label))
        {
            case CTRL_DATA_TYPE_FLOAT:
                get_float_data_handle(data_label)
                    .clear_overriding();
                break;

            case CTRL_DATA_TYPE_BOOL:
                get_bool_data_handle(data_label)
                    .clear_overriding();
                break;

            default:
                // @NOTE: Should not enter this branch bc of prior filtering.
                assert(false);
                break;
        }
    }
}

void BT::anim_frame_action::Runtime_controllable_data::map_animator_to_control_regions(
    Model_animator const& animator,
    Runtime_data_controls const& data_controls)
{
    anim_state_idx_to_timeline_idx_map.clear();

    auto const& animator_states{ animator.get_animator_states() };
    auto const& data_control_timelines{ data_controls.data.anim_frame_action_timelines };
    assert(animator_states.size() == data_control_timelines.size());

    anim_state_idx_to_timeline_idx_map.reserve(animator_states.size());
    for (size_t anim_state_idx = 0; anim_state_idx < animator_states.size(); anim_state_idx++)
        for (size_t timeline_idx = 0; timeline_idx < data_control_timelines.size(); timeline_idx++)
            if (animator_states[anim_state_idx].state_name ==
                data_control_timelines[timeline_idx].state_name)
            {   // Found a mapping!
                anim_state_idx_to_timeline_idx_map.emplace(anim_state_idx, timeline_idx);
                break;
            }

    assert(anim_state_idx_to_timeline_idx_map.size() == animator_states.size());
}

void BT::anim_frame_action::Runtime_controllable_data
    ::assign_hitcapsule_enabled_flags()
{
    static std::vector<Controllable_data_label> const s_all_hitcapsule_grp_data_labels{
        CTRL_DATA_LABEL_hitcapsule_group_0_enabled,
        CTRL_DATA_LABEL_hitcapsule_group_1_enabled,
        CTRL_DATA_LABEL_hitcapsule_group_2_enabled,
        CTRL_DATA_LABEL_hitcapsule_group_3_enabled,
        CTRL_DATA_LABEL_hitcapsule_group_4_enabled,
        CTRL_DATA_LABEL_hitcapsule_group_5_enabled,
        CTRL_DATA_LABEL_hitcapsule_group_6_enabled,
        CTRL_DATA_LABEL_hitcapsule_group_7_enabled,
        CTRL_DATA_LABEL_hitcapsule_group_8_enabled,
        CTRL_DATA_LABEL_hitcapsule_group_9_enabled,
    };

    size_t data_label_idx{ 0 };

    auto& hitcapsule_grps{ hitcapsule_group_set.get_hitcapsule_groups() };
    for (auto& hitcapsule_grp : hitcapsule_grps)
    {
        assert(data_label_idx < s_all_hitcapsule_grp_data_labels.size());

        // Use data handle to set hitcapsule group enabled flag.
        hitcapsule_grp.set_enabled(
            get_bool_data_handle(s_all_hitcapsule_grp_data_labels[data_label_idx]).get_val());

        data_label_idx++;
    }
}

void BT::anim_frame_action::Runtime_controllable_data::update_hitcapsule_transforms(
    mat4 base_transform,
    std::vector<mat4s> const& joint_matrices)
{
    auto& hitcapsule_grps{ hitcapsule_group_set.get_hitcapsule_groups() };
    for (auto& hitcapsule_grp : hitcapsule_grps)
    {
        auto& hitcapsules{ hitcapsule_grp.get_capsules() };
        for (auto& hitcapsule : hitcapsules)
        {   // Update hitcapsule to follow attached joint matrices.
            hitcapsule.update_transform(base_transform, joint_matrices);
        }
    }
}


// Data controls.

#if 0
// namespace
// {

// std::unordered_map<std::string, size_t> compile_anim_state_name_to_idx_map(
//     std::vector<BT::Animator_template::Animator_state> const& animator_states)
// {
//     std::unordered_map<std::string, size_t> rei_no_map;
//     size_t idx{ 0 };
//     for (auto& anim_state : animator_states)
//     {
//         rei_no_map.emplace(anim_state.state_name, idx);
//         idx++;
//     }
//     return rei_no_map;
// }

// }  // namespace
#endif  // 0

BT::anim_frame_action::Runtime_data_controls::Runtime_data_controls(std::string const& fname)
{
    // Deserialize json into data.
    data = Data(json_load_from_disk(fname));

    // Load model from bank.
    animated_model = Model_bank::get_model(data.animated_model_name);
    assert(animated_model != nullptr);

    // Ctrl items type calculation.
    calculate_all_ctrl_item_types();
}

// void BT::anim_frame_action::Runtime_data_controls::serialize(
//     Serialization_mode mode,
//     json& node_ref)
// {
//     if (mode == SERIAL_MODE_DESERIALIZE)
//     {   // Load model from bank.
//         std::string model_name{ node_ref["animated_model_name"] };
//         model = Model_bank::get_model(model_name);
//         assert(model != nullptr);

//         // Control items.
//         control_items.clear();
//         auto& nr_control_items{ node_ref["control_items"] };
//         if (!nr_control_items.is_null() && nr_control_items.is_array())
//         {
//             control_items.reserve(nr_control_items.size());
//             for (size_t i = 0; i < nr_control_items.size(); i++)
//             {   // Only include name since type calc happens later.
//                 control_items.emplace_back(nr_control_items[i]["name"]);
//             }

//             // Ctrl items type calculation.
//             calculate_all_ctrl_item_types();
//         }

//         // Animations (use anim state names as key).
//         anim_frame_action_timelines.clear();
//         auto& nr_anims{ node_ref["anim_frame_action_timelines"] };
//         if (!nr_anims.is_null() && nr_anims.is_array())
//         {
//             auto anim_state_name_to_idx_map{
//                 compile_anim_state_name_to_idx_map(service_finder
//                                                    ::find_service<Animator_template_bank>()
//                                                    .load_animator_template(model_name + ".btanitor")
//                                                    .animator_states) };

//             anim_frame_action_timelines.resize(anim_state_name_to_idx_map.size());
//             for (auto& nr_anim_entry : nr_anims)
//             {   // Animations level.
//                 size_t anim_state_idx{ anim_state_name_to_idx_map.at(nr_anim_entry["state_name"]) };
//                 auto& nr_anim_regions{ nr_anim_entry["regions"] };
//                 if (!nr_anim_regions.is_null() && nr_anim_regions.is_array())
//                 {   // Insert anim action regions.
//                     anim_frame_action_timelines[anim_state_idx]
//                         .regions.reserve(nr_anim_regions.size());
//                     for (auto& nr_region : nr_anim_regions)
//                         anim_frame_action_timelines[anim_state_idx].regions
//                             .emplace_back(nr_region["ctrl_item_idx"].get<uint32_t>(),
//                                           nr_region["start_frame"].get<int32_t>(),
//                                           nr_region["end_frame"].get<int32_t>());
//                 }
//             }
//         }
//     }
//     else if (mode == SERIAL_MODE_SERIALIZE)
//     {   // Save model from bank.
//         std::string model_name{ Model_bank::get_model_name(model) };
//         node_ref["animated_model_name"] = model_name;

//         // Control items.
//         auto& nr_control_items{ node_ref["control_items"] };
//         for (size_t i = 0; i < control_items.size(); i++)
//         {   // Only save name of control item bc of type calc.
//             nr_control_items[i]["name"] = control_items[i].name;
//         }

//         // Animations.
//         auto& anim_states{ service_finder::find_service<Animator_template_bank>()
//                            .load_animator_template(model_name + ".btanitor")
//                            .animator_states };

//         auto& nr_anims{ node_ref["anim_frame_action_timelines"] };
//         nr_anims = json::array();
//         for (size_t anim_state_idx = 0;
//              anim_state_idx < anim_frame_action_timelines.size();
//              anim_state_idx++)
//         {   // Animations level.
//             json nr_anim_entry = {};
//             nr_anim_entry["state_name"] = anim_states[anim_state_idx].state_name;
//             auto& nr_anim_regions{ nr_anim_entry["regions"] };
//             nr_anim_regions = json::array();

//             for (size_t reg_idx = 0;
//                  reg_idx < anim_frame_action_timelines[anim_state_idx].regions.size();
//                  reg_idx++)
//             {   // Insert anim action regions.
//                 auto const& region{ anim_frame_action_timelines[anim_state_idx].regions[reg_idx] };
//                 json nr_region = {};
//                 nr_region["ctrl_item_idx"] = region.ctrl_item_idx;
//                 nr_region["start_frame"]   = region.start_frame;
//                 nr_region["end_frame"]     = region.end_frame;
//                 nr_anim_regions.emplace_back(nr_region);
//             }

//             nr_anims.emplace_back(nr_anim_entry);
//         }
//     }

//     hitcapsule_group_set_template.scene_serialize(static_cast<Scene_serialization_mode>(mode),
//                                                   node_ref["hitcapsule_group_set"]);
// }

void BT::anim_frame_action::Runtime_data_controls::calculate_all_ctrl_item_types()
{
    for (auto& ctrl_item : data.control_items)
    {
        std::vector<std::string> tokens;
        {   // Get tokens in ctrl item name.
            std::stringstream ss(ctrl_item.name);
            std::string token;
            while (std::getline(ss, token, ':'))
            {
                tokens.emplace_back(token);
            }
        }

        {   // Get data label.
            auto& data_label_str{ tokens[0] };
            ctrl_item.affecting_data_label =
                anim_frame_action::Runtime_controllable_data::str_label_to_enum(data_label_str);
        }
        {   // Get control type.
            auto& ctrl_type_str{ tokens[1] };
            if (ctrl_type_str == "wr")
                ctrl_item.type = anim_frame_action::CTRL_ITEM_TYPE_DATA_WRITE;
            else if (ctrl_type_str == "ov")
                ctrl_item.type = anim_frame_action::CTRL_ITEM_TYPE_DATA_OVERRIDE;
            else if (ctrl_type_str == "tr")
                ctrl_item.type = anim_frame_action::CTRL_ITEM_TYPE_EVENT_TRIGGER;
            else
            {   // @TODO: Implement this new type of ctrl type.
                assert(false);
            }
        }
        {   // Get data point(s).
            static auto s_read_data_pt_fn = [](anim_frame_action::Controllable_data_label data_label,
                                               std::string const& str_token,
                                               uint32_t& out_data_point) {
                // Deduce how to read string token from data label data type.
                switch (anim_frame_action::Runtime_controllable_data::get_data_type(data_label))
                {
                    case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_FLOAT:
                    {
                        float_t token_as_float = std::stof(str_token);

                        static_assert(sizeof(out_data_point) >= sizeof(token_as_float));
                        *reinterpret_cast<float_t*>(&out_data_point) = token_as_float;
                        break;
                    }

                    case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_BOOL:
                    {
                        bool token_as_bool{ str_token == "true" ? true : false };

                        static_assert(sizeof(out_data_point) >= sizeof(token_as_bool));
                        *reinterpret_cast<bool*>(&out_data_point) = token_as_bool;
                        break;
                    }

                    default:
                        // Unsupported data type.
                        assert(false);
                        out_data_point = 0;
                        break;
                }
            };
            switch (ctrl_item.type)
            {
                case anim_frame_action::CTRL_ITEM_TYPE_DATA_WRITE:
                case anim_frame_action::CTRL_ITEM_TYPE_DATA_OVERRIDE:
                    // Read one data point.
                    s_read_data_pt_fn(ctrl_item.affecting_data_label,
                                      tokens[2],
                                      ctrl_item.data_point0);
                    break;

                case anim_frame_action::CTRL_ITEM_TYPE_EVENT_TRIGGER:
                    // Do nothing.
                    break;

                default:
                    // @TODO: Implement this new type of ctrl type.
                    assert(false);
                    break;
            }
        }
    }
}

// Bank of data controls.
void BT::anim_frame_action::Bank::emplace(std::string const& name,
                                          Runtime_data_controls&& runtime_state)
{
    s_runtime_states.emplace(name, std::move(runtime_state));
}

void BT::anim_frame_action::Bank::replace(std::string const& name,
                                          Runtime_data_controls&& runtime_state)
{
    s_runtime_states.at(name) = std::move(runtime_state);
}

BT::anim_frame_action::Runtime_data_controls const&
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
