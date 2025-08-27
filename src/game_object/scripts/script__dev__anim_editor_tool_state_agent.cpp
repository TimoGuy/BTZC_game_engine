#include "logger.h"
#include "scripts.h"

#include "../renderer/renderer.h"
#include "../renderer/mesh.h"
#include "../renderer/model_animator.h"
#include "../animation_frame_action_tool/editor_state.h"
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
    Script__dev__anim_editor_tool_state_agent(Renderer& renderer, UUID render_obj_key)
        : m_renderer{ renderer }
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
        node_ref["render_obj_key"] = UUID_helper::to_pretty_repr(m_render_obj_key);
    }

    void on_pre_render(float_t delta_time) override;

    Renderer& m_renderer;
    UUID m_render_obj_key;

    Model const* m_prev_working_model{ nullptr };
    Model_animator* m_working_model_animator{ nullptr };
    uint32_t m_working_anim_idx{ (uint32_t)-1 };
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
        m_working_model_animator = nullptr;

        // Check if new model is deformable by decorating an animator.
        auto const& new_model{ *anim_frame_action::s_editor_state.working_model };
        auto animator{ std::make_unique<Model_animator>(new_model) };

        auto num_anims{ animator->get_num_model_animations() };

        anim_frame_action::s_editor_state.anim_name_to_idx_map.clear();
        for (size_t i = 0; i < num_anims; i++)
        {   // Fill in animation map.
            anim_frame_action::s_editor_state.anim_name_to_idx_map
                .emplace(animator->get_model_animation_by_idx(i).get_name(), i);
        }

        if (num_anims > 0)
        {   // Create deformed model with animator.
            rend_obj.set_deformed_model(std::make_unique<Deformed_model>(new_model));

            // Force animator to get rebuilt immediately.
            m_working_anim_idx = (uint32_t)-1;
            anim_frame_action::s_editor_state.selected_anim_idx = 0;  // @HARDCODE: First anim.

            // Assign a temp configuration and finish deformed model rend obj.
            animator->configure_animator({}, nullptr);
            m_working_model_animator = animator.get();
            rend_obj.set_model_animator(std::move(animator));
        }
        else
        {   // Add non-deformed model and print warning.
            logger::printe(logger::WARN, "New animation editor working model is non-deformable.");
            rend_obj.set_model(&new_model);
        }

        m_prev_working_model = anim_frame_action::s_editor_state.working_model;
    }

    if (m_working_anim_idx != anim_frame_action::s_editor_state.selected_anim_idx)
    {   // Configure deformed model animator to new anim idx.
        m_working_anim_idx = anim_frame_action::s_editor_state.selected_anim_idx;
        if (m_working_model_animator)
        {
            m_working_model_animator->configure_animator({ { m_working_anim_idx,
                                                             0.0f,
                                                             false } },
                                                         nullptr);
            anim_frame_action::s_editor_state.selected_anim_num_frames =
                m_working_model_animator
                ->get_model_animation_by_idx(m_working_anim_idx)
                .get_num_frames();
        }
    }

    if (m_working_model_animator)
    {   // Update animator frame.
        auto current_frame_clamped{ anim_frame_action::s_editor_state.anim_current_frame };  // @NOTE: Assumed clamped.
        m_working_model_animator->set_time(current_frame_clamped
                                           / Model_joint_animation::k_frames_per_second);
    }

    m_renderer.get_render_object_pool().return_render_objs({ &rend_obj });
}
