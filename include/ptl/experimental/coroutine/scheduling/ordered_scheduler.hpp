#pragma once

#if !defined(__clang__)
#error Unsupported compiler
#endif
#include <vector>
#include <experimental/coroutine>
#include "ptl/containers/bounded_ring_buffer.hpp"

namespace ptl::experimental::coroutine {

template<size_t N = 25>
class OrderedScheduler
{
public:
    OrderedScheduler() noexcept
        : noop_(std::experimental::noop_coroutine())
    {

    }

private:
    struct schedule_operation
    {
        explicit schedule_operation(OrderedScheduler& s)
            : scheduler_(s)
        {}

        bool await_ready() noexcept
        {
            return false;
        }

        std::experimental::coroutine_handle<> await_suspend(
            std::experimental::coroutine_handle<> awaiting_coroutine) noexcept
        {
            return scheduler_.exchange_next(awaiting_coroutine);
        }

        void await_resume() noexcept
        {}

    private:
        OrderedScheduler& scheduler_;
    };
    friend struct schedule_operation;

    std::experimental::coroutine_handle<> exchange_next(std::experimental::coroutine_handle<> handle)
    {
        if (auto h = ring_buffer_.exchange(handle)) {
            return *h;
        }
        return noop_;
    }

    const std::experimental::coroutine_handle<> noop_;
    ptl::bounded_ring_buffer<std::experimental::coroutine_handle<>, N> ring_buffer_;
};

} // namespace ptl::experimental::coroutine