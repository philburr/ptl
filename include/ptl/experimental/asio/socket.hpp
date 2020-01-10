#pragma once

#include "ptl/asio/io_service.hpp"
#include "ptl/asio/descriptor.hpp"
#include "ptl/asio/detail/io_service_definitions.hpp"
#include "ptl/asio/ip_endpoint.hpp"

namespace ptl::experimental::asio {

struct socket_connect_operation;
struct socket_accept_operation;
struct socket_shutdown_operation;
struct socket_recv_operation;
struct socket_send_operation;

struct socket;
struct socket_internal : public ptl::asio::descriptor
{
    socket_internal(socket_internal& s, ptl::asio::descriptor::native_type d)
        : descriptor(d)
        , service_(s.service_)
    {}

    socket_internal(ptl::asio::detail::io_service_impl& service, ptl::asio::descriptor::native_type d)
        : descriptor(d)
        , service_(service)
    {
        service_.register_descriptor(*this, data_);
    }

    socket_internal(const socket_internal&) = delete;
    socket_internal& operator=(const socket_internal&) = delete;

    socket_internal(socket_internal&& o)
        : descriptor(std::exchange(o.descriptor_, 0))
        , service_(o.service_)
        , data_(std::exchange(o.data_, nullptr))
    {}
    socket_internal& operator=(socket_internal&& o)
    {
        data_ = std::move(o.data_);
        return *this;
    }

    ~socket_internal()
    {
        if (data_ != nullptr) {
            service_.deregister_descriptor(*this, data_);
        }
        if (descriptor_ != 0) {
            service_.close(descriptor_);
        }
    }

    void start_io(ptl::asio::io_kind kind, ptl::asio::io_service_operation* op)
    {
        service_.start_io(*data_, kind, op);
    }

    int bind(const ptl::asio::ip_endpoint& ep);
    int connect(const ptl::asio::ip_endpoint& addr);
    int accept();
    int listen();
    int shutdown(int how);

    ssize_t send(const uint8_t* buffer, size_t sz, int flags)
    {
        return service_.send(native_descriptor(), buffer, sz, flags);
    }
    ssize_t recv(uint8_t* buffer, size_t sz, int flags)
    {
        return service_.recv(native_descriptor(), buffer, sz, flags);
    }

    static inline socket internal_create(socket_internal& s, ptl::asio::descriptor::native_type d = 0);

protected:
    ptl::asio::detail::io_service_impl& service_;
    ptl::asio::detail::descriptor_service_data* data_;

    ptl::asio::ip_endpoint local_;
    ptl::asio::ip_endpoint remote_;
};

struct socket : private socket_internal
{
    ~socket() = default;

    socket(const socket&) = delete;
    socket(socket&&) = default;
    socket& operator=(socket&&) = default;

    static socket create_tcpv4(ptl::asio::io_service& service);
    static socket create_tcpv6(ptl::asio::io_service& service);
    static std::pair<socket, socket> create_pair(ptl::asio::io_service& service);

    using socket_internal::bind;
    using socket_internal::listen;

    socket_connect_operation connect(const ptl::asio::ip_endpoint& addr);
    socket_accept_operation accept();
    socket_shutdown_operation shutdown();
    socket_recv_operation recv(void* buffer, size_t size) noexcept;
    socket_recv_operation recv_some(void* buffer, size_t size) noexcept;
    socket_send_operation send(const void* buffer, size_t size) noexcept;

    using socket_internal::native_descriptor;
    socket_internal& internal()
    {
        return static_cast<socket_internal&>(*this);
    }

private:
    friend class socket_internal;
    socket(socket_internal& s, ptl::asio::descriptor::native_type d)
        : socket_internal(s, d)
    {}
    socket(ptl::asio::io_service& service, ptl::asio::descriptor::native_type d)
        : socket_internal(service.impl(), d)
    {}
};

} // namespace ptl::experimental::asio

#include "socket_connect_operation.hpp"
#include "socket_accept_operation.hpp"
#include "socket_shutdown_operation.hpp"
#include "socket_send_operation.hpp"
#include "socket_recv_operation.hpp"

namespace ptl::experimental::asio {

inline socket_connect_operation socket::connect(const ptl::asio::ip_endpoint& addr)
{
    return socket_connect_operation{*this, addr};
}

inline socket_accept_operation socket::accept()
{
    return socket_accept_operation{*this};
}

inline socket_shutdown_operation socket::shutdown()
{
    return socket_shutdown_operation{*this};
}

inline socket_recv_operation socket::recv(void* buffer, size_t size) noexcept
{
    return socket_recv_operation{*this, buffer, size, true};
}

inline socket_send_operation socket::send(const void* buffer, size_t size) noexcept
{
    return socket_send_operation{*this, buffer, size};
}

inline socket socket_internal::internal_create(socket_internal& s, ptl::asio::descriptor::native_type d)
{
    return socket{s, d};
}

} // namespace ptl::experimental::asio