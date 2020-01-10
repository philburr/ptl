#pragma once

#include <unistd.h>
#include <sys/syscall.h>
#include <sys/time.h>
#include <linux/futex.h>
#include <atomic>
#include <cstdint>
#include <cassert>

namespace {
namespace local {
// No futex() function provided by libc.
// Wrap the syscall ourselves here.
int futex(int *UserAddress, int FutexOperation, int Value, const struct timespec *timeout, int *UserAddress2,
          int Value3)
{
    return syscall(SYS_futex, UserAddress, FutexOperation, Value, timeout, UserAddress2, Value3);
}
} // namespace local
} // namespace

namespace ptl::sync {

namespace detail {

// Platform sepcific
class linux_manual_reset_event
{
public:
    linux_manual_reset_event()
        : state_(0)
    {}

    void set()
    {
        state_.store(1, std::memory_order_release);

        [[maybe_unused]] int count_awaken =
            local::futex(reinterpret_cast<int *>(&state_), FUTEX_WAKE_PRIVATE, INT32_MAX, nullptr, nullptr, 0);
        assert(count_awaken >= 0);
    }

    void reset()
    {
        state_.store(0, std::memory_order_relaxed);
    }

    void wait()
    {
        int old = state_.load(std::memory_order_acquire);
        while (old == 0) {
            int result = local::futex(reinterpret_cast<int *>(&state_), FUTEX_WAIT_PRIVATE, old, nullptr, nullptr, 0);
            if (result == -1 and errno == EAGAIN) {
                return;
            }
            old = state_.load(std::memory_order_acquire);
        }
    }

private:
    std::atomic<int> state_;
};
using platform_manual_reset_event = linux_manual_reset_event;

} // namespace detail

class manual_reset_event : private detail::platform_manual_reset_event
{
public:
    using detail::platform_manual_reset_event::linux_manual_reset_event;
    using detail::platform_manual_reset_event::set;
    using detail::platform_manual_reset_event::reset;
    using detail::platform_manual_reset_event::wait;
};

} // namespace ptl::sync