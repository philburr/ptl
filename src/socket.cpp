#include "ptl/experimental/coroutine/asio/socket.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

using namespace ptl::experimental::coroutine::asio;
using namespace ptl::experimental::coroutine::asio::detail;

std::pair<ptl::experimental::coroutine::asio::socket, ptl::experimental::coroutine::asio::socket> ptl::experimental::coroutine::asio::socket::create_pair(ptl::experimental::coroutine::asio::io_service& service)
{
    auto [s1, s2] = service.impl().create_pair();
    return { socket(service, s1), socket(service, s2) };
}
ptl::experimental::coroutine::asio::socket socket::create_tcpv4(ptl::experimental::coroutine::asio::io_service& service)
{
    auto s = service.impl().create_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return socket(service, s);
}
ptl::experimental::coroutine::asio::socket socket::create_tcpv6(ptl::experimental::coroutine::asio::io_service& service)
{
    auto s = service.impl().create_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    return socket(service, s);
}

expected_void socket_internal::bind(const ptl::experimental::coroutine::asio::ip_endpoint& ep)
{
    local_ = ep;
    if (local_.is_ipv4()) {
        const auto& ep4 = local_.to_ipv4();

        ::sockaddr_storage storage;
        ::sockaddr_in* addr = reinterpret_cast<::sockaddr_in*>(&storage);

        addr->sin_family = AF_INET;
        std::memcpy(&addr->sin_addr, ep4.address().bytes().data(), 4);
        addr->sin_port = ::htons(ep4.port());

        return service_.bind(native_descriptor(), addr, sizeof(::sockaddr_in));
    } else {
        const auto& ep6 = local_.to_ipv6();

        ::sockaddr_storage storage;
        ::sockaddr_in6* addr = reinterpret_cast<::sockaddr_in6*>(&storage);

        addr->sin6_family = AF_INET6;
        std::memcpy(&addr->sin6_addr, ep6.address().bytes().data(), 4);
        addr->sin6_port = ::htons(ep6.port());

        return service_.bind(native_descriptor(), addr, sizeof(::sockaddr_in));
    }
}

expected_void socket_internal::listen()
{
    auto r = service_.listen(native_descriptor(), 0);
    if (r.is_error()) {
        return r;
    }
    return get_local();
}

expected_void socket_internal::connect(const ptl::experimental::coroutine::asio::ip_endpoint& ep)
{
    if (ep.is_ipv4()) {
        const auto& ep4 = ep.to_ipv4();

        ::sockaddr_storage storage;
        ::sockaddr_in* addr = reinterpret_cast<::sockaddr_in*>(&storage);

        addr->sin_family = AF_INET;
        std::memcpy(&addr->sin_addr, ep4.address().bytes().data(), 4);
        addr->sin_port = ::htons(ep4.port());

        return service_.connect(native_descriptor(), addr, sizeof(::sockaddr_in));
    } else {
        const auto& ep6 = ep.to_ipv6();

        ::sockaddr_storage storage;
        ::sockaddr_in6* addr = reinterpret_cast<::sockaddr_in6*>(&storage);

        addr->sin6_family = AF_INET6;
        std::memcpy(&addr->sin6_addr, ep6.address().bytes().data(), 4);
        addr->sin6_port = ::htons(ep6.port());

        return service_.connect(native_descriptor(), addr, sizeof(::sockaddr_in));
    }
}

expected_socket socket_internal::accept()
{
    if (local_.is_ipv4()) {
        ::sockaddr_storage storage;
        ::sockaddr_in* addr = reinterpret_cast<::sockaddr_in*>(&storage);
        size_t len = sizeof(*addr);

        auto r = service_.accept(native_descriptor(), addr, &len);
        if (r.is_error()) {
            return r;
        }

        remote_ = ptl::experimental::coroutine::asio::ipv4_endpoint(ptl::experimental::coroutine::asio::ipv4_address(addr->sin_addr.s_addr), ntohs(addr->sin_port));
        return r;
    } else {
        ::sockaddr_storage storage;
        ::sockaddr_in6* addr = reinterpret_cast<::sockaddr_in6*>(&storage);
        size_t len = sizeof(*addr);

        auto r = service_.accept(native_descriptor(), addr, &len);
        if (r.is_error()) {
            return r;
        }

        remote_ = ptl::experimental::coroutine::asio::ipv6_endpoint(ptl::experimental::coroutine::asio::ipv6_address(addr->sin6_addr.s6_addr), ntohs(addr->sin6_port));
        return r;
    }
}

expected_void socket_internal::shutdown(int how)
{
    return service_.shutdown(native_descriptor(), how);
}

expected_void socket_internal::get_local()
{
    if (local_.is_ipv4()) {
        ::sockaddr_storage storage;
        ::sockaddr_in* addr = reinterpret_cast<::sockaddr_in*>(&storage);
        size_t len = sizeof(*addr);

        auto r = service_.getsockname(native_descriptor(), addr, &len);
        if (r.is_error()) {
            return r;
        }

        local_ = ptl::experimental::coroutine::asio::ipv4_endpoint(ptl::experimental::coroutine::asio::ipv4_address(ntohl(addr->sin_addr.s_addr)), ntohs(addr->sin_port));
    } else {
        ::sockaddr_storage storage;
        ::sockaddr_in6* addr = reinterpret_cast<::sockaddr_in6*>(&storage);
        size_t len = sizeof(*addr);

        auto r = service_.getsockname(native_descriptor(), addr, &len);
        if (r.is_error()) {
            return r;
        }

        local_ = ptl::experimental::coroutine::asio::ipv6_endpoint(ptl::experimental::coroutine::asio::ipv6_address(addr->sin6_addr.s6_addr), ntohs(addr->sin6_port));
    }
    return {};
}

expected_void socket_internal::get_remote()
{
    if (local_.is_ipv4()) {
        ::sockaddr_storage storage;
        ::sockaddr_in* addr = reinterpret_cast<::sockaddr_in*>(&storage);
        size_t len = sizeof(*addr);

        auto r = service_.getpeername(native_descriptor(), addr, &len);
        if (r.is_error()) {
            return r;
        }

        remote_ = ptl::experimental::coroutine::asio::ipv4_endpoint(ptl::experimental::coroutine::asio::ipv4_address(ntohl(addr->sin_addr.s_addr)), ntohs(addr->sin_port));
    } else {
        ::sockaddr_storage storage;
        ::sockaddr_in6* addr = reinterpret_cast<::sockaddr_in6*>(&storage);
        size_t len = sizeof(*addr);

        auto r = service_.getpeername(native_descriptor(), addr, &len);
        if (r.is_error()) {
            return r;
        }

        remote_ = ptl::experimental::coroutine::asio::ipv6_endpoint(ptl::experimental::coroutine::asio::ipv6_address(addr->sin6_addr.s6_addr), ntohs(addr->sin6_port));
    }
    return {};
}

