#pragma once
#include "ip_address.hpp"

namespace ptl::experimental::coroutine::asio {

class ipv4_endpoint
{
public:
    ipv4_endpoint() noexcept = default;
    explicit ipv4_endpoint(ipv4_address addr, uint16_t port = 0)
        : address_(addr)
        , port_(port)
    {}

    const ipv4_address& address() const noexcept
    {
        return address_;
    }

    uint16_t port() const noexcept
    {
        return port_;
    }


private:
    ipv4_address address_;
    uint16_t port_ = 0;
};

class ipv6_endpoint
{
public:
    ipv6_endpoint() noexcept = default;
    explicit ipv6_endpoint(ipv6_address addr, uint16_t port = 0)
        : address_(addr)
        , port_(port)
    {}

    const ipv6_address& address() const noexcept
    {
        return address_;
    }

    uint16_t port() const noexcept
    {
        return port_;
    }

private:
    ipv6_address address_;
    uint16_t port_ = 0;
};

class ip_endpoint
{
public:
    ip_endpoint()
        : ep_(ipv4_endpoint())
    {}

    ip_endpoint(ipv4_endpoint ep)
        : ep_(ep)
    {}

    ip_endpoint(ipv6_endpoint ep)
        : ep_(ep)
    {}

    bool is_ipv4() const
    {
        return ep_.index() == 0;
    }

    bool is_ipv6() const
    {
        return ep_.index() == 1;
    }

    const ipv4_endpoint& to_ipv4() const
    {
        assert(ep_.index() == 0);
        return std::get<0>(ep_);
    }
    const ipv6_endpoint& to_ipv6() const
    {
        assert(ep_.index() == 1);
        return std::get<1>(ep_);
    }


private:
    std::variant<ipv4_endpoint, ipv6_endpoint> ep_;
};

} // namespace ptl::experimental::coroutine::asio