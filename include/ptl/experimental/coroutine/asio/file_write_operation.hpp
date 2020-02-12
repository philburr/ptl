#pragma once

#include "file.hpp"
#include "detail/io_operation.hpp"

namespace ptl::experimental::coroutine::asio {

struct file_write_operation : public detail::io_xfer_operation<file_write_operation>
{
    file_write_operation(file& f, const void* buffer, size_t sz)
        : io_xfer_operation<file_write_operation>()
        , file_(f.internal())
        , buffer_(static_cast<const uint8_t*>(buffer))
        , size_(sz)
        , sent_(0)
    {}

private:
    friend class detail::io_operation<file_write_operation>;
    bool begin()
    {
        int r = file_.write(buffer_, size_, 0);
        if (r < 0) {
            int e = errno;
            if (e == EAGAIN || e == EWOULDBLOCK) {
                // we need notification
                socket_.start_io(ptl::experimental::coroutine::asio::io_kind::write, this);
                return false;
            }
            assert(false);
        }
        return true;
    }

    void work() override
    {
        int r = file_.write(buffer_, size_, 0);
        assert(r == size_);
        sent_ = size_;
        transferred_ = sent_;
        ec_ = std::error_code();
        resume();
    }

    file_internal& file_;
    const uint8_t* buffer_;
    size_t size_;
    size_t sent_;
};

} // namespace ptl::experimental::coroutine::asio