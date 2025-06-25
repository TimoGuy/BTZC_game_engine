#pragma once

// Uses https://github.com/mariusbancila/stduuid (MIT License)
#include "../../third_party/stduuid/include/uuid.h"

#include <cassert>
#include <string>

using std::string;


namespace BT
{

using UUID = uuids::uuid;

namespace UUID_helper
{

inline static string to_pretty_repr(UUID const& my_uuid)
{
    return uuids::to_string(my_uuid);
}

inline static UUID to_UUID(string const& pretty_uuid)
{
    auto opt_uuid{ uuids::uuid::from_string(pretty_uuid) };
    assert(opt_uuid.has_value());
    assert(uuids::to_string(opt_uuid.value()) == pretty_uuid);
    return opt_uuid.value();
}

}  // namespace UUID_helper

}  // namespace BT
