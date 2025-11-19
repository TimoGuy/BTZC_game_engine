#include "propagate_changed_transforms.h"

#include "btglm.h"
#include "cglm/quat.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"


namespace
{

using namespace BT;

/// Appends transform, where `a` is parent transform, and `b` is child transform, however, with `a`
/// being inverted, so it's like `a^-1 * b`.
component::Transform append_transform_a_inv(component::Transform const& a,
                                            component::Transform const& b)
{
    component::Transform result;

    // Invert scale of `a`.
    vec3s a_scale_inv = GLM_VEC3_ZERO_INIT;
    if (!glm_eq(a.scale.x, 0))
        a_scale_inv.x = 1.0f / a.scale.x;
    if (!glm_eq(a.scale.y, 0))
        a_scale_inv.y = 1.0f / a.scale.y;
    if (!glm_eq(a.scale.z, 0))
        a_scale_inv.z = 1.0f / a.scale.z;

    // Scale.
    glm_vec3_mul(a_scale_inv.raw, const_cast<float_t*>(b.scale.raw), result.scale.raw);

    // Invert rotation.
    versors a_rot_inv;
    glm_quat_conjugate(const_cast<float_t*>(a.rotation.raw), a_rot_inv.raw);

    // Rotation.
    // @NOTE: Quats multiply in reverse.
    glm_quat_mul(const_cast<float_t*>(b.rotation.raw), a_rot_inv.raw, result.rotation.raw);
    glm_quat_normalize(result.rotation.raw);

    // Translation.
    // @NOTE: This is kinda the trickiest thing. Kinda undoing what `Translation` does in
    //        `append_transform()` ig???
    rvec3s a_tra_neg;
    btglm_rvec3_negate_to(a.position.raw, a_tra_neg.raw);

    btglm_rvec3_add(b.position.raw, a_tra_neg.raw, result.position.raw);
    btglm_quat_mul_rvec3(a_rot_inv.raw, result.position.raw, result.position.raw);
    btglm_rvec3_scale_v3(result.position.raw, a_scale_inv.raw, result.position.raw);

    return result;
}

/// Appends transform, where `a` is parent transform, and `b` is child transform.
component::Transform append_transform(component::Transform const& a, component::Transform const& b)
{
    // Reference: https://gabormakesgames.com/blog_transforms_transforms.html

    component::Transform result;

    // Scale.
    glm_vec3_mul(const_cast<float_t*>(a.scale.raw),
                 const_cast<float_t*>(b.scale.raw),
                 result.scale.raw);

    // Rotation.
    // @NOTE: Quats multiply in reverse.
    glm_quat_mul(const_cast<float_t*>(b.rotation.raw),
                 const_cast<float_t*>(a.rotation.raw),
                 result.rotation.raw);
    glm_quat_normalize(result.rotation.raw);

    // Translation.
    btglm_rvec3_scale_v3(b.position.raw, a.scale.raw, result.position.raw);
    btglm_quat_mul_rvec3(a.rotation.raw, result.position.raw, result.position.raw);
    btglm_rvec3_add(a.position.raw, result.position.raw, result.position.raw);

    return result;
}

void apply_delta_transform_recursive(auto& view,
                                     entt::registry& reg,
                                     Entity_container& entity_container,
                                     entt::entity entity,
                                     component::Transform const& prev_transform,
                                     component::Transform const& next_transform,
                                     bool directly_apply)
{
    auto& transform{ view.template get<component::Transform>(entity) };
    if (directly_apply)
    {
        transform = next_transform;
    }
    else
    {   // Apply inverse of `prev_transform` to get local transform, then apply `next_transform` to
        // get global transform.
        transform = append_transform_a_inv(prev_transform, transform);
        transform = append_transform(next_transform, transform);

        // Apply same transformation to the `Transform_changed` if there is one.
        auto trans_changed{ reg.try_get<component::Transform_changed>(entity) };
        if (trans_changed != nullptr)
        {
            auto& next_trans{ trans_changed->next_transform };
            next_trans = append_transform_a_inv(prev_transform, next_trans);
            next_trans = append_transform(next_transform, next_trans);
        }
    }

    // Search thru transform hierarchy children.
    auto const& transform_hierarchy{
        view.template get<component::Transform_hierarchy const>(entity) };
    for (auto child_entity : transform_hierarchy.children_entities)
    {
        apply_delta_transform_recursive(view,
                                        reg,
                                        entity_container,
                                        entity_container.find_entity(child_entity),
                                        prev_transform,
                                        next_transform,
                                        false);
    }
}

}  // namespace


void BT::system::propagate_changed_transforms()
{
    auto& entity_container{ service_finder::find_service<Entity_container>() };

    auto& reg{ entity_container.get_ecs_registry() };
    auto changed_trans_view = reg.view<component::Transform,
                                       component::Transform_hierarchy,
                                       component::Transform_changed>();
    auto trans_view = reg.view<component::Transform,
                               component::Transform_hierarchy>();

    // Propagate transforms with delta transforms.
    for (auto entity : changed_trans_view)
    {
        auto& transform{ changed_trans_view.get<component::Transform>(entity) };
        auto const& transform_changed{ changed_trans_view.get<component::Transform_changed>(
            entity) };
        auto const& transform_hierarchy{
            changed_trans_view.get<component::Transform_hierarchy const>(entity)
        };

        // Make copies for propagation.
        auto prev_trans_copy{ transform };
        auto next_trans_copy{ transform_changed.next_transform };

        // Propagate to children.
        apply_delta_transform_recursive(trans_view,
                                        reg,
                                        entity_container,
                                        entity,
                                        prev_trans_copy,
                                        next_trans_copy,
                                        true);
    }

    // Mark all transforms as propagated now (i.e. remove "changed" flag).
    reg.clear<component::Transform_changed>();
}
