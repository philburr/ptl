#pragma once

#include "socket.hpp"
#include "detail/io_operation.hpp"

namespace ptl::experimental::coroutine::asio {

struct socket_connect_operation : public detail::io_operation<socket_connect_operation>
{
    socket_connect_operation(socket& s, const ptl::experimental::coroutine::asio::ip_endpoint& address)
        : io_operation<socket_connect_operation>()
        , socket_(s.internal())
        , address_(address)
    {}

private:
    friend class detail::io_operation<socket_connect_operation>;
    bool begin()
    {
        auto r = socket_.connect(address_);
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

    void work() override
    {
        auto r = socket_.connect(address_);
        if (r.is_error()) {
            ec_ = r.error();
        }
        resume();
    }

    void get_return() const noexcept
    {
    }

    socket_internal& socket_;
    ptl::experimental::coroutine::asio::ip_endpoint address_;
};

} // namespace ptl::experimental::coroutine::asio