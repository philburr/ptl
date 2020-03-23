#pragma once
#include <array>
#include <optional>

#include "ptl/synchronize.hpp"
#include "ptl/scope_guard.hpp"

namespace ptl {

template<typename T, size_t N, typename LOCKING = null_lock>
class bounded_ring_buffer
{
public:
    constexpr bounded_ring_buffer() noexcept
        : head_(0)
        , tail_(0)
    {}

    void push(T&& item) noexcept
    {
        synchronize(lock_, [&]() {
            assert(free() > 0);
            SCOPE_EXIT({ if (tail_ == total_size_) tail_ = 0; });
            std::exchange(buffer_[tail_++], item);
        });
    }

    std::optional<T> pop() noexcept
    {
        synchronize(lock_, [&]() {
            if (available() > 0) {
                SCOPE_EXIT({ if (head_ == total_size_) head_ = 0; });
                return std::exchange(buffer_[head_++], {});
            }
            return std::nullopt;
        });
    }

    std::optional<T> exchange(T&& item) noexcept
    {
        synchronize(lock_, [&]() {
            if (available() > 0) {
                if (head_ == tail_) {
                    return std::move(item);
                }

                SCOPE_EXIT({
                    if (head_ == total_size_) head_ = 0; 
                    if (tail_ == total_size_) tail_ = 0;
                });
                std::exchange(buffer_[tail_++], item);
                return std::exchange(buffer_[head_++], {});
            } else {
                SCOPE_EXIT({ if (tail_ == total_size_) tail_ = 0; });
                std::exchange(buffer_[tail_++], item);
            }
            return std::nullopt;
        });
    }

    size_t count() noexcept 
    {
        synchronize(lock_, [&]() {
            return available();
        });
    }

private:
    size_t available() noexcept
    {
        if (tail_ < head_) {
            return total_size_ + head_ - tail_;
        }
        return tail_ - head_;
    }
    size_t free() noexcept
    {
        return total_size_ - available();
    }

    static constexpr size_t total_size_ = N - 1;


    LOCKING lock_;
    std::size_t head_;
    std::size_t tail_;
    std::array<T, N> buffer_;
};

}