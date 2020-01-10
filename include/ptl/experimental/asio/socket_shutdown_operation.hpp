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
        int r = socket_.shutdown(ptl::asio::detail::shutdown_read_write);
        if (r < 0) {
            int e = errno;
            if (e == EINPROGRESS) {
                // we need notification
                socket_.start_io(ptl::asio::io_kind::write, this);
                return false;
            }
            assert(false);
        }
        return true;
    }

    void get_return() {}

    void work() override
    {
        int r = socket_.shutdown(ptl::asio::detail::shutdown_read_write);
        if (r < 0) {
            ec_ = std::error_code(errno, std::system_category());
        } else {
            ec_ = std::error_code();
        }
        resume();
    }

    socket_internal& socket_;
};

} // namespace ptl::experimental::asio