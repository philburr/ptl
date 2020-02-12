#pragma once

#include "ptl/asio/io_service.hpp"
#include "ptl/asio/descriptor.hpp"
#include "ptl/asio/detail/io_service_definitions.hpp"
#include "ptl/asio/ip_endpoint.hpp"

namespace ptl::experimental::coroutine::asio {

struct file_read_operation;
struct file_write_operation;

struct socket;
struct file_internal : public ptl::experimental::coroutine::asio::descriptor
{
    file_internal(file_internal& s, ptl::experimental::coroutine::asio::descriptor::native_type d)
        : descriptor(d)
        , service_(s.service_)
    {}

    file_internal(ptl::experimental::coroutine::asio::detail::io_service_impl& service, ptl::experimental::coroutine::asio::descriptor::native_type d)
        : descriptor(d)
        , service_(service)
    {
        service_.register_descriptor(*this, data_);
    }

    file_internal(const file_internal&) = delete;
    file_internal& operator=(const file_internal&) = delete;

    file_internal(file_internal&& o)
        : descriptor(std::exchange(o.descriptor_, 0))
        , service_(o.service_)
        , data_(std::exchange(o.data_, nullptr))
    {}
    file_internal& operator=(file_internal&& o)
    {
        data_ = std::move(o.data_);
        return *this;
    }

    ~file_internal()
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

    int bind(const ptl::experimental::coroutine::asio::ip_endpoint& ep);
    int connect(const ptl::experimental::coroutine::asio::ip_endpoint& addr);
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

    static inline socket internal_create(file_internal& s, ptl::experimental::coroutine::asio::descriptor::native_type d = 0);

protected:
    ptl::experimental::coroutine::asio::detail::io_service_impl& service_;
    ptl::experimental::coroutine::asio::detail::descriptor_service_data* data_;

    ptl::experimental::coroutine::asio::ip_endpoint local_;
    ptl::experimental::coroutine::asio::ip_endpoint remote_;
};

struct file : private file_internal
{
    ~file() = default;

    file(const file&) = delete;
    file(file&&) = default;
    file& operator=(file&&) = default;

    static std::pair<file, file> create_pair(ptl::experimental::coroutine::asio::io_service& service);


    file_read_operation read(void* buffer, size_t size) noexcept;
    file_read_operation read_some(void* buffer, size_t size) noexcept;
    file_write_operation write(const void* buffer, size_t size) noexcept;

    using file_internal::native_descriptor;
    file_internal& internal()
    {
        return static_cast<file_internal&>(*this);
    }

private:
    friend class file_internal;
    file(file_internal& s, ptl::experimental::coroutine::asio::descriptor::native_type d)
        : file_internal(s, d)
    {}
    file(ptl::experimental::coroutine::asio::io_service& service, ptl::experimental::coroutine::asio::descriptor::native_type d)
        : file_internal(service.impl(), d)
    {}
};

} // namespace ptl::experimental::coroutine::asio

#include "file_write_operation.hpp"
#include "file_read_operation.hpp"

namespace ptl::experimental::coroutine::asio {

inline file_read_operation file::read(void* buffer, size_t size) noexcept
{
    return {*this, buffer, size, true};
}

inline fil_write_operation file::send(const void* buffer, size_t size) noexcept
{
    return {*this, buffer, size};
}

inline file file_internal::internal_create(file_internal& s, ptl::experimental::coroutine::asio::descriptor::native_type d)
{
    return {s, d};
}

} // namespace ptl::experimental::coroutine::asio