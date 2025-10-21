// clang-format off
#include "concrete_systems.h"
// clang-format on

#include "../../animation_frame_action_tool/editor_state.h"
#include "../../renderer/animator_template.h"
#include "../../renderer/mesh.h"
#include "../../renderer/model_animator.h"
#include "../../renderer/renderer.h"
#include "../../service_finder/service_finder.h"
#include "../component_registry.h"
#include "../components.h"
#include "../game_object.h"
#include "btlogger.h"
#include "system_ifc.h"

#include <memory>



namespace
{
enum : int32_t
{
    Q_IDX_COMP_LISTS_WITH_AFA_EDITOR_COMM,
};
}  // namespace


BT::component_system::system::System__dev__anim_editor_tool_state_agent::
    System__dev__anim_editor_tool_state_agent()
    : System_ifc({
          Component_list_query::compile_query_string(
              "(Component_anim_editor_tool_communicator_state)"),
      })
{   // @NOTE: Do not remove adding concrete class to service finder!!
    BT_SERVICE_FINDER_ADD_SERVICE(System__dev__anim_editor_tool_state_agent, this);
}

void BT::component_system::system::System__dev__anim_editor_tool_state_agent::invoke_system_inner(
    Component_lists_per_query&& comp_lists_per_query) const /*override*/
{
    assert(comp_lists_per_query[Q_IDX_COMP_LISTS_WITH_AFA_EDITOR_COMM].size() <= 1);

    for (auto comp_list : comp_lists_per_query[Q_IDX_COMP_LISTS_WITH_AFA_EDITOR_COMM])
    {   // Get component handles.
        auto& comp_afa_comm_state{
            comp_list->get_component_handle<Component_anim_editor_tool_communicator_state>()
        };

        auto& rend_obj{ *comp_afa_comm_state.rend_obj };
        // auto& rend_obj{ *m_renderer.get_render_object_pool()
        //                 .checkout_render_obj_by_key({ m_render_obj_key }).front() };

        // Update self of any changes.
        if (comp_afa_comm_state.prev_working_model != anim_frame_action::s_editor_state.working_model)
        {
            anim_frame_action::s_editor_state.working_model_animator = nullptr;
            comp_afa_comm_state.prev_working_timeline_copy = nullptr;  // To ensure working timeline and animators get realigned.

            // Check if new model is deformable by whether it has animations.
            auto const& new_model{ *anim_frame_action::s_editor_state.working_model };

            if (!new_model.get_joint_animations().empty())
            {   // Create deformed model with animator.
                rend_obj.set_deformed_model(std::make_unique<Deformed_model>(new_model));

                // Force animator to get rebuilt immediately.
                comp_afa_comm_state.working_anim_state_idx = (uint32_t)-1;
                anim_frame_action::s_editor_state.selected_anim_state_idx = 0;  // @HARDCODE: First anim.
            }
            else
            {   // Add non-deformed model and print warning.
                logger::printe(logger::WARN, "New animation editor working model is non-deformable.");
                rend_obj.set_model(&new_model);
            }

            comp_afa_comm_state.prev_working_model = anim_frame_action::s_editor_state.working_model;
        }

        bool reconfigure_animator_needed{ false };

        if (comp_afa_comm_state.working_anim_state_idx != anim_frame_action::s_editor_state.selected_anim_state_idx)
        {   // Reset prev anim frame.
            comp_afa_comm_state.prev_anim_frame = (size_t)-1;

            // Configure deformed model animator to new anim idx.
            comp_afa_comm_state.working_anim_state_idx = anim_frame_action::s_editor_state.selected_anim_state_idx;
            reconfigure_animator_needed = true;
        }

        if (anim_frame_action::s_editor_state.working_model_animator &&
            comp_afa_comm_state.prev_working_timeline_copy != anim_frame_action::s_editor_state.working_timeline_copy)
        {   // Configure animator to use new timeline.
            reconfigure_animator_needed = true;
        }

        if (reconfigure_animator_needed)
        {
            assert(anim_frame_action::s_editor_state.working_model != nullptr);
            assert(anim_frame_action::s_editor_state.working_timeline_copy != nullptr);

            {   // Create new animator.
                auto animator{ std::make_unique<Model_animator>(*anim_frame_action::s_editor_state.working_model) };

                service_finder::find_service<Animator_template_bank>().load_animator_template_into_animator(
                    *animator, anim_frame_action::s_editor_state.working_model->get_model_name() + ".btanitor");

                // Attach animator to render obj.
                anim_frame_action::s_editor_state.working_model_animator = animator.get();
                rend_obj.set_model_animator(std::move(animator));

                // @TODO: @NOCHECKIN: @THEA: Create the functionality vvbelowvv but as
                // systems/components.
                assert(false);

                // // Create hitcapsule set driver script onto gameobject.
                // static auto s_script_type_str{
                //     Scripts::Helper_funcs::get_script_name_from_type(
                //         SCRIPT_TYPE_animator_driven_hitcapsule_set) };

                // static auto s_script_creation_fn = [](UUID render_obj_key) {
                //     json node_ref = {};
                //     node_ref["script_type"] = s_script_type_str;
                //     node_ref["script_datas"]["render_obj"] =
                //         UUID_helper::to_pretty_repr(render_obj_key);

                //     return node_ref;
                // };

                // auto& game_obj_pool{ service_finder::find_service<Game_object_pool>() };

                // auto& game_obj{ *game_obj_pool.get_one_no_lock(m_game_obj_key) };

                // game_obj.remove_script(s_script_type_str);

                // auto creation_json = s_script_creation_fn(m_render_obj_key);
                // game_obj.add_script(creation_json);
            }

            // Set animator state.
            anim_frame_action::s_editor_state.working_model_animator->change_state_idx(
                comp_afa_comm_state.working_anim_state_idx);

            // Fill in animator state name to idx map.
            auto& anim_states{
                anim_frame_action::s_editor_state.working_model_animator->get_animator_states() };

            anim_frame_action::s_editor_state.anim_state_name_to_idx_map.clear();
            for (size_t i = 0; i < anim_states.size(); i++)
            {   // Fill in animation state map.
                anim_frame_action::s_editor_state.anim_state_name_to_idx_map
                    .emplace(anim_states[i].state_name, i);
            }

            // Reconfigure animator.
            anim_frame_action::s_editor_state.working_model_animator
                ->configure_anim_frame_action_controls(
                    anim_frame_action::s_editor_state.working_timeline_copy);

            auto anim_state_anim_idx{
                anim_frame_action::s_editor_state.working_model_animator
                ->get_animator_state(comp_afa_comm_state.working_anim_state_idx)
                .animation_idx };

            anim_frame_action::s_editor_state.selected_anim_num_frames =
                anim_frame_action::s_editor_state.working_model_animator
                ->get_model_animation(anim_state_anim_idx)
                .get_num_frames();

            comp_afa_comm_state.prev_working_timeline_copy =
                anim_frame_action::s_editor_state.working_timeline_copy;
        }

        if (anim_frame_action::s_editor_state.working_model_animator)
        {   // Update animator frame.
            auto current_frame_clamped{ anim_frame_action::s_editor_state.anim_current_frame };  // @NOTE: Assumed clamped.
            anim_frame_action::s_editor_state.working_model_animator
                ->set_time(current_frame_clamped
                        / Model_joint_animation::k_frames_per_second);

            // Process all controllable datas.
            // @TODO: Get all of them in here doing meaningful things.
            //        @NOTE: Currently, it's just checking for event triggers.
            static std::vector<anim_frame_action::Controllable_data_label> s_all_data_labels;
            if (s_all_data_labels.empty())
            {   // Add in data labels.
                auto const& all_controllable_data_strs{ anim_frame_action::Runtime_controllable_data::get_all_str_labels() };
                for (auto& data_str : all_controllable_data_strs)
                {
                    auto data_label{ anim_frame_action::Runtime_controllable_data::str_label_to_enum(data_str) };
                    s_all_data_labels.emplace_back(data_label);
                }
            }

            for (auto label : s_all_data_labels)
            {
                switch (anim_frame_action::Runtime_controllable_data::get_data_type(label))
                {
                    case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_FLOAT:
                        break;

                    case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_BOOL:
                        break;

                    case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_RISING_EDGE_EVENT:
                        // Activate any events.
                        // @TODO: This should be in its own script for processing each event
                        //   individually. E.g.: If there's an sfx script it will watch the reeve data
                        //   handle for the sfx and check `check_if_rising_edge_occurred()` for it.
                        (void)anim_frame_action::s_editor_state.working_model_animator
                            ->get_anim_frame_action_data_handle()
                            .get_reeve_data_handle(label)
                            .check_if_rising_edge_occurred();
                        break;

                    default:
                        break;
                }
            }
        }

        // m_renderer.get_render_object_pool().return_render_objs({ &rend_obj });
    }
}






#if 0
#include "btlogger.h"
#include "scripts.h"

#include "../animation_frame_action_tool/editor_state.h"
#include "../game_object/game_object.h"
#include "../game_object/scripts/scripts.h"
#include "../renderer/animator_template.h"
#include "../renderer/mesh.h"
#include "../renderer/model_animator.h"
#include "../renderer/renderer.h"
#include "../service_finder/service_finder.h"
#include <memory>

namespace BT
{
class Input_handler;
class Physics_engine;
class Game_object_pool;
}


namespace BT::Scripts
{

class Script__dev__anim_editor_tool_state_agent : public Script_ifc
{
public:
    Script__dev__anim_editor_tool_state_agent(Renderer& renderer,
                                              UUID game_obj_key,
                                              UUID render_obj_key)
        : m_renderer{ renderer }
        , m_game_obj_key{ game_obj_key }
        , m_render_obj_key{ render_obj_key }
    {
    }

    // Script_ifc.
    Script_type get_type() override
    {
        return SCRIPT_TYPE__dev__anim_editor_tool_state_agent;
    }

    void serialize_datas(json& node_ref) override
    {
        node_ref["game_obj_key"] = UUID_helper::to_pretty_repr(m_game_obj_key);
        node_ref["render_obj_key"] = UUID_helper::to_pretty_repr(m_render_obj_key);
    }

    void on_pre_render(float_t delta_time) override;

    Renderer& m_renderer;
    UUID m_game_obj_key;
    UUID m_render_obj_key;

    Model const* m_prev_working_model{ nullptr };
    uint32_t m_working_anim_state_idx{ (uint32_t)-1 };
    size_t m_prev_anim_frame{ (size_t)-1 };

    anim_frame_action::Runtime_data_controls const* m_prev_working_timeline_copy{ nullptr };
};

}  // namespace BT


// Create script.
std::unique_ptr<BT::Scripts::Script_ifc>
BT::Scripts::Factory_impl_funcs
    ::create_script__dev__anim_editor_tool_state_agent_from_serialized_datas(
        Input_handler* input_handler,
        Physics_engine* phys_engine,
        Renderer* renderer,
        Game_object_pool* game_obj_pool,
        json const& node_ref)
{
    return std::unique_ptr<Script_ifc>(
        new Script__dev__anim_editor_tool_state_agent{ *renderer,
                                                       UUID_helper::to_UUID(node_ref["game_obj_key"]),
                                                       UUID_helper::to_UUID(node_ref["render_obj_key"]) });
}


// Script__dev__anim_editor_tool_state_agent.
void BT::Scripts::Script__dev__anim_editor_tool_state_agent::on_pre_render(float_t delta_time)
{
    auto& rend_obj{ *m_renderer.get_render_object_pool()
                    .checkout_render_obj_by_key({ m_render_obj_key }).front() };

    // Update self of any changes.
    if (m_prev_working_model != anim_frame_action::s_editor_state.working_model)
    {
        anim_frame_action::s_editor_state.working_model_animator = nullptr;
        m_prev_working_timeline_copy = nullptr;  // To ensure working timeline and animators get realigned.

        // Check if new model is deformable by whether it has animations.
        auto const& new_model{ *anim_frame_action::s_editor_state.working_model };

        if (!new_model.get_joint_animations().empty())
        {   // Create deformed model with animator.
            rend_obj.set_deformed_model(std::make_unique<Deformed_model>(new_model));

            // Force animator to get rebuilt immediately.
            m_working_anim_state_idx = (uint32_t)-1;
            anim_frame_action::s_editor_state.selected_anim_state_idx = 0;  // @HARDCODE: First anim.
        }
        else
        {   // Add non-deformed model and print warning.
            logger::printe(logger::WARN, "New animation editor working model is non-deformable.");
            rend_obj.set_model(&new_model);
        }

        m_prev_working_model = anim_frame_action::s_editor_state.working_model;
    }

    bool reconfigure_animator_needed{ false };

    if (m_working_anim_state_idx != anim_frame_action::s_editor_state.selected_anim_state_idx)
    {   // Reset prev anim frame.
        m_prev_anim_frame = (size_t)-1;

        // Configure deformed model animator to new anim idx.
        m_working_anim_state_idx = anim_frame_action::s_editor_state.selected_anim_state_idx;
        reconfigure_animator_needed = true;
    }

    if (anim_frame_action::s_editor_state.working_model_animator &&
        m_prev_working_timeline_copy != anim_frame_action::s_editor_state.working_timeline_copy)
    {   // Configure animator to use new timeline.
        reconfigure_animator_needed = true;
    }

    if (reconfigure_animator_needed)
    {
        assert(anim_frame_action::s_editor_state.working_model != nullptr);
        assert(anim_frame_action::s_editor_state.working_timeline_copy != nullptr);

        {   // Create new animator.
            auto animator{ std::make_unique<Model_animator>(*anim_frame_action::s_editor_state.working_model) };

            service_finder::find_service<Animator_template_bank>().load_animator_template_into_animator(
                *animator, anim_frame_action::s_editor_state.working_model->get_model_name() + ".btanitor");

            // Attach animator to render obj.
            anim_frame_action::s_editor_state.working_model_animator = animator.get();
            rend_obj.set_model_animator(std::move(animator));

            // Create hitcapsule set driver script onto gameobject.
            static auto s_script_type_str{
                Scripts::Helper_funcs::get_script_name_from_type(
                    SCRIPT_TYPE_animator_driven_hitcapsule_set) };

            static auto s_script_creation_fn = [](UUID render_obj_key) {
                json node_ref = {};
                node_ref["script_type"] = s_script_type_str;
                node_ref["script_datas"]["render_obj"] =
                    UUID_helper::to_pretty_repr(render_obj_key);

                return node_ref;
            };

            auto& game_obj_pool{ service_finder::find_service<Game_object_pool>() };

            auto& game_obj{ *game_obj_pool.get_one_no_lock(m_game_obj_key) };

            game_obj.remove_script(s_script_type_str);

            auto creation_json = s_script_creation_fn(m_render_obj_key);
            game_obj.add_script(creation_json);
        }

        // Set animator state.
        anim_frame_action::s_editor_state.working_model_animator->change_state_idx(
            m_working_anim_state_idx);

        // Fill in animator state name to idx map.
        auto& anim_states{
            anim_frame_action::s_editor_state.working_model_animator->get_animator_states() };

        anim_frame_action::s_editor_state.anim_state_name_to_idx_map.clear();
        for (size_t i = 0; i < anim_states.size(); i++)
        {   // Fill in animation state map.
            anim_frame_action::s_editor_state.anim_state_name_to_idx_map
                .emplace(anim_states[i].state_name, i);
        }

        // Reconfigure animator.
        anim_frame_action::s_editor_state.working_model_animator
            ->configure_anim_frame_action_controls(
                anim_frame_action::s_editor_state.working_timeline_copy);

        auto anim_state_anim_idx{
            anim_frame_action::s_editor_state.working_model_animator
            ->get_animator_state(m_working_anim_state_idx)
            .animation_idx };

        anim_frame_action::s_editor_state.selected_anim_num_frames =
            anim_frame_action::s_editor_state.working_model_animator
            ->get_model_animation(anim_state_anim_idx)
            .get_num_frames();

        m_prev_working_timeline_copy = anim_frame_action::s_editor_state.working_timeline_copy;
    }

    if (anim_frame_action::s_editor_state.working_model_animator)
    {   // Update animator frame.
        auto current_frame_clamped{ anim_frame_action::s_editor_state.anim_current_frame };  // @NOTE: Assumed clamped.
        anim_frame_action::s_editor_state.working_model_animator
            ->set_time(current_frame_clamped
                       / Model_joint_animation::k_frames_per_second);

        // Process all controllable datas.
        // @TODO: Get all of them in here doing meaningful things.
        //        @NOTE: Currently, it's just checking for event triggers.
        static std::vector<anim_frame_action::Controllable_data_label> s_all_data_labels;
        if (s_all_data_labels.empty())
        {   // Add in data labels.
            auto const& all_controllable_data_strs{ anim_frame_action::Runtime_controllable_data::get_all_str_labels() };
            for (auto& data_str : all_controllable_data_strs)
            {
                auto data_label{ anim_frame_action::Runtime_controllable_data::str_label_to_enum(data_str) };
                s_all_data_labels.emplace_back(data_label);
            }
        }

        for (auto label : s_all_data_labels)
        {
            switch (anim_frame_action::Runtime_controllable_data::get_data_type(label))
            {
                case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_FLOAT:
                    break;

                case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_BOOL:
                    break;

                case anim_frame_action::Runtime_controllable_data::CTRL_DATA_TYPE_RISING_EDGE_EVENT:
                    // Activate any events.
                    // @TODO: This should be in its own script for processing each event
                    //   individually. E.g.: If there's an sfx script it will watch the reeve data
                    //   handle for the sfx and check `check_if_rising_edge_occurred()` for it.
                    (void)anim_frame_action::s_editor_state.working_model_animator
                        ->get_anim_frame_action_data_handle()
                        .get_reeve_data_handle(label)
                        .check_if_rising_edge_occurred();
                    break;

                default:
                    break;
            }
        }
    }

    m_renderer.get_render_object_pool().return_render_objs({ &rend_obj });
}
#endif  // 0
