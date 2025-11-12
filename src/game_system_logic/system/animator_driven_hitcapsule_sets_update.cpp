#include "animator_driven_hitcapsule_sets_update.h"

#include "btlogger.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/animator_driven_hitcapsule_set.h"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"


void BT::system::animator_driven_hitcapsule_sets_update()
{
    auto view{ service_finder::find_service<Entity_container>()
                   .get_ecs_registry()
                   .view<component::Animator_driven_hitcapsule_set const,
                         component::Transform const,
                         component::Created_render_object_reference const>() };
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };

    // Work with tagged entities.
    for (auto entity : view)
    {
        auto const& rend_obj_ref{ view.get<component::Created_render_object_reference const>(
            entity) };
        auto& rend_obj{
            *rend_obj_pool.checkout_render_obj_by_key({ rend_obj_ref.render_obj_uuid_ref }).front()
        };

        // Update whether capsules are enabled and keep capsules attached to connecting bone in
        // animator.
        auto& animator{ *rend_obj.get_model_animator() };
        animator.get_anim_frame_action_data_handle().assign_hitcapsule_enabled_flags();

        // @TODO: @FIXME: If this could be physics thread dependent instead of having to depend on
        // the timing of the render thread, then that would be wonderful. It would be sooo much more
        // consistent  -Thea 2025/10/07
        std::vector<mat4s> joint_matrices;
        animator.get_anim_floored_frame_pose(joint_matrices);

        animator.get_anim_frame_action_data_handle().update_hitcapsule_transforms(
            rend_obj.render_transform(),
            joint_matrices);

        rend_obj_pool.return_render_objs({ &rend_obj });
    }
}
