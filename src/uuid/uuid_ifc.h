#pragma once

// Uses https://github.com/mariusbancila/stduuid (MIT License)
#include "../../third_party/stduuid/include/uuid.h"

#include <array>
#include <string>

using std::array;
using std::string;


namespace BT
{

// class UUID
// {
// public:
//     static constexpr UUID const get_invalid() { return UUID(); }

//     bool operator==(UUID const& rhs) const;
//     bool operator< (UUID const& rhs) const;
//     bool operator!=(UUID const& rhs) const { return !(*this == rhs); }
//     bool operator> (UUID const& rhs) const { return rhs < *this; }
//     bool operator<=(UUID const& rhs) const { return !(*this > rhs); }
//     bool operator>=(UUID const& rhs) const { return !(*this < rhs); }

//     void assign_uuid(string const& pretty_uuid);
//     void generate_uuid();
//     bool is_valid() const;
//     string pretty_repr() const;
//     size_t hash() const;

// private:
//     static constexpr size_t k_uuid_size_bytes{ 128 };
//     void emplace_uuid(char* data);
//     bool m_uuid_initialized{ false };
//     array<char, k_uuid_size_bytes> m_uuid_holder;
// };

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
    // bool is_valid() const;
    // string pretty_repr() const;
    UUID get_uuid() const;

private:
    UUID m_uuid;
};

}  // namespace BT


// namespace std
// {

// template <>
// struct hash<BT::UUID>
// {
//     size_t operator()(BT::UUID const& uuid) const
//     {
//         return uuid.hash();
//     }
// };

// }  // namespace std
