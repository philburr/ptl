#pragma once

#include "socket.hpp"
#include "ptl/experimental/coroutine/io_service/detail/io_operation.hpp"

namespace ptl::experimental::coroutine::asio {

struct socket_recv_operation : iosvc::detail::io_xfer_operation<socket_recv_operation>, iosvc::io_service_operation
{
    socket_recv_operation(socket& s, void* buffer, size_t sz, bool all) noexcept
        : io_xfer_operation<socket_recv_operation>()
        , socket_(s.internal()), buffer_(static_cast<uint8_t*>(buffer)), size_(sz), received_(0), all_(all)
    {
        
    }

private:
    friend class iosvc::detail::io_operation<socket_recv_operation>;
    bool begin()
    {
        int r = socket_.recv(buffer_, size_, 0);
        if (r < 0) {
            int e = errno;
            if (e == EAGAIN || e == EWOULDBLOCK) {
                // we need notification
                socket_.start_io(iosvc::io_kind::read, this);
                return false;
            }
            ec_ = ptl::error_code{ errno };
            return true;
        }
        received_ = r;
        if (all_ && received_ < size_) {
            socket_.start_io(iosvc::io_kind::read, this);
            return false;
        }
        return true;
    }

    // called when io_service says there is something to do
    void work() override
    {
        int r = socket_.recv(buffer_ + received_, size_ - received_, 0);
        if (r < 0) {
            ec_ = ptl::error_code{ errno };
        } else {
            received_ += r;
            if (r > 0 && all_ && received_ < size_) {
                socket_.start_io(iosvc::io_kind::read, this);
                return;
            }
            transferred_ = received_;
        }
        resume();
    }


    socket_internal& socket_;
    uint8_t* buffer_;
    size_t size_;
    size_t received_;
    bool all_;
};

}