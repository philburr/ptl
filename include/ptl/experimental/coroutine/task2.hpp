#pragma once

#if !defined(__clang__)
#error Unsupported compiler
#endif
#include <experimental/coroutine>
#include "ptl/scope_guard.hpp"
#include "ptl/exceptable.hpp"

#include "ptl/experimental/coroutine/detail/schedule_awaitable.hpp"
#include "ptl/experimental/coroutine/ordered_scheduler.hpp"

namespace ptl::experimental::coroutine {

#define DEBUG_CORO 1
#if defined(DEBUG_CORO)
#include <cstdio>
#define SIZE_T_N size_t N
#define COMMA_SIZE_T_N , size_t N
#define TEMPLATE_SIZE_T_N template <size_t N>
#define BRACKET_N <N>
#define COMMA_N , N
#else
#define SIZE_T_N
#define COMMA_SIZE_T_N
#define TEMPLATE_SIZE_T_N
#define BRACKET_N
#define COMMA_N
#endif

template <typename T COMMA_SIZE_T_N>
class Task;

namespace detail {

TEMPLATE_SIZE_T_N
class TaskPromiseBase
{
public:
    auto initial_suspend() noexcept
    {
#if defined(DEBUG_CORO)
        std::printf("task_promise(%lu): initial_suspend\n", N);
#endif
        return std::experimental::suspend_always{};
    }

    auto final_suspend() noexcept
    {
#if defined(DEBUG_CORO)
        std::printf("task_promise(%lu): final_suspend\n", N);
#endif
        return final_awaitable{};
    }

    void set_continuation(std::experimental::coroutine_handle<> continuation) noexcept
    {
#if defined(DEBUG_CORO)
        std::printf("task_promise(%lu): will continue to %p\n", N, continuation.address());
#endif
        continuation_ = continuation;
        executor_ = execution::Executor::current;
    }

#if defined(DEBUG_CORO)
    size_t getN() const noexcept
    {
        return N;
    }
#endif

    void* operator new(std::size_t sz) {
        printf("promise new: %lu\n", sz);
        return malloc(sz);
    }
    void operator delete(void* ptr, std::size_t sz) {
        free(ptr);
    }

protected:
    TaskPromiseBase() noexcept
        : executor_(nullptr)
    {}

private:
    std::experimental::coroutine_handle<> continuation_;
    execution::Executor* executor_;

    friend struct final_awaitable;
    struct final_awaitable
    {
        bool await_ready() const noexcept
        {
            return false;
        }

        template <typename T>
        std::experimental::coroutine_handle<> await_suspend(std::experimental::coroutine_handle<T> coroutine) noexcept
        {
            auto& promise = coroutine.promise();
#if defined(DEBUG_CORO)
            std::printf("%lu: final_awaitable::await_suspend: continuing to %p\n", N, promise.continuation_.address());
#endif
            return coroutine.promise().continuation_;
        }

        void await_resume() noexcept
        {
#if defined(DEBUG_CORO)
            std::printf("%lu: final_awaitable::await_resume\n", N);
#endif

        }
    };
};

template <typename T COMMA_SIZE_T_N>
class TaskPromise final : public TaskPromiseBase BRACKET_N
{
public:
    TaskPromise() noexcept
    {
#if defined(DEBUG_CORO)
        std::printf("task_promise(%lu %p): ctor\n", N, this);
#endif
    }
    ~TaskPromise()
    {
#if defined(DEBUG_CORO)
        std::printf("task_promise(%lu): dtor\n", N);
#endif
    }

    void unhandled_exception() noexcept
    {
        value_ = ptl::exceptable<T>(std::current_exception());
    }

    template <typename VALUE_TYPE, typename = std::enable_if_t<std::is_convertible_v<VALUE_TYPE&&, T>>>
    void return_value(VALUE_TYPE&& value) noexcept(std::is_nothrow_constructible_v<T, VALUE_TYPE&&>)
    {
        value_ = ptl::exceptable(value);
    }

    Task<T COMMA_N> get_return_object() noexcept;

    T& result() &
    {
        return value_.value();
    }

    T&& result() &&
    {
        return std::move(value_.value());
    }

private:
    ptl::exceptable<T> value_;
};

// void specialization
template <SIZE_T_N>
class TaskPromise<void COMMA_N> final : public TaskPromiseBase BRACKET_N
{
public:
    TaskPromise() noexcept
    {}

    Task<void COMMA_N> get_return_object() noexcept;

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

template <typename T COMMA_SIZE_T_N>
class TaskPromise<T & COMMA_N> final : public TaskPromiseBase BRACKET_N
{
public:
    TaskPromise() noexcept = default;

    Task<T & COMMA_N> get_return_object() noexcept;

    void return_value(T& value) noexcept
    {
        value_ = std::addressof(value);
    }
    void unhandled_exception() noexcept
    {
        exception_ = std::current_exception();
    }

    T& result()
    {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
        return *value_;
    }

private:
    T* value_ = nullptr;
    std::exception_ptr exception_;
};

} // namespace detail

template <typename T COMMA_SIZE_T_N>
class Task
{
public:
    using promise_type = detail::TaskPromise<T COMMA_N>;
    using value_type = T;

    Task() noexcept
        : coroutine_(nullptr)
    {
#if defined(DEBUG_CORO)
        std::printf("task(%lu %lu): ctor()\n", N, sizeof(*this));
#endif
    }

    explicit Task(std::experimental::coroutine_handle<promise_type> coroutine) noexcept
        : coroutine_(coroutine)
    {
#if defined(DEBUG_CORO)
        std::printf("task(%lu %lu): ctor %p\n", N, sizeof(*this), coroutine.address());
#endif
    }

    Task(Task&& t) noexcept
        : coroutine_(t.coroutine_)
    {
        t.coroutine_ = nullptr;
    }

    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    ~Task()
    {
#if defined(DEBUG_CORO)
        std::printf("task(%lu): dtor\n", N);
#endif
        if (coroutine_) {
            coroutine_.destroy();
        }
    }

    Task& operator=(Task&& other) noexcept
    {
        if (std::addressof(other) != this) {
            if (coroutine_) {
                coroutine_.destroy();
            }
            coroutine_ = other.coroutine_;
            other.coroutine_ = nullptr;
        }
        return *this;
    }

    bool is_ready() const noexcept
    {
        return !coroutine_ || coroutine_.done();
    }

    auto operator co_await() const& noexcept
    {
#if defined(DEBUG_CORO)
        std::printf("task(%lu): co_await()&\n", N);
#endif
        struct awaitable : public awaitable_base
        {
            using awaitable_base::awaitable_base;

            decltype(auto) await_resume()
            {
#if defined(DEBUG_CORO)
                std::printf("task&::awaitable(%lu): await_resume()\n", N);
#endif
                if (!this->coroutine_) {
                    assert(false);
                    // throw broken_promise {};
                }
                printf("coro4: %p\n", this->coroutine_.address());
                return this->coroutine_.promise().result();
            }
        };

        return awaitable{coroutine_};
    }

    auto operator co_await() const&& noexcept
    {
#if defined(DEBUG_CORO)
        std::printf("task(%lu): co_await()&&\n", N);
#endif
        struct awaitable : public awaitable_base
        {
            using awaitable_base::awaitable_base;

            decltype(auto) await_resume()
            {
#if defined(DEBUG_CORO)
                std::printf("task&&::awaitable(%lu): await_resume()\n", N);
#endif
                if (!this->coroutine_) {
                    assert(false);
                    // throw broken_promise {};
                }
                printf("coro4: %p\n", this->coroutine_.address());
                return std::move(this->coroutine_.promise()).result();
            }
        };

        return awaitable{coroutine_};
    }

#if 0
    task_in_context<T COMMA_N> schedule(execution::Executor* exec) && noexcept
    {
        auto& promise = coroutine_.promise();
        promise.set_executor(std::move(exec));
#if defined(DEBUG_CORO)
        std::printf("task(%lu)::schedule: %p\n", N, &promise);
        std::printf("task(%lu)::schedule: %p %p\n", N, exec, coroutine_.address());
#endif
        auto t = task_in_context<T COMMA_N>{std::exchange(coroutine_, {})};
#if defined(DEBUG_CORO)
        std::printf("task(%lu)::schedule: %p\n", N, &t);
#endif
        return t;
    }
#endif

private:
    std::experimental::coroutine_handle<promise_type> coroutine_;

    struct awaitable_base
    {
        std::experimental::coroutine_handle<promise_type> coroutine_;

        awaitable_base(std::experimental::coroutine_handle<promise_type> coroutine) noexcept
            : coroutine_(coroutine)
        {}

        bool await_ready() const noexcept
        {
            return !coroutine_ || coroutine_.done();
        }

        std::experimental::coroutine_handle<> await_suspend(
            std::experimental::coroutine_handle<> awaiting_coroutine) noexcept
        {
#if defined(DEBUG_CORO)
            std::printf("task::awaitable(%lu): await_suspend %p\n", N, awaiting_coroutine.address());
#endif
            coroutine_.promise().set_continuation(awaiting_coroutine);
            return coroutine_;
        }
    };
};

namespace detail {

template <typename T COMMA_SIZE_T_N>
Task<T COMMA_N> TaskPromise<T COMMA_N>::get_return_object() noexcept
{
#if defined(DEBUG_CORO)
    std::printf("task_promise::get_return_object(%lu)\n", N);
#endif
    return Task<T COMMA_N>{std::experimental::coroutine_handle<TaskPromise>::from_promise(*this)};
}

TEMPLATE_SIZE_T_N
inline Task<void COMMA_N> TaskPromise<void COMMA_N>::get_return_object() noexcept
{
#if defined(DEBUG_CORO)
    std::printf("task_promise::get_return_object(%lu)\n", N);
#endif
    return Task<void COMMA_N>{std::experimental::coroutine_handle<TaskPromise>::from_promise(*this)};
}

template <typename T COMMA_SIZE_T_N>
Task<T & COMMA_N> TaskPromise<T & COMMA_N>::get_return_object() noexcept
{
#if defined(DEBUG_CORO)
    std::printf("task_promise::get_return_object(%lu)\n", N);
#endif
    return Task<T & COMMA_N>{std::experimental::coroutine_handle<TaskPromise>::from_promise(*this)};
}

} // namespace detail

} // namespace ptl::experimental::coroutine