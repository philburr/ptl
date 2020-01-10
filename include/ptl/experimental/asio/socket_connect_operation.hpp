#pragma once

#include "socket.hpp"
#include "detail/socket_operation.hpp"

namespace ptl::experimental::asio {

struct socket_connect_operation : public detail::socket_operation<socket_connect_operation>
{
    socket_connect_operation(socket& s, const ptl::asio::ip_endpoint& address)
        : socket_operation<socket_connect_operation>()
        , socket_(s.internal())
        , address_(address)
    {}

private:
    friend class detail::socket_operation<socket_connect_operation>;
    bool begin()
    {
        int r = socket_.connect(address_);
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

    void work() override
    {
        int r = socket_.connect(address_);
        if (r < 0) {
            ec_ = std::error_code(errno, std::system_category());
        } else {
            ec_ = std::error_code();
        }
        resume();
    }

    socket_internal& socket_;
    ptl::asio::ip_endpoint address_;
};

} // namespace ptl::experimental::asio