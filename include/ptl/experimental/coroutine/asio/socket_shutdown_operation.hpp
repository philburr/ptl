#pragma once

#include "socket.hpp"
#include "detail/socket_operation.hpp"

namespace ptl::experimental::asio {

struct socket_shutdown_operation : public detail::socket_operation<socket_shutdown_operation>
{
    socket_shutdown_operation(socket& s)
        : socket_operation<socket_shutdown_operation>()
        , socket_(s.internal())
    {}

private:
    friend class detail::socket_operation<socket_shutdown_operation>;
    bool begin()
    {
        auto r = socket_.shutdown(ptl::experimental::asio::detail::shutdown_read_write);
        if (r.is_error()) {
            if (r.error().value() == EINPROGRESS) {
                // we need notification
                socket_.start_io(ptl::experimental::asio::io_kind::write, this);
                return false;
            }
            ec_ = r.error();
        }
        return true;
    }

    void get_return() {}

    void work() override
    {
        auto r = socket_.shutdown(ptl::experimental::asio::detail::shutdown_read_write);
        if (r.is_error()) {
            ec_ = r.error();
        }
        resume();
    }

    socket_internal& socket_;
};

} // namespace ptl::experimental::asio