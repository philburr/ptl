#pragma once
#if !defined(__clang__)
#error Unsupported compiler
#endif
#include <experimental/coroutine>
#include <cstdio>

#include "awaitable_traits.hpp"
#include "ptl/sync/event.hpp"

namespace ptl::experimental::coroutine {
namespace detail {

template <typename T>
class async_wait_task;
template <typename T>
class sync_wait_task;

template<typename T, typename DERIVED>
class wait_task_promise_base
{
protected:
    using coroutine_handle_t = std::experimental::coroutine_handle<DERIVED>;
    using reference = T&&;

    wait_task_promise_base() noexcept = default;

    void base_start()
    {
        auto coro = coroutine_handle_t::from_promise(*static_cast<DERIVED*>(this));
        coro.resume();
    }

public:
    auto get_return_object() noexcept
    {
        return coroutine_handle_t::from_promise(*static_cast<DERIVED*>(this));
    }

    auto initial_suspend() noexcept
    {
        return typename DERIVED::initial_suspend_type{};
    }

    auto final_suspend() noexcept
    {
        return typename DERIVED::final_suspend_type{};
    }

    auto yield_value(reference result) noexcept
    {
        result_ = std::addressof(result);
        return final_suspend();
    }

    void return_void() noexcept
    {
        assert(false);
    }

    void unhandled_exception() noexcept
    {
        exception_ = std::current_exception();
    }

    reference result()
    {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
        return static_cast<reference>(*result_);
    }

private:
    std::remove_reference_t<T>* result_;
    std::exception_ptr exception_;
};

template <typename DERIVED>
class wait_task_promise_base<void, DERIVED>
{
protected:
    using coroutine_handle_t = std::experimental::coroutine_handle<DERIVED>;

    wait_task_promise_base() noexcept = default;

    void base_start()
    {
        auto coro = coroutine_handle_t::from_promise(*static_cast<DERIVED*>(this));
        coro.resume();
    }

public:
    auto get_return_object() noexcept
    {
        return coroutine_handle_t::from_promise(*static_cast<DERIVED*>(this));
    }

    auto initial_suspend() noexcept
    {
        return typename DERIVED::initial_suspend_type{};
    }

    auto final_suspend() noexcept
    {
        return typename DERIVED::final_suspend_type{};
    }

    void return_void() noexcept
    {}

    void unhandled_exception() noexcept
    {
        exception_ = std::current_exception();
    }

    void result()
    {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
    }

private:
    std::exception_ptr exception_;
};



template <typename T>
class async_wait_task_promise final : public wait_task_promise_base<T, async_wait_task_promise<T>>
{
    using super = wait_task_promise_base<T, async_wait_task_promise<T>>;
private:
    class completion_notifier
    {
    public:
        bool await_ready() const noexcept
        {
            return false;
        }

        void await_suspend(typename super::coroutine_handle_t coroutine) const noexcept
        {
            coroutine.promise().event_->set();
        }

        void await_resume() noexcept
        {
        }
    };

    ptl::sync::manual_reset_event* event_;

public:
    using initial_suspend_type = std::experimental::suspend_always;
    using final_suspend_type = completion_notifier;

    void start(ptl::sync::manual_reset_event& event)
    {
        event_ = &event;
        this->base_start();
    }

};

template <typename T>
class sync_wait_task_promise final : public wait_task_promise_base<T, sync_wait_task_promise<T>>
{
    using super = wait_task_promise_base<T, async_wait_task_promise<T>>;

public:
    using initial_suspend_type = std::experimental::suspend_always;
    using final_suspend_type = std::experimental::suspend_always;

    void start()
    {
        this->base_start();
    }
};

template <typename T>
class async_wait_task final
{
public:
    using promise_type = async_wait_task_promise<T>;
    using coroutine_handle_t = std::experimental::coroutine_handle<promise_type>;

    async_wait_task(coroutine_handle_t coroutine) noexcept
        : coroutine_(coroutine)
    {
    }

    async_wait_task(async_wait_task&& other) noexcept
        : coroutine_(std::exchange(other.coroutine_, coroutine_handle_t{}))
    {}

    ~async_wait_task()
    {
        if (coroutine_) {
            coroutine_.destroy();
        }
    }

    async_wait_task(const async_wait_task&) = delete;
    async_wait_task& operator=(const async_wait_task&) = delete;

    void start(ptl::sync::manual_reset_event& event) noexcept
    {
        coroutine_.promise().start(event);
    }

    decltype(auto) result()
    {
        return coroutine_.promise().result();
    }

private:
    coroutine_handle_t coroutine_;
};

template <typename T>
class sync_wait_task final
{
public:
    using promise_type = sync_wait_task_promise<T>;
    using coroutine_handle_t = std::experimental::coroutine_handle<promise_type>;

    sync_wait_task(coroutine_handle_t coroutine) noexcept
        : coroutine_(coroutine)
    {
    }

    sync_wait_task(sync_wait_task&& other) noexcept
        : coroutine_(std::exchange(other.coroutine_, coroutine_handle_t{}))
    {}

    ~sync_wait_task()
    {
        if (coroutine_) {
            coroutine_.destroy();
        }
    }

    sync_wait_task(const sync_wait_task&) = delete;
    sync_wait_task& operator=(const sync_wait_task&) = delete;

    void start() noexcept
    {
        coroutine_.promise().start();
    }

    decltype(auto) result()
    {
        return coroutine_.promise().result();
    }

private:
    coroutine_handle_t coroutine_;
};

template <typename AWAITABLE, typename RESULT = typename awaitable_traits<AWAITABLE&&>::await_result_t,
          std::enable_if_t<!std::is_void_v<RESULT>, int> = 0>
async_wait_task<RESULT> make_async_wait_task(AWAITABLE&& awaitable)
{
    co_yield co_await std::forward<AWAITABLE>(awaitable);
}

template <typename AWAITABLE, typename RESULT = typename awaitable_traits<AWAITABLE&&>::await_result_t,
          std::enable_if_t<std::is_void_v<RESULT>, int> = 0>
async_wait_task<RESULT> make_async_wait_task(AWAITABLE&& awaitable)
{
    co_return co_await std::forward<AWAITABLE>(awaitable);
}

template <typename AWAITABLE, typename RESULT = typename awaitable_traits<AWAITABLE&&>::await_result_t,
          std::enable_if_t<!std::is_void_v<RESULT>, int> = 0>
sync_wait_task<RESULT> make_sync_wait_task(AWAITABLE&& awaitable)
{
    co_yield co_await std::forward<AWAITABLE>(awaitable);
}

template <typename AWAITABLE, typename RESULT = typename awaitable_traits<AWAITABLE&&>::await_result_t,
          std::enable_if_t<std::is_void_v<RESULT>, int> = 0>
sync_wait_task<RESULT> make_sync_wait_task(AWAITABLE&& awaitable)
{
    co_return co_await std::forward<AWAITABLE>(awaitable);
}


} // namespace detail

template <typename AWAITABLE>
auto async_wait(AWAITABLE&& awaitable) -> typename awaitable_traits<AWAITABLE&&>::await_result_t
{
    auto task = detail::make_async_wait_task<AWAITABLE>(std::forward<AWAITABLE>(awaitable));

    ptl::sync::manual_reset_event event;
    task.start(event);
    event.wait();
    return task.result();
}

template <typename AWAITABLE>
auto sync_wait(AWAITABLE&& awaitable) -> typename awaitable_traits<AWAITABLE&&>::await_result_t
{
    auto task = detail::make_sync_wait_task<AWAITABLE>(std::forward<AWAITABLE>(awaitable));

    task.start();
    return task.result();
}


} // namespace ptl::experimental::coroutine