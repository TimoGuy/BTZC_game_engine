#include "write_render_transforms.h"

#include "btlogger.h"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/render_object_settings.h"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "renderer/renderer.h"
#include "service_finder/service_finder.h"
#include "uuid/uuid.h"


void BT::system::write_render_transforms()
{
    auto& reg{ service_finder::find_service<Entity_container>().get_ecs_registry() };
    auto& rend_obj_pool{ service_finder::find_service<Renderer>().get_render_object_pool() };

    // Write transforms.
    auto view{
        reg.view<component::Transform const, component::Created_render_object_reference const>()
    };
    for (auto entity : view)
    {
        auto const& transform{ view.get<component::Transform const>(entity) };
        auto const& rend_obj_ref{
            view.get<component::Created_render_object_reference const>(entity) };

        // @TODO: Include interpolation instead of just straight copying.

        auto rend_obj{
            rend_obj_pool.checkout_render_obj_by_key({ rend_obj_ref.render_obj_uuid_ref }).front()
        };

        // Calculate TRS into mat4 transform.
        auto rend_trans{ rend_obj->render_transform() };
        glm_translate_make(rend_trans,
                           vec3{ static_cast<float_t>(transform.position.x),
                                 static_cast<float_t>(transform.position.y),
                                 static_cast<float_t>(transform.position.z) });
        glm_quat_rotate(rend_trans, const_cast<float_t*>(transform.rotation.raw), rend_trans);
        glm_scale(rend_trans, const_cast<float_t*>(transform.scale.raw));

        rend_obj_pool.return_render_objs({ rend_obj });
    }
}
