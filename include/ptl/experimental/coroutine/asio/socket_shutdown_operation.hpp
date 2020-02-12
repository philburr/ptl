#pragma once

#include "socket.hpp"
#include "detail/io_operation.hpp"

namespace ptl::experimental::coroutine::asio {

struct socket_shutdown_operation : public detail::io_operation<socket_shutdown_operation>
{
    socket_shutdown_operation(socket& s)
        : io_operation<socket_shutdown_operation>()
        , socket_(s.internal())
    {}

private:
    friend class detail::io_operation<socket_shutdown_operation>;
    bool begin()
    {
        auto r = socket_.shutdown(ptl::experimental::coroutine::asio::detail::shutdown_read_write);
        if (r.is_error()) {
            if (r.error().value() == EINPROGRESS) {
                // we need notification
                socket_.start_io(ptl::experimental::coroutine::asio::io_kind::write, this);
                return false;
            }
            ec_ = r.error();
        }
        return true;
    }

    void get_return() {}

    void work() override
    {
        auto r = socket_.shutdown(ptl::experimental::coroutine::asio::detail::shutdown_read_write);
        if (r.is_error()) {
            ec_ = r.error();
        }
        resume();
    }

    socket_internal& socket_;
};

} // namespace ptl::experimental::coroutine::asio