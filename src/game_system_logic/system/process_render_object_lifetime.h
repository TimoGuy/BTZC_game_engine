#pragma once


namespace BT
{
namespace system
{

/// Creates render objects for entities that need render objects and destroys render objects that
/// aren't connected to an entity anymore.
void process_render_object_lifetime();

}  // namespace system
}  // namespace BT
