#pragma once

#include "socket.hpp"
#include "detail/io_operation.hpp"

namespace ptl::experimental::coroutine::asio {

struct file_read_operation : public detail::io_xfer_operation<file_read_operation>
{
    file_read_operation(file& f, void* buffer, size_t sz, bool all)
        : io_xfer_operation<file_read_operation>()
        , file_(f.internal()), buffer_(static_cast<uint8_t*>(buffer)), size_(sz), received_(0), all_(all)
    {
        
    }

private:
    friend class detail::io_operation<file_read_operation>;
    bool begin()
    {
        int r = file_.read(buffer_, size_, 0);
        if (r < 0) {
            int e = errno;
            if (e == EAGAIN || e == EWOULDBLOCK) {
                // we need notification
                socket_.start_io(ptl::experimental::coroutine::asio::io_kind::read, this);
                return false;
            }
            assert(false);
        }
        received_ = r;
        if (all_ && received_ < size_) {
            socket_.start_io(ptl::experimental::coroutine::asio::io_kind::read, this);
            return false;
        }
        return true;
    }

    // called when io_service says there is something to do
    void work() override
    {
        int r = file_.read(buffer_ + received_, size_ - received_, 0);
        if (r < 0) {
            // handle error
            assert(false);
        }

        received_ += r;
        if (all_ && received_ < size_) {
            socket_.start_io(ptl::experimental::coroutine::asio::io_kind::read, this);
            return;
        }
        transferred_ = received_;
        ec_ = std::error_code();
        resume();
    }


    file_internal& socket_;
    uint8_t* buffer_;
    size_t size_;
    size_t received_;
    bool all_;
};

}