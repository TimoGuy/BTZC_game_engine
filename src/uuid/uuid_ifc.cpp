#include "uuid_ifc.h"

#include "../../third_party/stduuid/include/uuid.h"
#include "logger.h"
#include "uuid.h"
#include <cassert>

void BT::UUID_ifc::assign_uuid(string const& pretty_uuid, bool generate_if_nil)
{
    m_uuid = UUID_helper::to_UUID(pretty_uuid);
    assert(m_uuid.as_bytes().size() == 16);

    if (m_uuid.is_nil())
    {
        if (generate_if_nil)
        {
            assign_generated_uuid();
        }
        else
        {
            logger::printe(logger::ERROR, "Nil UUID assigned.");
            assert(false);
        }
    }
}

void BT::UUID_ifc::assign_generated_uuid()
{
    m_uuid = UUID_helper::generate_uuid();
}

BT::UUID BT::UUID_ifc::get_uuid() const
{
    return m_uuid;
}
