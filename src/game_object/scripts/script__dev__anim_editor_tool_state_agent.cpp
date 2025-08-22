#include "logger.h"
#include "scripts.h"

#include "../renderer/renderer.h"
#include "../renderer/mesh.h"
#include "../renderer/model_animator.h"
#include "../animation_editor_tool/animation_editor_tool.h"
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
    if (m_prev_working_model != anim_editor::s_editor_state.working_model)
    {   // Check if new model is deformable by decorating an animator.
        auto const& new_model{ *anim_editor::s_editor_state.working_model };
        auto animator{ std::make_unique<Model_animator>(new_model) };

        auto num_anims{ animator->get_num_model_animations() };
        anim_editor::s_editor_state.num_animations = num_anims;
        if (num_anims > 0)
        {   // Create deformed model with animator.
            rend_obj.set_deformed_model(std::make_unique<Deformed_model>(new_model));
            animator->configure_animator({ { 24 } });  // @NOCHECKIN: @HARDCODE.
            rend_obj.set_model_animator(std::move(animator));
        }
        else
        {   // Add non-deformed model and print warning.
            logger::printe(logger::WARN, "New animation editor working model is non-deformable.");
            rend_obj.set_model(&new_model);
        }

        m_prev_working_model = anim_editor::s_editor_state.working_model;
    }

    m_renderer.get_render_object_pool().return_render_objs({ &rend_obj });
}
