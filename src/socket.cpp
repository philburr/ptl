#include "ptl/experimental/asio/socket.hpp"
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>

using namespace ptl::experimental::asio;

std::pair<ptl::experimental::asio::socket, ptl::experimental::asio::socket> ptl::experimental::asio::socket::create_pair(ptl::asio::io_service& service)
{
    auto [s1, s2] = service.impl().create_pair();
    return { socket(service, s1), socket(service, s2) };
}
ptl::experimental::asio::socket socket::create_tcpv4(ptl::asio::io_service& service)
{
    auto s = service.impl().create_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    return socket(service, s);
}
ptl::experimental::asio::socket socket::create_tcpv6(ptl::asio::io_service& service)
{
    auto s = service.impl().create_socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
    return socket(service, s);
}

int socket_internal::bind(const ptl::asio::ip_endpoint& ep)
{
    if (ep.is_ipv4()) {
        const auto& ep4 = ep.to_ipv4();

        ::sockaddr_storage storage;
        ::sockaddr_in* addr = reinterpret_cast<::sockaddr_in*>(&storage);

        addr->sin_family = AF_INET;
        std::memcpy(&addr->sin_addr, ep4.address().bytes().data(), 4);
        addr->sin_port = ::htons(ep4.port());

        return service_.bind(native_descriptor(), addr, sizeof(::sockaddr_in));
    } else {
        const auto& ep6 = ep.to_ipv6();

        ::sockaddr_storage storage;
        ::sockaddr_in6* addr = reinterpret_cast<::sockaddr_in6*>(&storage);

        addr->sin6_family = AF_INET6;
        std::memcpy(&addr->sin6_addr, ep6.address().bytes().data(), 4);
        addr->sin6_port = ::htons(ep6.port());

        return service_.bind(native_descriptor(), addr, sizeof(::sockaddr_in));
    }
}

int socket_internal::listen()
{
    return service_.listen(native_descriptor(), 0);
}

int socket_internal::connect(const ptl::asio::ip_endpoint& ep)
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

int socket_internal::accept()
{
    if (local_.is_ipv4()) {
        ::sockaddr_storage storage;
        ::sockaddr_in* addr = reinterpret_cast<::sockaddr_in*>(&storage);
        size_t len = sizeof(*addr);

        int r = service_.accept(native_descriptor(), addr, &len);
        if (r < 0) {
            return r;
        }

        remote_ = ptl::asio::ipv4_endpoint(ptl::asio::ipv4_address(addr->sin_addr.s_addr), ntohs(addr->sin_port));
        return 0;
    } else {
        ::sockaddr_storage storage;
        ::sockaddr_in6* addr = reinterpret_cast<::sockaddr_in6*>(&storage);
        size_t len = sizeof(*addr);

        int r = service_.accept(native_descriptor(), addr, &len);
        if (r < 0) {
            return r;
        }

        remote_ = ptl::asio::ipv6_endpoint(ptl::asio::ipv6_address(addr->sin6_addr.s6_addr), ntohs(addr->sin6_port));
        return 0;
    }
}