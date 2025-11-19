#pragma once


namespace BT
{
namespace system
{

/// Creates render objects for entities that need render objects and destroys render objects that
/// aren't connected to an entity anymore.
/// @param force_allow_deformed_render_objs - If set to true, does not require the world to be in
///                                           simulation running mode.
void process_render_object_lifetime(bool force_allow_deformed_render_objs);

}  // namespace system
}  // namespace BT
