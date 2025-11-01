#pragma once


namespace BT
{
namespace system
{

/// Creates physics objects for entities that need physics objects and destroys dangling physics
/// objects.
void process_physics_object_lifetime();

}  // namespace system
}  // namespace BT
