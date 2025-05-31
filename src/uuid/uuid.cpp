#include "uuid.h"

// Uses https://github.com/crashoz/uuid_v4 (MIT License)
#include "uuid_v4.h"
#include "endianness.h"

#include "logger.h"
#include <cassert>
#include <memory>
#include <random>

using std::make_unique;


namespace
{

inline UUIDv4::UUID* rcaUUID(char* ptr)
{
    return reinterpret_cast<UUIDv4::UUID*>(ptr);
}

}  // namespace


BT::UUID::UUID()
{
}

BT::UUID::UUID(string const& pretty_uuid)
{
    auto new_uuid{ UUIDv4::UUID::fromStrFactory(pretty_uuid) };
    emplace_uuid(sizeof(new_uuid), reinterpret_cast<char*>(&new_uuid));
}

BT::UUID::UUID(UUID& other)
{
    emplace_uuid(sizeof(UUIDv4::UUID), other.m_uuid_holder.get());
}

bool BT::UUID::operator==(UUID const& rhs) const
{
    return (*rcaUUID(m_uuid_holder.get()) == *rcaUUID(rhs.m_uuid_holder.get()));
}

bool BT::UUID::operator<(UUID const& rhs) const
{
    return (*rcaUUID(m_uuid_holder.get()) < *rcaUUID(rhs.m_uuid_holder.get()));
}

void BT::UUID::generate_uuid()
{
    if (m_uuid_holder != nullptr)
    {
        logger::printe(logger::ERROR, "Generating UUID when UUID is already assigned.");
        assert(false);
        return;
    }

    static thread_local UUIDv4::UUIDGenerator<std::mt19937_64> s_uuid_generator;
    auto new_uuid{ s_uuid_generator.getUUID() };
    emplace_uuid(sizeof(new_uuid), reinterpret_cast<char*>(&new_uuid));
}

string BT::UUID::pretty_repr() const
{
    return rcaUUID(m_uuid_holder.get())->str();
}

size_t BT::UUID::hash() const
{
    return rcaUUID(m_uuid_holder.get())->hash();
}

void BT::UUID::emplace_uuid(size_t bytes, char* data)
{
    assert(bytes == sizeof(UUIDv4::UUID));
    m_uuid_holder = make_unique<char[]>(sizeof(UUIDv4::UUID));
    memcpy(m_uuid_holder.get(), data, bytes);
}
