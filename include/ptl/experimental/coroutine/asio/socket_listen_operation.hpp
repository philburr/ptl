#pragma once

#include "socket.hpp"
#include "detail/socket_operation.hpp"

namespace ptl::experimental::asio {

struct socket_listen_operation : public detail::socket_operation<socket_listen_operation>
{
    socket_listen_operation(socket& s)
        : socket_operation<socket_listen_operation>()
        , socket_(s.internal())
    {}

private:
    friend class detail::socket_operation<socket_listen_operation>;
    bool begin()
    {
        int r = socket_.listen();
        if (r < 0) {
            int e = errno;
            if (e == EINPROGRESS || e == EAGAIN) {
                // we need notification
                socket_.start_io(ptl::experimental::asio::io_kind::read, this);
                return false;
            }
            assert(false);
        }
        have_listened_ = true;

        r = socket_.get_local();
        if (r < 0) {
            int e = errno;
            if (e == EINPROGRESS || e == EAGAIN) {
                // we need notification
                socket_.start_io(ptl::experimental::asio::io_kind::read, this);
                return false;
            }
            assert(false);
        }
        return true;
    }

    void work() override
    {
        if (!have_listened_) {
            int r = socket_.accept();
            if (r < 0) {
                ec_ = std::error_code(errno, std::system_category());
                resume();
                return;
            }
            have_listened_ = true;
        }

        {
            int r = socket_.get_local();
            if (r < 0) {
                int e = errno;
                if (e == EINPROGRESS || e == EAGAIN) {
                    // we need notification
                    socket_.start_io(ptl::experimental::asio::io_kind::read, this);
                    return;
                }
                ec_ = std::error_code(errno, std::system_category());
                resume();
                return;
            }
        }
        ec_ = {};
        resume();
    }

    void get_return() {}

    socket_internal& socket_;
    bool have_listened_ = false;
};

} // namespace ptl::experimental::asio