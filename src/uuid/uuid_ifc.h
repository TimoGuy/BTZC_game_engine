#pragma once

#include "uuid.h"
#include <string>

using std::string;


namespace BT
{

class UUID_ifc
{
public:
    void assign_uuid(string const& pretty_uuid, bool generate_if_nil);
    void assign_generated_uuid();
    UUID get_uuid() const;

private:
    UUID m_uuid;
};

}  // namespace BT
