#pragma once
#include <experimental/coroutine>
#include <atomic>
#include <cstdint>

namespace ptl::experimental::coroutine::detail {

struct wait_all_counter {

    wait_all_counter(size_t initial) noexcept
        : count_(initial + 1)  // plus one for inital calls to begin_await and one call to notify_completed
        , coroutine_(nullptr)
    {}

    bool is_ready() const noexcept{
        return coroutine_ != nullptr;
    }

    bool begin_await(std::experimental::coroutine_handle<> coroutine) noexcept
    {
        coroutine_ = coroutine;
        return count_.fetch_sub(1, std::memory_order_acq_rel) > 1;
    }

    void notify_complete() noexcept {
        if (count_.fetch_sub(1, std::memory_order_acq_rel) == 1) {
            coroutine_.resume();
        }
    }


private:
    std::experimental::coroutine_handle<> coroutine_;
    std::atomic<size_t> count_;
};

}