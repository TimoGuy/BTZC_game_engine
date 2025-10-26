#pragma once

#include "btjson.h"
#include "uuid/uuid.h"

#include <string>


namespace BT
{
namespace component
{

/// Metadata of an entity.
struct Entity_metadata
{
    std::string name;
    UUID uuid;

    NLOHMANN_DEFINE_TYPE_INTRUSIVE_WITH_DEFAULT(
        Entity_metadata,
        name,
        uuid
    );
};

}  // namespace component
}  // namespace BT
