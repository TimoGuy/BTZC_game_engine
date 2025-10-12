////////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief Declaration of all concrete system classes using a macro to quickly create them.
///
/// @details To create a system:
///            1. Add the system using the `BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS` or
///               `BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS_WITH_PRIV_VARS` macros.
///            2. Create a .cpp file to define the ctor and `invoke_system_inner()` function.
///               See @TEMPLATE below.
///            3. Add creation of the system before the start of the main loop in the `main()`
///               function. This should also add the concrete class to the service finder.
///
/// @TEMPLATE:
// // // // clang-format off
// // // #include "concrete_systems.h"
// // // // clang-format on
// // //
// // // #include "../../service_finder/service_finder.h"
// // // #include "../component_registry.h"
// // // #include "../components.h"
// // // #include "system_ifc.h"
// // //
// // //
// // // namespace
// // // {
// // // enum : int32_t
// // // {
// // //     Q_IDX_COMP_LISTS_WITH_,
// // // };
// // // }  // namespace
// // //
// // //
// // // BT::component_system::system::__SYSTEM_CLASS_NAME_HERE__::__SYSTEM_CLASS_NAME_HERE__()
// // //     : System_ifc({
// // //           Component_list_query::compile_query_string("(@TODO query here!!)"),
// // //       })
// // // {   // @NOTE: Do not remove adding concrete class to service finder!!
// // //     BT_SERVICE_FINDER_ADD_SERVICE(__SYSTEM_CLASS_NAME_HERE__, this);
// // // }
// // //
// // // void BT::component_system::system::__SYSTEM_CLASS_NAME_HERE__::invoke_system_inner(
// // //     Component_lists_per_query&& comp_lists_per_query) const /*override*/
// // // {
// // //     // @TODO: Add code here!
// // // }
////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once

#include "system_ifc.h"


namespace BT
{
namespace component_system
{
namespace system
{

// Macros for quickly creating a system class declaration.
#define BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS(_class_name)                                         \
    BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS_WITH_PRIV_VARS(_class_name, )

#define BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS_WITH_PRIV_VARS(_class_name, _private_vars)           \
    class _class_name : public System_ifc                                                          \
    {                                                                                              \
    public:                                                                                        \
        _class_name();                                                                             \
                                                                                                   \
    protected:                                                                                     \
        void invoke_system_inner(Component_lists_per_query&& comp_lists_per_query) const override; \
                                                                                                   \
    private:                                                                                       \
        _private_vars                                                                              \
    };

// ---- List of systems ----------------------------------------------------------------------------
BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS(System__dev__anim_editor_tool_state_agent)
BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS(System_animator_driven_hitcapsule_set)
BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS(System_apply_phys_xform_to_rend_obj)
BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS(System_player_character_movement)
// -------------------------------------------------------------------------------------------------

// Undefine macros.
#undef BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS
#undef BT_COMP_SYS_SYS_SYSTEM_CONCRETE_CLASS_WITH_PRIV_VARS

}  // namespace system
}  // namespace component_system
}  // namespace BT
