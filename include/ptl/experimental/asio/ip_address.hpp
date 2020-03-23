#pragma once
#include <array>
#include <cstdint>
#include <algorithm>
#include <variant>
#include <cassert>

namespace ptl::experimental::asio {

class ipv4_address
{
public:
    using data_type = std::array<uint8_t, 4>;

    constexpr ipv4_address()
        : address_{0, 0, 0, 0}
    {}

    explicit constexpr ipv4_address(uint32_t addr)
        : address_{
              static_cast<uint8_t>(addr >> 24),
              static_cast<uint8_t>(addr >> 16),
              static_cast<uint8_t>(addr >> 8),
              static_cast<uint8_t>(addr >> 0),
          }
    {}

    explicit constexpr ipv4_address(const uint8_t (&addr)[4])
        : address_{addr[0], addr[1], addr[2], addr[3]}
    {}

    explicit constexpr ipv4_address(uint8_t a0, uint8_t a1, uint8_t a2, uint8_t a3)
        : address_{a0, a1, a2, a3}
    {}

    constexpr uint32_t to_integer() const
    {
        return (uint32_t(address_[0]) << 24) | (uint32_t(address_[1]) << 16) | (uint32_t(address_[2]) << 8) |
               (uint32_t(address_[3]) << 0);
    }

    constexpr const data_type& bytes() const
    {
        return address_;
    }

    static constexpr ipv4_address loopback()
    {
        return ipv4_address{127, 0, 0, 1};
    }

    constexpr bool is_loopback() const
    {
        return address_[0] == 127;
    }

    constexpr bool is_private_network() const
    {
        return (address_[0] == 10) or (address_[0] == 172 and (address_[1] & 0xf0) == 16) or
               (address_[0] == 192 and address_[1] == 168);
    }

    constexpr bool operator==(ipv4_address other) const
    {
        return address_[0] == other.address_[0] and address_[1] == other.address_[1] and
               address_[2] == other.address_[2] and address_[3] == other.address_[3];
    }
    constexpr bool operator!=(ipv4_address other) const
    {
        return !operator==(other);
    }
    constexpr bool operator<(ipv4_address other) const
    {
        return to_integer() < other.to_integer();
    }
    constexpr bool operator>(ipv4_address other) const
    {
        return other.operator<(*this);
    }
    constexpr bool operator<=(ipv4_address other) const
    {
        return !operator>(other);
    }
    constexpr bool operator>=(ipv4_address other) const
    {
        return !operator<(other);
    }

private:
    alignas(uint32_t) data_type address_;
};

class ipv6_address
{
public:
    using data_type = std::array<uint8_t, 16>;

    constexpr ipv6_address()
        : address_{0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    {}

    explicit constexpr ipv6_address(uint64_t subnet, uint64_t iface)
        : address_{
              static_cast<uint8_t>(subnet >> 56), static_cast<uint8_t>(subnet >> 48),
              static_cast<uint8_t>(subnet >> 40), static_cast<uint8_t>(subnet >> 32),
              static_cast<uint8_t>(subnet >> 24), static_cast<uint8_t>(subnet >> 16),
              static_cast<uint8_t>(subnet >> 8),  static_cast<uint8_t>(subnet >> 0),
              static_cast<uint8_t>(iface >> 56),  static_cast<uint8_t>(iface >> 48),
              static_cast<uint8_t>(iface >> 40),  static_cast<uint8_t>(iface >> 32),
              static_cast<uint8_t>(iface >> 24),  static_cast<uint8_t>(iface >> 16),
              static_cast<uint8_t>(iface >> 8),   static_cast<uint8_t>(iface >> 0),
          }
    {}

    explicit constexpr ipv6_address(uint16_t a0, uint16_t a1, uint16_t a2, uint16_t a3, uint16_t a4, uint16_t a5,
                                    uint16_t a6, uint16_t a7)
        : address_{
              static_cast<uint8_t>(a0 >> 8), static_cast<uint8_t>(a0 >> 0), static_cast<uint8_t>(a1 >> 8),
              static_cast<uint8_t>(a1 >> 0), static_cast<uint8_t>(a2 >> 8), static_cast<uint8_t>(a2 >> 0),
              static_cast<uint8_t>(a3 >> 8), static_cast<uint8_t>(a3 >> 0), static_cast<uint8_t>(a4 >> 8),
              static_cast<uint8_t>(a4 >> 0), static_cast<uint8_t>(a5 >> 8), static_cast<uint8_t>(a5 >> 0),
              static_cast<uint8_t>(a6 >> 8), static_cast<uint8_t>(a6 >> 0), static_cast<uint8_t>(a7 >> 8),
              static_cast<uint8_t>(a7 >> 0),
          }
    {}

    explicit constexpr ipv6_address(const uint8_t (&addr)[16])
        : address_{addr[0], addr[1], addr[2],  addr[3],  addr[4],  addr[5],  addr[6],  addr[7],
                   addr[8], addr[9], addr[10], addr[11], addr[12], addr[13], addr[14], addr[15]}
    {}

    explicit constexpr ipv6_address(uint8_t a0, uint8_t a1, uint8_t a2, uint8_t a3, uint8_t a4, uint8_t a5, uint8_t a6,
                                    uint8_t a7, uint8_t a8, uint8_t a9, uint8_t a10, uint8_t a11, uint8_t a12,
                                    uint8_t a13, uint8_t a14, uint8_t a15)
        : address_{a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, 15}
    {}

    constexpr uint32_t subnet() const
    {
        return (uint64_t(address_[0]) << 56) | (uint64_t(address_[1]) << 48) | (uint64_t(address_[2]) << 40) |
               (uint64_t(address_[3]) << 32) | (uint64_t(address_[4]) << 24) | (uint64_t(address_[5]) << 16) |
               (uint64_t(address_[6]) << 8) | (uint64_t(address_[7]) << 0);
    }

    constexpr uint32_t interface() const
    {
        return (uint64_t(address_[0 + 8]) << 56) | (uint64_t(address_[1 + 8]) << 48) |
               (uint64_t(address_[2 + 8]) << 40) | (uint64_t(address_[3 + 8]) << 32) |
               (uint64_t(address_[4 + 8]) << 24) | (uint64_t(address_[5 + 8]) << 16) |
               (uint64_t(address_[6 + 8]) << 8) | (uint64_t(address_[7 + 8]) << 0);
    }

    constexpr const data_type& bytes() const
    {
        return address_;
    }

    static constexpr ipv6_address loopback()
    {
        return ipv6_address{0, 0, 0, 0, 0, 0, 0, 1};
    }

    static constexpr ipv6_address unspecified()
    {
        return ipv6_address{0, 0, 0, 0, 0, 0, 0, 0};
    }

    bool operator==(ipv6_address other) const
    {
        return std::equal(address_.begin(), address_.end(), other.address_.begin(), other.address_.end());
    }
    bool operator!=(ipv6_address other) const
    {
        return !operator==(other);
    }
    bool operator<(ipv6_address other) const
    {
        auto [left, right] =
            std::mismatch(address_.begin(), address_.end() - 1, other.address_.begin(), other.address_.end() - 1);
        return *left < *right;
    }
    bool operator>(ipv6_address other) const
    {
        return other.operator<(*this);
    }
    constexpr bool operator<=(ipv6_address other) const
    {
        return !operator>(other);
    }
    constexpr bool operator>=(ipv6_address other) const
    {
        return !operator<(other);
    }

private:
    alignas(uint64_t) data_type address_;
};

class ip_address
{
public:
    ip_address() noexcept
        : address_(ipv4_address())
    {}

    ip_address(ipv4_address addr) noexcept
        : address_(addr)
    {}

    ip_address(ipv6_address addr) noexcept
        : address_(addr)
    {}

    bool is_ipv4() const
    {
        return address_.index() == 0;
    }

    bool is_ipv6() const
    {
        return address_.index() == 1;
    }

    const ipv4_address& to_ipv4() const
    {
        assert(address_.index() == 0);
        return std::get<0>(address_);
    }

    const ipv6_address& to_ipv6() const
    {
        assert(address_.index() == 1);
        return std::get<1>(address_);
    }

private:
    std::variant<ipv4_address, ipv6_address> address_;
};

} // namespace ptl::experimental::asio
