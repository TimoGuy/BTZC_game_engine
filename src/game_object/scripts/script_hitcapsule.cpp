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
    }

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

void BT::Scripts::Script_hitcapsule::serialize_datas(json& node_ref)
{
    node_ref["render_obj"] = UUID_helper::to_pretty_repr(m_rend_obj_key);
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
    return std::unique_ptr<Script_ifc>(new Script_hitcapsule{ node_ref });
}
