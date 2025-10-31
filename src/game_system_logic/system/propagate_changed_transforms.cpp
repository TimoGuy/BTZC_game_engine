#include "propagate_changed_transforms.h"

#include "btglm.h"
#include "btlogger.h"
#include "timer/timer.h"
#include "cglm/quat.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/transform.h"
#include "game_system_logic/entity_container.h"
#include "service_finder/service_finder.h"


namespace
{

using namespace BT;

/// Inverses TRS transform.
component::Transform inverse_transform(component::Transform const& trans)
{
    component::Transform inv_trans;

#if 0  // @TODO: @NOCHECKIN: Delete this dead code.
    glm_quat_conjugate(const_cast<float_t*>(trans.rotation.raw), inv_trans.rotation.raw);

    mat3 conj_rot;
    glm_quat_mat3(inv_trans.rotation.raw, conj_rot);

    // @NOTE: Inverting the matrix requires that the position (which has rotation applied)
    //   is also undone, so instead of just inversing it needs to be multiplied by
    //   the conjugate of the rotation quat.  -Thea 2025/06/19

    {   // glm_mat3_mulv(conj_rot, position, result.position);
        auto& m{ conj_rot };
        auto& v{ trans.position.raw };
        rvec3 res;
        res[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2];
        res[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2];
        res[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2];
        {   // glm_vec3_copy(res, dest);
            auto& a{ res };
            auto& dest{ inv_trans.position.raw };
            dest[0] = a[0];
            dest[1] = a[1];
            dest[2] = a[2];
        }
    }

    {   // glm_vec3_inv(result.position);
        auto& v{ inv_trans.position.raw };
        v[0] = -v[0];
        v[1] = -v[1];
        v[2] = -v[2];
    }

    inv_trans.scale.raw[0] = 1.0f / trans.scale.raw[0];
    inv_trans.scale.raw[1] = 1.0f / trans.scale.raw[1];
    inv_trans.scale.raw[2] = 1.0f / trans.scale.raw[2];
#else

    // Reference: https://gabormakesgames.com/blog_transforms_transform_world.html

    // Rotation.
    // glm_quat_normalize_to(const_cast<float_t*>(trans.rotation.raw), inv_trans.rotation.raw);
    glm_quat_conjugate(const_cast<float_t*>(trans.rotation.raw), inv_trans.rotation.raw);
    glm_quat_normalize(inv_trans.rotation.raw);

    // Scale.
    glm_vec3_zero(inv_trans.scale.raw);
    if (!glm_eq(trans.scale.x, 0))
        inv_trans.scale.x = 1.0f / trans.scale.x;
    if (!glm_eq(trans.scale.y, 0))
        inv_trans.scale.y = 1.0f / trans.scale.y;
    if (!glm_eq(trans.scale.z, 0))
        inv_trans.scale.z = 1.0f / trans.scale.z;

    // Translation.
    if constexpr (true)
    {
        btglm_rvec3_negate_to(trans.position.raw, inv_trans.position.raw);
        btglm_rvec3_scale_v3(inv_trans.position.raw, inv_trans.scale.raw, inv_trans.position.raw);
        btglm_quat_mul_rvec3(inv_trans.rotation.raw, inv_trans.position.raw, inv_trans.position.raw);
    }
    else if constexpr (false)
    {   // My former method (see `game_object.cpp:calc_inverse()`)
        btglm_quat_mul_rvec3(inv_trans.rotation.raw, trans.position.raw, inv_trans.position.raw);
        btglm_rvec3_scale_v3(inv_trans.position.raw, inv_trans.scale.raw, inv_trans.position.raw);  // Maybe this???
        btglm_rvec3_negate_to(inv_trans.position.raw, inv_trans.position.raw);
    }
    else if constexpr (false)
    {   // Idk maybe just negate the pos and that's it???
        btglm_rvec3_negate_to(trans.position.raw, inv_trans.position.raw);
    }
    else if constexpr (false)
    {   // Maybe just do the same order as `append_transform()`???
        btglm_rvec3_scale_v3(trans.position.raw, inv_trans.scale.raw, inv_trans.position.raw);
        btglm_quat_mul_rvec3(inv_trans.rotation.raw, inv_trans.position.raw, inv_trans.position.raw);
        // btglm_rvec3_add(inv_trans.position.raw, inv_trans.position.raw, inv_trans.position.raw);
        btglm_rvec3_negate_to(inv_trans.position.raw, inv_trans.position.raw);
    }
    else
    {   // Maybe try the reverse of the method above?
        btglm_rvec3_negate_to(trans.position.raw, inv_trans.position.raw);
        btglm_quat_mul_rvec3(inv_trans.rotation.raw, inv_trans.position.raw, inv_trans.position.raw);
        btglm_rvec3_scale_v3(trans.position.raw, inv_trans.scale.raw, inv_trans.position.raw);
    }

#endif  // 0

    return inv_trans;
}

/// Appends transform, where `a` is parent transform, and `b` is child transform.
component::Transform append_transform(component::Transform const& a, component::Transform const& b)
{
    // @TODO: @NOCHECKIN: Delete dead code below.

#if 0
    #define GLM_RVEC3_SCALE_V3(a, v, dest)                                      \
    dest[0] = a[0] * v[0];                                                      \
    dest[1] = a[1] * v[1];                                                      \
    dest[2] = a[2] * v[2]

    #define GLM_MAT3_MUL_RVEC3(m, v, dest)                                      \
    do {                                                                        \
    rvec3 temp;                                                                 \
    temp[0] = m[0][0] * v[0] + m[1][0] * v[1] + m[2][0] * v[2];                 \
    temp[1] = m[0][1] * v[0] + m[1][1] * v[1] + m[2][1] * v[2];                 \
    temp[2] = m[0][2] * v[0] + m[1][2] * v[1] + m[2][2] * v[2];                 \
    dest[0] = temp[0];                                                          \
    dest[1] = temp[1];                                                          \
    dest[2] = temp[2];                                                          \
    } while(false)

    #define GLM_RVEC3_ADD(a, b, dest)                                           \
    dest[0] = a[0] + b[0];                                                      \
    dest[1] = a[1] + b[1];                                                      \
    dest[2] = a[2] + b[2]

    component::Transform result;

    mat3 my_rot_mat3;
    glm_quat_mat3(const_cast<float_t*>(a.rotation.raw), my_rot_mat3);

    GLM_RVEC3_SCALE_V3(b.position.raw, a.scale.raw, result.position.raw);
    GLM_MAT3_MUL_RVEC3(my_rot_mat3, result.position.raw, result.position.raw);
    GLM_RVEC3_ADD(a.position.raw, result.position.raw, result.position.raw);

    // @NOTE: Quaternions multiply in reverse.
    glm_quat_mul(const_cast<float_t*>(a.rotation.raw),
                 const_cast<float_t*>(b.rotation.raw),
                 result.rotation.raw);

    glm_vec3_mul(const_cast<float_t*>(b.scale.raw),
                 const_cast<float_t*>(a.scale.raw),
                 result.scale.raw);

    #undef GLM_RVEC3_SCALE_V3
    #undef GLM_MAT3_MUL_RVEC3
    #undef GLM_RVEC3_ADD
#else

    // Reference: https://gabormakesgames.com/blog_transforms_transforms.html

    component::Transform result;

    // Scale.
    glm_vec3_mul(const_cast<float_t*>(a.scale.raw),
                 const_cast<float*>(b.scale.raw),
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

#endif  // 0
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
        #define DIRECT_WAY 1
        #if DIRECT_WAY

        transform = next_transform;

        #else

        // transform = append_transform(inverse_transform(prev_transform), transform);
        // transform = append_transform(next_transform, transform);


        #define HAWSOO_TEMP(_label, _trans)                                                                \
            BT_WARNF(                                                                                      \
                "%s\npos=(%0.3f, %0.3f, %0.3f)  rot=(%0.3f, %0.3f, %0.3f, %0.3f)  sca=(%0.3f, %0.3f, "     \
                "%0.3f)",                                                                                  \
                _label,                                                                                    \
                _trans.position.x, _trans.position.y, _trans.position.z,                                   \
                _trans.rotation.x, _trans.rotation.y, _trans.rotation.z, _trans.rotation.w,                \
                _trans.scale.x, _trans.scale.y, _trans.scale.z)

        // HAWSOO_TEMP("1. `transform`", transform);
        // HAWSOO_TEMP("1. `prev_transform`", prev_transform);
        // HAWSOO_TEMP("1. `inv(prev_transform)`", inverse_transform(prev_transform));


        // transform = append_transform(inverse_transform(prev_transform), transform);
        transform = append_transform(transform, inverse_transform(prev_transform));
        
        // HAWSOO_TEMP("2. `transform`", transform);
        // HAWSOO_TEMP("2. `next_transform`", next_transform);
        transform = append_transform(next_transform, transform);
        // HAWSOO_TEMP("3. `transform`", transform);

        #endif  // DIRECT_WAY
        #undef DIRECT_WAY
    }
    else
    {   // Apply inverse of `prev_transform` to get local transform, then apply `next_transform` to
        // get global transform.
        Timer my_timer;
        my_timer.start_timer();
        transform = append_transform(inverse_transform(prev_transform), transform);
        transform = append_transform(next_transform, transform);
        auto delta_time{ my_timer.calc_delta_time() };
        BT_WARNF("Time to trans (s): %0.12f", delta_time);

        // Apply same transformation to the `Transform_changed` if there is one.
        auto trans_changed{ reg.try_get<component::Transform_changed>(entity) };
        if (trans_changed != nullptr)
        {
            auto& next_trans{ trans_changed->next_transform };
            next_trans = append_transform(inverse_transform(prev_transform), next_trans);
            next_trans = append_transform(next_transform, next_trans);
        }
    }
    // Apply delta trans to self.

    // // transform = append_transform(transform, next_transform);
    // transform = append_transform(inv_transform, transform);  // @TODO: FIGURE OUT WHY THE TRANSFORMATIONS DONT WOKR!!!!!
    // transform = append_transform(transform, next_transform);

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

    // Calculate delta transforms.
    for (auto entity : changed_trans_view)
    {
        auto const& transform{ changed_trans_view.get<component::Transform const>(entity) };
        auto& transform_changed{ changed_trans_view.get<component::Transform_changed>(entity) };

        // // Append current transform to inverse of `transform` to get delta transform.
        // // @NOTE: Store the result inside of `new_transform`, even though it's not technically the
        // //        right name for it.
        // transform_changed.new_transform =
        //     append_transform(inverse_transform(transform), transform_changed.new_transform);
    }

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
