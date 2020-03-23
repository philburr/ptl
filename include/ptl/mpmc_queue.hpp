#pragma once
#include "ptl/aligned_allocator.hpp"

namespace ptl {

enum class Lockless {
    LOCKFULL,
    MPMC,
};

template<typename T, Lockless KIND>
struct queue {};

template<typename T>
struct queue<T, Lockless::MPMC> {
    static_assert(std::is_nothrow_copy_assignable_v<T> || std::is_nothrow_move_assignable_v<T>);
    static_assert(std::is_nothrow_destructible_v<T>);

public:
    explicit queue(const size_t capacity)
        : slots_(capacity)
    {
    }
    ~queue() noexcept = default;
    queue(const queue&) = delete;
    queue& operator=(const queue&) = delete;

    template<typename... Args>
    void emplace(Args&&... args) noexcept
    {
        static_assert(std::is_nothrow_constructible_v<T, Args&&...>);

        const auto ticket = head_.fetch_add(1);
        auto& slot = slots_[idx(ticket)];

        while (acquire_turn(ticket) != slot.sequence_.load(std::memory_order_acquire)) {
            ;
        }

        slot.construct(std::forward<Args>(args)...);
        slot.sequence_.store(release_turn(ticket), std::memory_order_release);
    }

    template<typename... Args>
    bool try_emplace(Args&&... args) noexcept{
        static_assert(std::is_nothrow_constructible_v<T, Args&&...>);

        const auto ticket = head_.load(std::memory_order_acquire);
        for(;;) {
            auto& slot = slots_[idx(ticket)];
            if (acquire_turn(ticket) == slot.sequence_.load(std::memory_order_acquire)) {
                if (head_.compare_exchange_strong(ticket, ticket + 1)) {
                    slot.construct(std::forward<Args>(args)...);
                    slot.sequence_.store(release_turn(ticket), std::memory_order_release);
                    return true;
                }
            } else {
                const auto prev_ticket = ticket;
                ticket = head_.load(std::memory_order_acquire);
                if (ticket == prev_ticket) {
                    return false;
                }
            }
        }
    }

    void push(const T& v) noexcept
    {
        static_assert(std::is_nothrow_copy_constructible_v<T>);
        emplace(v);
    }

    template<typename P, typename = typename std::enable_if<std::is_nothrow_constructible_v<T, P&&>>::type>
    void push(P&& v) noexcept
    {
        emplace(std::forward<P>(v));
    }

    bool try_push(const T& v) noexcept
    {
        static_assert(std::is_nothrow_copy_constructible_v<T>);
        try_emplace(v);
    }

    template<typename P, typename = typename std::enable_if<std::is_nothrow_constructible_v<T, P&&>>::type>
    bool try_push(P&& v) noexcept
    {
        try_emplace(std::forward<P>(v));
    }

    void pop(T& v) noexcept
    {
        const auto ticket = tail_.fetch_add(1);
        auto& slot = slots_[idx(ticket)];

        while (release_turn(ticket) != slot.sequence_.load(std::memory_order_acquire)) {
            ;
        }

        v = slot.move();
        slot.destruct();
        slot.sequence_.store(release_turn(ticket) + 1, std::memory_order_release);
    }

    bool try_pop(T& v) noexcept
    {
        auto ticket = tail_.load(std::memory_order_acquire);
        for(;;) {
            auto& slot = slots_[idx(ticket)];
            if (release_turn(ticket) == slot.sequence_.load(std::memory_order_acquire)) {
                if (tail_.compare_exchange_strong(ticket, ticket + 1)) {
                    v = slot.move();
                    slot.destruct();
                    slot.sequence_.store(release_turn(ticket) + 1, std::memory_order_release);
                    return true;
                }
            } else {
                const auto prev_ticket = ticket;
                ticket = tail_.load(std::memory_order_acquire);
                if (ticket == prev_ticket) {
                    return false;
                }
            }
        }
    }

private:
    size_t idx(size_t i) {
        return i % slots_.size();
    }
    size_t turn(size_t i) {
        return i / slots_.size();
    }
    size_t acquire_turn(size_t i) {
        return 2 * turn(i);
    }
    size_t release_turn(size_t i) {
        return 2 * turn(i) + 1;
    }

    static constexpr size_t alignment = Alignment::CACHE_LINE;
    struct slot {
        using storage = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

        std::atomic<size_t> sequence_ = {0};
        storage             data_;
        alignas(Alignment::CACHE_LINE) size_t padding_[0];

        ~slot() noexcept
        {
            if (sequence_ & 1) {
                destruct();
            }
        }

        template<typename... Args>
        void construct(Args&&... args) noexcept
        {
            static_assert(std::is_nothrow_constructible<T, Args&&...>::value);
            new(&data_) T(std::forward<Args>(args)...);
        }

        void destruct() noexcept
        {
            static_assert(std::is_nothrow_destructible_v<T>);
            reinterpret_cast<T*>(&data_)->~T();
        }

        T&& move() noexcept
        {
            return reinterpret_cast<T&&>(data_);
        }
    };
    static_assert(sizeof(slot) <= Alignment::CACHE_LINE, "Slot should really fit in a cache line");
    std::vector<slot, aligned_allocator<slot, Alignment::CACHE_LINE>> slots_;

    alignas(Alignment::CACHE_LINE) std::atomic_size_t head_ = {0};
    alignas(Alignment::CACHE_LINE) std::atomic_size_t tail_ = {0};
};

}