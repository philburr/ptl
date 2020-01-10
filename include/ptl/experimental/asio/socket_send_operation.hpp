#pragma once

#include "socket.hpp"
#include "detail/socket_operation.hpp"

namespace ptl::experimental::asio {

struct socket_send_operation : public detail::socket_xfer_operation<socket_send_operation>
{
    socket_send_operation(socket& s, const void* buffer, size_t sz)
        : socket_xfer_operation<socket_send_operation>()
        , socket_(s.internal())
        , buffer_(static_cast<const uint8_t*>(buffer))
        , size_(sz)
        , sent_(0)
    {}

private:
    friend class detail::socket_operation<socket_send_operation>;
    bool begin()
    {
        int r = socket_.send(buffer_, size_, 0);
        if (r < 0) {
            int e = errno;
            if (e == EAGAIN || e == EWOULDBLOCK) {
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
        int r = socket_.send(buffer_, size_, 0);
        assert(r == size_);
        sent_ = size_;
        transferred_ = sent_;
        ec_ = std::error_code();
        resume();
    }

    socket_internal& socket_;
    const uint8_t* buffer_;
    size_t size_;
    size_t sent_;
};

} // namespace ptl::experimental::asio