#pragma once
#include <cstddef>
#include <memory>
#include "ptl/experimental/asio/detail/io_service_impl.hpp"

namespace ptl::experimental::asio {

class io_service : protected detail::io_service_impl
{
public:
    io_service() = default;
    ~io_service() = default;

    io_service(const io_service&) = delete;
    io_service& operator=(const io_service&) = delete;
    io_service(io_service&&) = delete;
    io_service& operator=(io_service&&) = delete;

    using io_service_impl::stop;
    using io_service_impl::run;

    io_service_impl& impl() { return static_cast<io_service_impl&>(*this); }
};

}