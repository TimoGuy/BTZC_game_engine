#include "logger.h"
#include "scripts.h"

#include "../renderer/renderer.h"
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
        m_rend_obj_key = UUID_helper::to_UUID(node_ref["render_obj"]);
        deserialize_into_hitcapsule_groups(node_ref);

        // Calc hitcapsule animator.
        auto& rend_obj_pool{ service_finder::find_service<Renderer>()
                                 .get_render_object_pool() };
        auto rend_obj{ rend_obj_pool
                           .checkout_render_obj_by_key({ m_rend_obj_key })
                           .front() };
        auto animator{ rend_obj->get_model_animator() };

        for (auto& group : m_hitcapsule_grps)
        for (auto& capsule : group.get_capsules())
        {
            capsule.init_calc_info(animator);
        }

        rend_obj_pool.return_render_objs({ rend_obj });

        // Add hitcapsule groups to solver service.
        auto& hitcapsule_solver{
            service_finder::find_service<Hitcapsule_group_overlap_solver>() };

        m_hitcapsule_grp_uuids.reserve(m_hitcapsule_grps.size());
        for (auto const& group : m_hitcapsule_grps)
        {
            // @NOCHECKIN
            UUID group_uuid; //{ hitcapsule_solver.add_group(group) };
            m_hitcapsule_grp_uuids.emplace_back(group_uuid);
        }
    }

    ~Script_hitcapsule()
    {   // Remove hitcapsule groups from solver service.
        auto& hitcapsule_solver{
            service_finder::find_service<Hitcapsule_group_overlap_solver>() };

        for (auto group_uuid : m_hitcapsule_grp_uuids)
        {
            // @NOCHECKIN
            // hitcapsule_solver.remove_group(group_uuid);
        }
    }

    void deserialize_into_hitcapsule_groups(json const& node_ref);

    // Script_ifc.
    Script_type get_type() override { return SCRIPT_TYPE_hitcapsule; }
    void serialize_datas(json& node_ref) override;

    // Script data.
    UUID m_rend_obj_key;
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
{
    node_ref["render_obj"] = UUID_helper::to_pretty_repr(m_rend_obj_key);

    // Access list of hitcapsule groups.
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
