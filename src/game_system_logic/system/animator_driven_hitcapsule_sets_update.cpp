#include "animator_driven_hitcapsule_sets_update.h"

#include "btdatecheck.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"
#include "txp_renderer_public.h"


void BT::system::animator_driven_hitcapsule_sets_update()
{
    date_deadline(2026, 9, 4);  // @CHECK: does this work?? needs the debug drawing.
    // @TODO: this needs to not use the render transform bc it's not updated yet. this needs to use the physics transform bc it was just updated.

    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto view{ reg.view<TXP::component::Animator_driven_hitcapsule_set const,
                        component::Transform const,
                        TXP::component::Render_object_config const>() };
    auto& renderer{ service_finder::find_service<TXP::Renderer>() };

    // Work with tagged entities.
    for (auto ent : view)
    {
        // Update whether capsules are enabled and keep capsules attached to connecting bone in
        // animator.
        auto animator{ renderer.try_get_skeletal_animator(ent).value() };

        animator.get_anim_frame_action_data_handle().assign_hitcapsule_enabled_flags();

        std::vector<mat4s> joint_matrices;
        animator.get_simulation_profile_frame_pose(animator.get_is_using_root_motion(),
                                                   joint_matrices);

        auto& render_transform{ const_cast<mat4s&>(
            view.get<TXP::component::Render_object_config const>(ent).transform) };

        animator.get_anim_frame_action_data_handle().update_hitcapsule_transforms(
            render_transform.raw,
            joint_matrices);
    }
}
