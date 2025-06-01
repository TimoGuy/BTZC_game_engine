#include "uuid_ifc.h"

#include "../../third_party/stduuid/include/uuid.h"
#include "logger.h"
#include <algorithm>
#include <cassert>
#include <random>


namespace
{

std::array<int, std::mt19937::state_size> generate_seed_data()
{
    std::array<int, std::mt19937::state_size> seed_data{};
    std::random_device rd;
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    return seed_data;
}

}  // namespace


void BT::UUID_ifc::assign_uuid(string const& pretty_uuid, bool generate_if_nil)
{
    m_uuid = UUID_helper::to_UUID(pretty_uuid);
    assert(m_uuid.as_bytes().size() == 16);

    if (m_uuid.is_nil())
    {
        if (generate_if_nil)
        {
            generate_uuid();
        }
        else
        {
            logger::printe(logger::ERROR, "Nil UUID assigned.");
            assert(false);
        }
    }
}

void BT::UUID_ifc::generate_uuid()
{
    static thread_local auto s_seed_data{ generate_seed_data() };
    static thread_local std::seed_seq s_seq(std::begin(s_seed_data), std::end(s_seed_data));
    static thread_local std::mt19937 s_generator(s_seq);
    static thread_local uuids::uuid_random_generator s_gen{s_generator};

    m_uuid = s_gen();
    assert(!m_uuid.is_nil());
    assert(m_uuid.as_bytes().size() == 16);
    assert(m_uuid.version() == uuids::uuid_version::random_number_based);
    assert(m_uuid.variant() == uuids::uuid_variant::rfc);
}

BT::UUID BT::UUID_ifc::get_uuid() const
{
    return m_uuid;
}
