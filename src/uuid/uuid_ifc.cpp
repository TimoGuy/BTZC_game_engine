#include "uuid_ifc.h"

#include "logger.h"
#include "uuid.h"
#include <algorithm>
#include <cassert>
#include <random>


// namespace
// {

// inline UUIDv4::UUID* as_UUID(char const* ptr)
// {
//     return reinterpret_cast<UUIDv4::UUID*>(const_cast<char*>(ptr));
// }

// }  // namespace


// bool BT::UUID::operator==(UUID const& rhs) const
// {
//     return (*as_UUID(m_uuid_holder.data()) == *as_UUID(rhs.m_uuid_holder.data()));
// }

// bool BT::UUID::operator<(UUID const& rhs) const
// {
//     return (*as_UUID(m_uuid_holder.data()) < *as_UUID(rhs.m_uuid_holder.data()));
// }

// void BT::UUID::assign_uuid(string const& pretty_uuid)
// {
//     auto new_uuid{ UUIDv4::UUID::fromStrFactory(pretty_uuid) };
//     emplace_uuid(reinterpret_cast<char*>(&new_uuid));
// }

// void BT::UUID::generate_uuid()
// {
//     if (m_uuid_initialized)
//     {
//         logger::printe(logger::ERROR, "Generating UUID when UUID is already initialized.");
//         assert(false);
//         return;
//     }

//     static thread_local UUIDv4::UUIDGenerator<std::mt19937_64> s_uuid_generator;
//     auto new_uuid{ s_uuid_generator.getUUID() };
//     emplace_uuid(reinterpret_cast<char*>(&new_uuid));
// }

// bool BT::UUID::is_valid() const
// {
//     return m_uuid_initialized;
// }

// string BT::UUID::pretty_repr() const
// {
//     return as_UUID(m_uuid_holder.data())->str();
// }

// size_t BT::UUID::hash() const
// {
//     return as_UUID(m_uuid_holder.data())->hash();
// }

// void BT::UUID::emplace_uuid(char* data)
// {
//     static_assert(k_uuid_size_bytes == sizeof(UUIDv4::UUID));
//     memcpy(m_uuid_holder.data(), data, sizeof(UUIDv4::UUID));
//     m_uuid_initialized = true;
// }

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


void BT::UUID_ifc::assign_uuid(string const& pretty_uuid)
{
    auto opt_uuid{ uuids::uuid::from_string(pretty_uuid) };
    assert(opt_uuid.has_value());
    assert(uuids::to_string(opt_uuid.value()) == pretty_uuid);
    m_uuid = opt_uuid.value();
    assert(!m_uuid.is_nil());
    assert(m_uuid.as_bytes().size() == 16);
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

// bool BT::UUID_ifc::is_valid() const
// {
//     return !m_uuid.is_nil();
// }

// string BT::UUID_ifc::pretty_repr() const
// {
//     return UUID_helper::pretty_repr(m_uuid);
// }

BT::UUID BT::UUID_ifc::get_uuid() const
{
    return m_uuid;
}
