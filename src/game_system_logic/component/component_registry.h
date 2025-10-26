#pragma once

#include "btjson.h"
#include "entt/entity/fwd.hpp"

#include <string>


namespace BT
{
namespace component
{

/// Sets up all components to ensure that there are proper serialization/deserialization procedures
/// created.
void register_all_components();

/// Takes component name and serialized member data and constructs a component into the ECS entity.
void construct_component(entt::entity ecs_entity, std::string const& type_name, json const& members_j);

}  // namespace component
}  // namespace BT
