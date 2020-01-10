#pragma once

#include "socket.hpp"
#include "detail/socket_operation.hpp"

namespace ptl::experimental::asio {

struct socket_accept_operation : public detail::socket_operation<socket_accept_operation>
{
    socket_accept_operation(socket& s)
        : socket_operation<socket_accept_operation>()
        , socket_(s.internal())
        , accepted_socket_(socket_internal::internal_create(s.internal()))
    {}

private:
    friend class detail::socket_operation<socket_accept_operation>;
    bool begin()
    {
        int r = socket_.accept();
        if (r < 0) {
            int e = errno;
            if (e == EINPROGRESS) {
                // we need notification
                socket_.start_io(ptl::asio::io_kind::read, this);
                return false;
            }
            assert(false);
        }
        accepted_socket_ = socket_internal::internal_create(socket_, r);
        return true;
    }

    void work() override
    {
        int r = socket_.accept();
        if (r < 0) {
            ec_ = std::error_code(errno, std::system_category());
        } else {
            ec_ = std::error_code();
            accepted_socket_ = socket_internal::internal_create(socket_, r);
        }
        resume();
    }

    decltype(auto) get_return()
    {
        return std::move(accepted_socket_);
    }

    socket_internal& socket_;
    socket accepted_socket_;
};

} // namespace ptl::experimental::asio