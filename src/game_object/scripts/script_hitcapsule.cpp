#include "logger.h"
#include "scripts.h"

// #include "../renderer/renderer.h"
// #include "../renderer/mesh.h"
// #include "../renderer/model_animator.h"
// #include "../animation_frame_action_tool/editor_state.h"
#include "../hitbox_interactor/hitcapsule.h"
#include "../service_finder/service_finder.h"
#include "../uuid/uuid.h"
#include "cglm/types-struct.h"
// #include <memory>
#include <vector>

namespace BT
{
class Input_handler;
class Physics_engine;
class Game_object_pool;
}


namespace BT::Scripts
{

class Script_hitcapsule : public Script_ifc
{
public:
    Script_hitcapsule(json const& node_ref)
    {
        deserialize_into_hitcapsule_groups(node_ref);

        // Add hitcapsule groups to solver service.
        auto& hitcapsule_solver{
            service_finder::find_service<Hitcapsule_group_overlap_solver>() };

        m_hitcapsule_grp_uuids.reserve(m_hitcapsule_grps.size());
        for (auto const& group : m_hitcapsule_grps)
        {
            UUID group_uuid{ hitcapsule_solver.add_group(group) };
            m_hitcapsule_grp_uuids.emplace_back(group_uuid);
        }
    }

    ~Script_hitcapsule()
    {   // Remove hitcapsule groups to solver service.
        auto& hitcapsule_solver{
            service_finder::find_service<Hitcapsule_group_overlap_solver>() };

        for (auto group_uuid : m_hitcapsule_grp_uuids)
        {
            hitcapsule_solver.remove_group(group_uuid);
        }
    }

    void deserialize_into_hitcapsule_groups(json const& node_ref);

    // Script_ifc.
    Script_type get_type() override { return SCRIPT_TYPE_hitcapsule; }
    void serialize_datas(json& node_ref) override;

    // Script data.
    std::vector<Hitcapsule_group> m_hitcapsule_grps;
    std::vector<UUID> m_hitcapsule_grp_uuids;

    void on_pre_physics(float_t physics_delta_time) override
    {   // Attach capsules to connecting bone in animator.
        assert(false);  // @TODO: IMPLEMENT!
    }
};

}  // namespace BT

void BT::Scripts::Script_hitcapsule::deserialize_into_hitcapsule_groups(json const& node_ref)
{   // Access list of hitcapsule groups.
    m_hitcapsule_grps.reserve(node_ref["hitcapsule_groups"].size());
    for (auto& capsule_group_json : node_ref["hitcapsule_groups"])
    {   // Deserialize capsules.
        std::vector<Hitcapsule> capsules;
        capsules.reserve(capsule_group_json["capsules"].size());
        for (auto& capsule_json : capsule_group_json["capsules"])
        {
            capsules.emplace_back(vec3s{ capsule_json["origin_a"][0].get<float_t>(),
                                         capsule_json["origin_a"][1].get<float_t>(),
                                         capsule_json["origin_a"][2].get<float_t>() },
                                  vec3s{ capsule_json["origin_b"][0].get<float_t>(),
                                         capsule_json["origin_b"][1].get<float_t>(),
                                         capsule_json["origin_b"][2].get<float_t>() },
                                  capsule_json["radius"].get<float_t>(),
                                  capsule_json["connecting_bone_name"].get<std::string>(),
                                  capsule_json["connecting_bone_name_2"].get<std::string>());
        }

        m_hitcapsule_grps.emplace_back(capsule_group_json["enabled"].get<bool>(),
                                       capsule_group_json["type"].get<Hitcapsule_group::Type>(),
                                       std::move(capsules));
    }
}

void BT::Scripts::Script_hitcapsule::serialize_datas(json& node_ref)
{   // Access list of hitcapsule groups.
    for (auto& capsule_group : m_hitcapsule_grps)
    {   // Serialize capsules.
        json capsule_group_json = {};
        for (auto& capsule : capsule_group.get_capsules())
        {
            json capsule_json = {};
            capsule_json["origin_a"][0]            = capsule.origin_a.x;
            capsule_json["origin_a"][1]            = capsule.origin_a.y;
            capsule_json["origin_a"][2]            = capsule.origin_a.z;
            capsule_json["origin_b"][0]            = capsule.origin_b.x;
            capsule_json["origin_b"][1]            = capsule.origin_b.y;
            capsule_json["origin_b"][2]            = capsule.origin_b.z;
            capsule_json["radius"]                 = capsule.radius;
            capsule_json["connecting_bone_name"]   = capsule.connecting_bone_name;
            capsule_json["connecting_bone_name_2"] = capsule.connecting_bone_name_2;

            capsule_group_json["capsules"].push_back(capsule_json);
        }

        capsule_group_json["enabled"] = capsule_group.is_enabled();
        capsule_group_json["type"] = capsule_group.get_type();

        node_ref["hitcapsule_groups"].push_back(capsule_group_json);
    }
}

// Create script.
std::unique_ptr<BT::Scripts::Script_ifc>
BT::Scripts::Factory_impl_funcs
    ::create_script_hitcapsule_from_serialized_datas(
        Input_handler*    input_handler,
        Physics_engine*   phys_engine,
        Renderer*         renderer,
        Game_object_pool* game_obj_pool,
        json const&       node_ref)
{
    return std::unique_ptr<Script_ifc>(
        new Script_hitcapsule{ node_ref });
}


// // Script_hitcapsule.
// void BT::Scripts::Script_hitcapsule::on_pre_render(float_t delta_time)
// {
//     auto& rend_obj{ *m_renderer.get_render_object_pool()
//                     .checkout_render_obj_by_key({ m_render_obj_key }).front() };
    
//     // Update self of any changes.
//     if (m_prev_working_model != anim_frame_action::s_editor_state.working_model)
//     {
//         anim_frame_action::s_editor_state.working_model_animator = nullptr;
//         m_prev_working_timeline_copy = nullptr;  // To ensure working timeline and animators get realigned.

//         // Check if new model is deformable by decorating an animator.
//         auto const& new_model{ *anim_frame_action::s_editor_state.working_model };
//         auto animator{ std::make_unique<Model_animator>(new_model) };

//         auto num_anims{ animator->get_num_model_animations() };

//         anim_frame_action::s_editor_state.anim_name_to_idx_map.clear();
//         for (size_t i = 0; i < num_anims; i++)
//         {   // Fill in animation map.
//             anim_frame_action::s_editor_state.anim_name_to_idx_map
//                 .emplace(animator->get_model_animation_by_idx(i).get_name(), i);
//         }

//         if (num_anims > 0)
//         {   // Create deformed model with animator.
//             rend_obj.set_deformed_model(std::make_unique<Deformed_model>(new_model));

//             // Force animator to get rebuilt immediately.
//             m_working_anim_idx = (uint32_t)-1;
//             anim_frame_action::s_editor_state.selected_anim_idx = 0;  // @HARDCODE: First anim.

//             // Assign a temp configuration and finish deformed model rend obj.
//             animator->configure_animator({}, nullptr);
//             anim_frame_action::s_editor_state.working_model_animator = animator.get();
//             rend_obj.set_model_animator(std::move(animator));
//         }
//         else
//         {   // Add non-deformed model and print warning.
//             logger::printe(logger::WARN, "New animation editor working model is non-deformable.");
//             rend_obj.set_model(&new_model);
//         }

//         m_prev_working_model = anim_frame_action::s_editor_state.working_model;
//     }

//     bool reconfigure_animator_needed{ false };

//     if (m_working_anim_idx != anim_frame_action::s_editor_state.selected_anim_idx)
//     {   // Reset prev anim frame.
//         m_prev_anim_frame = (size_t)-1;

//         // Configure deformed model animator to new anim idx.
//         m_working_anim_idx = anim_frame_action::s_editor_state.selected_anim_idx;
//         reconfigure_animator_needed = true;
//     }

//     if (anim_frame_action::s_editor_state.working_model_animator)
//     {
//         if (reconfigure_animator_needed ||
//             m_prev_working_timeline_copy != anim_frame_action::s_editor_state.working_timeline_copy)
//         {   // Reconfigure animator.
//             assert(anim_frame_action::s_editor_state.working_timeline_copy != nullptr);

//             anim_frame_action::s_editor_state.working_model_animator
//                 ->configure_animator({ { m_working_anim_idx,
//                                          0.0f,
//                                          false } },
//                                      anim_frame_action::s_editor_state.working_timeline_copy);
//             anim_frame_action::s_editor_state.selected_anim_num_frames =
//                 anim_frame_action::s_editor_state.working_model_animator
//                 ->get_model_animation_by_idx(m_working_anim_idx)
//                 .get_num_frames();

//             m_prev_working_timeline_copy = anim_frame_action::s_editor_state.working_timeline_copy;
//         }
        
//         // Update animator frame.
//         auto current_frame_clamped{ anim_frame_action::s_editor_state.anim_current_frame };  // @NOTE: Assumed clamped.
//         anim_frame_action::s_editor_state.working_model_animator
//             ->set_time(current_frame_clamped
//                        / Model_joint_animation::k_frames_per_second);

//         // Process all controllable datas.
//         // @TODO: Get all of them in here doing meaningful things.
//         //        @NOTE: Currently, it's just checking for event triggers.
//         static std::vector<anim_frame_action::Controllable_data_label> s_all_data_labels;
//         if (s_all_data_labels.empty())
//         {   // Add in data labels.
//             auto const& all_controllable_data_strs{ anim_frame_action::Runtime_controllable_data::get_all_str_labels() };
//             for (auto& data_str : all_controllable_data_strs)
//             {
//                 auto data_label{ anim_frame_action::Runtime_controllable_data::str_label_to_enum(data_str) };
//                 s_all_data_labels.emplace_back(data_label);
//             }
//         }

//         for (auto label : s_all_data_labels)
//         {
//             switch (anim_frame_action::Runtime_controllable_data::get_data_type(label))
//             {
//                 case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_FLOAT:
//                     break;

//                 case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_BOOL:
//                     break;

//                 case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_RISING_EDGE_EVENT:
//                     // Activate any events.
//                     (void)anim_frame_action::s_editor_state.working_model_animator
//                         ->get_anim_frame_action_data_handle()
//                         .get_reeve_data_handle(label)
//                         .check_if_rising_edge_occurred();
//                     break;

//                 default:
//                     break;
//             }
//         }
//     }

//     m_renderer.get_render_object_pool().return_render_objs({ &rend_obj });
// }
