#pragma once
#include <experimental/coroutine>
#include "is_awaiter.hpp"
#include <tuple>

namespace ptl::experimental::coroutine::detail {

template<typename CONTAINER>
class wait_all_awaitable;

template<>
class wait_all_awaitable<std::tuple<>> {
public:
    wait_all_awaitable() noexcept {}
    explicit wait_all_awaitable(std::tuple<>) noexcept {}

    bool await_ready() const noexcept { return true; }
    void await_suspend(std::experimental::coroutine_handle<>) noexcept {}
    std::tuple<> await_resume() const noexcept { return {}; }
};

template<typename... TASKS>
class wait_all_awaitable<std::tuple<TASKS...>> {
public:
    explicit wait_all_awaitable(TASKS&&... tasks) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<TASKS>...>)
        : counter_(sizeof...(TASKS))
        , tasks_(std::move(tasks)...)
    {}
    explicit wait_all_awaitable(std::tuple<TASKS...>&& tasks) noexcept(std::is_nothrow_move_constructible_v<std::tuple<TASKS...>>)
        : counter_(sizeof...(TASKS))
        , tasks_(std::move(tasks))
    {}

    wait_all_awaitable(wait_all_awaitable&& other) noexcept
        : counter_(other.counter_)
        , tasks_(std::move(other.tasks_))
    {}

    auto operator co_await() & noexcept
    {
        struct awaiter : awaiter_base {
            std::tuple<TASKS...>& await_resume() const noexcept
            {
                return this->awaitable_.tasks_;
            }
        };
        return awaiter{ *this };
    }

    auto operator co_await() && noexcept
    {
        struct awaiter : awaiter_base {
            std::tuple<TASKS...>&& await_resume() const noexcept
            {
                return std::move(this->awaitable_.tasks_);
            }
        };
        return awaiter{ *this };
    }

private:
    struct awaiter_base {
        awaiter_base(wait_all_awaitable& awaitable) noexcept
            : awaitable_(awaitable)
        {}

        bool await_ready() const noexcept
        {
            return awaitable_.is_ready();
        }
        bool await_suspend(std::experimental::coroutine_handle<> coroutine) noexcept
        {
            return awaitable_.begin_await(coroutine);
        }

    protected:
        wait_all_awaitable& awaitable_;
    };

    bool is_ready() const noexcept
    {
        return counter_.is_ready();
    }
    bool begin_await(std::experimental::coroutine_handle<> coroutine) noexcept
    {
        start_tasks(std::make_integer_sequence<size_t, sizeof...(TASKS)>{});
        return counter_.begin_await(coroutine);
    }

    template<size_t... INDICES>
    void start_tasks(std::integer_sequence<size_t, INDICES...>) noexcept
    {
        (void)std::initializer_list<int>{
            (std::get<INDICES>(tasks_).start(counter_), 0)...
        };
    }

    std::tuple<TASKS...> tasks_;
    wait_all_counter counter_;
};

}