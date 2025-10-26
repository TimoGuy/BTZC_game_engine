#pragma once

#include "nlohmann/json.hpp"
using json = nlohmann::json;


namespace BT
{

/// Helper to load json from disk.
json json_load_from_disk(std::string const& fname);

/// Helper to save json to disk.
void json_save_to_disk(json const& data_j, std::string const& fname);

}  // namespace BT
