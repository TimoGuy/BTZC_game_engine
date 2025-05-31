#pragma once

// Uses https://github.com/mariusbancila/stduuid (MIT License)
#include "../../third_party/stduuid/include/uuid.h"

#include <array>
#include <string>

using std::array;
using std::string;


namespace BT
{

using UUID = uuids::uuid;

namespace UUID_helper
{

inline static string pretty_repr(UUID const& my_uuid)
{
    return uuids::to_string(my_uuid);
}

}  // namespace UUID_helper

class UUID_ifc
{
public:
    void assign_uuid(string const& pretty_uuid);
    void generate_uuid();
    UUID get_uuid() const;

private:
    UUID m_uuid;
};

}  // namespace BT
