#pragma once

#include <memory>
#include <string>

using std::string;
using std::unique_ptr;


namespace BT
{

class UUID
{
public:
    UUID();
    explicit UUID(string const& pretty_uuid);
    UUID(UUID& other);

    bool operator==(UUID const& rhs) const;
    bool operator< (UUID const& rhs) const;
    bool operator!=(UUID const& rhs) const { return !(*this == rhs); }
    bool operator> (UUID const& rhs) const { return rhs < *this; }
    bool operator<=(UUID const& rhs) const { return !(*this > rhs); }
    bool operator>=(UUID const& rhs) const { return !(*this < rhs); }

    void generate_uuid();
    string pretty_repr() const;
    size_t hash() const;

private:
    void emplace_uuid(size_t bytes, char* data);
    unique_ptr<char[]> m_uuid_holder{ nullptr };
};

}  // namespace BT


namespace std
{

template <>
struct hash<BT::UUID>
{
    size_t operator()(BT::UUID const& uuid) const
    {
        return uuid.hash();
    }
};

}  // namespace std
