#pragma once

#include "ptl/experimental/coroutine/asio/io_service.hpp"
#include "ptl/experimental/coroutine/asio/descriptor.hpp"
#include "ptl/experimental/coroutine/asio/detail/io_service_definitions.hpp"
#include "ptl/experimental/coroutine/asio/ip_endpoint.hpp"

namespace ptl::experimental::coroutine::asio {

struct socket_connect_operation;
struct socket_accept_operation;
struct socket_shutdown_operation;
struct socket_recv_operation;
struct socket_send_operation;

struct socket;
struct socket_internal : public ptl::experimental::coroutine::asio::descriptor
{
    socket_internal(socket_internal& s, ptl::experimental::coroutine::asio::descriptor::native_type d)
        : descriptor(d)
        , service_(s.service_)
    {
        service_.register_descriptor(*this, data_);
    }

    socket_internal(ptl::experimental::coroutine::asio::detail::io_service_impl& service, ptl::experimental::coroutine::asio::descriptor::native_type d)
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

    friend void swap(socket_internal& left, socket_internal&& right)
    {
        std::swap(left.descriptor_, right.descriptor_);
        std::swap(left.data_, right.data_);
    }

    socket_internal& operator=(socket_internal&& o)
    {
        swap(*this, socket_internal(std::move(o)));
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

    void start_io(ptl::experimental::coroutine::asio::io_kind kind, ptl::experimental::coroutine::asio::io_service_operation* op)
    {
        service_.start_io(*data_, kind, op);
    }

    detail::expected_void bind(const ptl::experimental::coroutine::asio::ip_endpoint& ep);
    detail::expected_void connect(const ptl::experimental::coroutine::asio::ip_endpoint& addr);
    detail::expected_socket accept();
    detail::expected_void listen();
    detail::expected_void shutdown(int how);

    ssize_t send(const uint8_t* buffer, size_t sz, int flags)
    {
        return service_.send(native_descriptor(), buffer, sz, flags);
    }
    ssize_t recv(uint8_t* buffer, size_t sz, int flags)
    {
        return service_.recv(native_descriptor(), buffer, sz, flags);
    }

    detail::expected_void get_local();
    detail::expected_void get_remote();

    static inline socket internal_create(socket_internal& s, ptl::experimental::coroutine::asio::descriptor::native_type d = 0);

    const ptl::experimental::coroutine::asio::ip_endpoint& local_address() const noexcept
    {
        return local_;
    }

    const ptl::experimental::coroutine::asio::ip_endpoint& remote_address() const noexcept
    {
        return remote_;
    }

protected:
    ptl::experimental::coroutine::asio::detail::io_service_impl& service_;
    ptl::experimental::coroutine::asio::detail::descriptor_service_data* data_;

    ptl::experimental::coroutine::asio::ip_endpoint local_;
    ptl::experimental::coroutine::asio::ip_endpoint remote_;
};

struct socket : private socket_internal
{
    ~socket() = default;

    socket(const socket&) = delete;
    socket(socket&&) = default;
    socket& operator=(socket&&) = default;

    static socket create_tcpv4(ptl::experimental::coroutine::asio::io_service& service);
    static socket create_tcpv6(ptl::experimental::coroutine::asio::io_service& service);
    static std::pair<socket, socket> create_pair(ptl::experimental::coroutine::asio::io_service& service);

    using socket_internal::bind;
    using socket_internal::listen;

    socket_connect_operation connect(const ptl::experimental::coroutine::asio::ip_endpoint& addr);
    socket_accept_operation accept();
    socket_shutdown_operation shutdown();
    socket_recv_operation recv(void* buffer, size_t size) noexcept;
    socket_recv_operation recv_some(void* buffer, size_t size) noexcept;
    socket_send_operation send(const void* buffer, size_t size) noexcept;

    using socket_internal::local_address;
    using socket_internal::remote_address;

    using socket_internal::native_descriptor;
    socket_internal& internal()
    {
        return static_cast<socket_internal&>(*this);
    }

private:
    friend class socket_internal;
    socket(socket_internal& s, ptl::experimental::coroutine::asio::descriptor::native_type d)
        : socket_internal(s, d)
    {}
    socket(ptl::experimental::coroutine::asio::io_service& service, ptl::experimental::coroutine::asio::descriptor::native_type d)
        : socket_internal(service.impl(), d)
    {}
};

} // namespace ptl::experimental::coroutine::asio

#include "socket_connect_operation.hpp"
#include "socket_accept_operation.hpp"
#include "socket_shutdown_operation.hpp"
#include "socket_send_operation.hpp"
#include "socket_recv_operation.hpp"

namespace ptl::experimental::coroutine::asio {

inline socket_connect_operation socket::connect(const ptl::experimental::coroutine::asio::ip_endpoint& addr)
{
    return {*this, addr};
}

inline socket_accept_operation socket::accept()
{
    return {*this};
}

inline socket_shutdown_operation socket::shutdown()
{
    return {*this};
}

inline socket_recv_operation socket::recv(void* buffer, size_t size) noexcept
{
    return socket_recv_operation{*this, buffer, size, true};
}

inline socket_send_operation socket::send(const void* buffer, size_t size) noexcept
{
    return socket_send_operation{*this, buffer, size};
}

inline socket socket_internal::internal_create(socket_internal& s, ptl::experimental::coroutine::asio::descriptor::native_type d)
{
    return socket{s, d};
}

} // namespace ptl::experimental::coroutine::asio
