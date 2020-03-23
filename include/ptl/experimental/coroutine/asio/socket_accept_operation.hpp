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
        auto r = socket_.accept();
        if (r.is_error()) {
            if (r.error().value() == EINPROGRESS || r.error().value() == EAGAIN) {
                // we need notification
                socket_.start_io(ptl::experimental::asio::io_kind::read, this);
                return false;
            }
            ec_ = r.error();
        } else {
            accepted_socket_ = socket_internal::internal_create(socket_, r.value());
        }
        return true;
    }

    void work() override
    {
        auto r = socket_.accept();
        if (r.is_error()) {
            ec_ = r.error();
        } else {
            accepted_socket_ = socket_internal::internal_create(socket_, r.value());
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