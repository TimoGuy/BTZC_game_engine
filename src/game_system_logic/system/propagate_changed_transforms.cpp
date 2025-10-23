#include "propagate_changed_transforms.h"

#include "btglm.h"
#include "entt/entity/fwd.hpp"
#include "entt/entity/registry.hpp"
#include "game_system_logic/component/transform.h"


namespace
{

using namespace BT;

component::Transform inverse_transform(component::Transform const& trans)
{
    component::Transform inv_trans;
    
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

    return inv_trans;
}

component::Transform append_transform(component::Transform const& a, component::Transform const& b)
{
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

    return result;
}

void apply_delta_transform_recursive(auto& view,
                                     entt::entity entity,
                                     component::Transform const& delta_transform)
{   // Apply delta trans to self.
    assert(false);  // @TODO implement.

    // Search thru transform hierarchy children.
    auto const& transform_hierarchy{ view.get<component::Transform_hierarchy const>(entity) };
    for (auto entity : transform_hierarchy.children_entities)
    {
        apply_delta_transform_recursive(view, entity, delta_transform);
    }
}

}  // namespace


void BT::system::propagate_changed_transforms(entt::registry& reg)
{
    auto changed_trans_view = reg.view<component::Transform,
                                       component::Transform_hierarchy,
                                       component::Transform_changed>();
    auto trans_view = reg.view<component::Transform,
                               component::Transform_hierarchy>();

    // Calculate delta transforms.
    for (auto entity : changed_trans_view)
    {
        auto const& transform{ changed_trans_view.get<component::Transform const>(entity) };
        auto const& transform_hierarchy{
            changed_trans_view.get<component::Transform_hierarchy const>(entity)
        };
        auto& transform_changed{ changed_trans_view.get<component::Transform_changed>(entity) };

        // Invert `prev_transform`.
        component::Transform inv_prev_transform =
            inverse_transform(transform_changed.prev_transform);

        // Append current transform to `inv_prev_transform` to get delta transform.
        // @NOTE: Store the result inside of `prev_transform`, even though it's not technically the
        //        right name for it.
        transform_changed.prev_transform = append_transform(inv_prev_transform, transform);
    }

    // Propagate transforms with delta transforms.
    for (auto entity : changed_trans_view)
    {
        auto const& transform_hierarchy{
            changed_trans_view.get<component::Transform_hierarchy const>(entity)
        };
        auto const& transform_changed{ changed_trans_view.get<component::Transform_changed>(entity) };

        // @NOTE: Prev step inserts delta transform into here teehee.
        auto const& delta_transform{ transform_changed.prev_transform };

        for (auto entity : transform_hierarchy.children_entities)
        {
            apply_delta_transform_recursive(trans_view, entity, delta_transform);
        }
    }

    // Mark all transforms as propagated now (i.e. remove "changed" flag).
    reg.clear<component::Transform_changed>();

    assert(false);  // @TODO: implement.
}
