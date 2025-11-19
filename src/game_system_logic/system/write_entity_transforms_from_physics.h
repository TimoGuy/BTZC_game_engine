#pragma once


namespace BT
{
namespace system
{

/// Reads physics calculations result and writes to the entity transforms.
/// @NOTE: Since entity transforms have a hierarchy, this only submits `Transform_changed`
///        components.
void write_entity_transforms_from_physics();

}  // namespace system
}  // namespace BT
