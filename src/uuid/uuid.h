#pragma once

// Uses https://github.com/mariusbancila/stduuid (MIT License)
#include "../../third_party/stduuid/include/uuid.h"
#include "nlohmann/json.hpp"

#include <cassert>
#include <string>

using json = nlohmann::json;
using std::string;


namespace BT
{

using UUID = uuids::uuid;

namespace UUID_helper
{

UUID generate_uuid();

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

// ---- UUID serialization -------------------------------------------------------------------------
template<
    typename BasicJsonType,
    nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0>
void to_json(BasicJsonType& nlohmann_json_j, const UUID& nlohmann_json_t)
{
    nlohmann_json_j["uuid_str"] = UUID_helper::to_pretty_repr(nlohmann_json_t);
}

template<
    typename BasicJsonType,
    nlohmann::detail::enable_if_t<nlohmann::detail::is_basic_json<BasicJsonType>::value, int> = 0>
void from_json(const BasicJsonType& nlohmann_json_j, UUID& nlohmann_json_t)
{
    std::string uuid_str;
    nlohmann_json_j.at("uuid_str").get_to(uuid_str);
    nlohmann_json_t = UUID_helper::to_UUID(uuid_str);
}
// -------------------------------------------------------------------------------------------------

}  // namespace BT
