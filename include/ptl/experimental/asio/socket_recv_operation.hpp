#pragma once

#include "socket.hpp"
#include "detail/socket_operation.hpp"

namespace ptl::experimental::asio {

struct socket_recv_operation : public detail::socket_xfer_operation<socket_recv_operation>
{
    socket_recv_operation(socket& s, void* buffer, size_t sz, bool all)
        : socket_xfer_operation<socket_recv_operation>()
        , socket_(s.internal()), buffer_(static_cast<uint8_t*>(buffer)), size_(sz), received_(0), all_(all)
    {
        
    }

private:
    friend class detail::socket_operation<socket_recv_operation>;
    bool begin()
    {
        int r = socket_.recv(buffer_, size_, 0);
        if (r < 0) {
            int e = errno;
            if (e == EAGAIN || e == EWOULDBLOCK) {
                // we need notification
                socket_.start_io(ptl::asio::io_kind::read, this);
                return false;
            }
            assert(false);
        }
        received_ = r;
        if (all_ && received_ < size_) {
            socket_.start_io(ptl::asio::io_kind::read, this);
            return false;
        }
        return true;
    }

    // called when io_service says there is something to do
    void work() override
    {
        int r = socket_.recv(buffer_ + received_, size_ - received_, 0);
        if (r < 0) {
            // handle error
            assert(false);
        }

        received_ += r;
        if (all_ && received_ < size_) {
            socket_.start_io(ptl::asio::io_kind::read, this);
            return;
        }
        transferred_ = received_;
        ec_ = std::error_code();
        resume();
    }


    socket_internal& socket_;
    uint8_t* buffer_;
    size_t size_;
    size_t received_;
    bool all_;
};

}