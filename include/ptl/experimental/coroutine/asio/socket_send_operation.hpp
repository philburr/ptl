#pragma once

#include "socket.hpp"
#include "ptl/experimental/coroutine/io_service/detail/io_operation.hpp"

namespace ptl::experimental::coroutine::asio {

struct socket_send_operation : iosvc::detail::io_xfer_operation<socket_send_operation>, iosvc::io_service_operation
{
    socket_send_operation(socket& s, const void* buffer, size_t sz) noexcept
        : io_xfer_operation<socket_send_operation>()
        , socket_(s.internal())
        , buffer_(static_cast<const uint8_t*>(buffer))
        , size_(sz)
        , sent_(0)
    {}

private:
    friend class iosvc::detail::io_operation<socket_send_operation>;
    bool begin()
    {
        int r = socket_.send(buffer_, size_, 0);
        if (r < 0) {
            int e = errno;
            if (e == EAGAIN || e == EWOULDBLOCK) {
                // we need notification
                socket_.start_io(iosvc::io_kind::write, this);
                return false;
            }
            ec_= ptl::error_code{ errno };
        }
        return true;
    }

    void work() override
    {
        int r = socket_.send(buffer_, size_, 0);
        if (r < 0) {
            ec_ = ptl::error_code{ errno };
        } else {
            sent_ = size_;
            transferred_ = sent_;
        }
        resume();
    }

    socket_internal& socket_;
    const uint8_t* buffer_;
    size_t size_;
    size_t sent_;
};

} // namespace ptl::experimental::coroutine::asio