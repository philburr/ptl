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

template <typename T>
class Task;

namespace detail {

class TaskPromiseBase
{
public:
    auto initial_suspend() noexcept
    {
        return std::experimental::suspend_always{};
    }

    auto final_suspend() noexcept
    {
        return final_awaitable{};
    }

    void set_continuation(std::experimental::coroutine_handle<> continuation) noexcept
    {
        continuation_ = continuation;
        executor_ = execution::Executor::current;
    }

    void* operator new(std::size_t sz) {
        printf("promise new: %lu\n", sz);
        return ::operator new(sz);
    }
    void operator delete(void* ptr, std::size_t sz) {
        return ::operator delete(ptr, sz);
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
            if (promise.executor_ == nullptr || promise.executor_ == execution::Executor::current) {
                return promise.continuation_;
            }
            promise.executor_->add([coroutine = promise.continuation_]() mutable { coroutine.resume(); });
            return std::experimental::noop_coroutine();
        }

        void await_resume() noexcept
        {}
    };
};

template <typename T>
class TaskPromise final : public TaskPromiseBase
{
public:
    TaskPromise() noexcept
    {}
    ~TaskPromise()
    {}

    void unhandled_exception() noexcept
    {
        value_ = ptl::exceptable<T>(std::current_exception());
    }

    template <typename VALUE_TYPE, typename = std::enable_if_t<std::is_convertible_v<VALUE_TYPE&&, T>>>
    void return_value(VALUE_TYPE&& value) noexcept(std::is_nothrow_constructible_v<T, VALUE_TYPE&&>)
    {
        value_ = ptl::exceptable<T>(value);
    }

    void return_value(T& value)
    {
        value_ = ptl::exceptable<T>(value);
    }

    Task<T> get_return_object() noexcept;

    T& result() &
    {
        return value_.value();
    }

    T&& result() &&
    {
        if constexpr (std::is_lvalue_reference_v<T>) {
            return value_.value();
        } else {
            return std::move(value_.value());
        }
    }

private:
    ptl::exceptable<T> value_;
};

// void specialization
template<>
class TaskPromise<void> final : public TaskPromiseBase
{
public:
    TaskPromise() noexcept
    {}

    Task<void> get_return_object() noexcept;

    void return_void() noexcept
    {
    }
    void unhandled_exception() noexcept
    {
        value_ = ptl::exceptable<void>(std::current_exception());
    }

    void result()
    {
        value_.value();
    }

private:
    ptl::exceptable<void> value_;
};

#if defined(TASK_PROMISE_REF)
template <typename T>
class TaskPromise<T&> final : public TaskPromiseBase
{
public:
    TaskPromise() noexcept = default;

    Task<T&> get_return_object() noexcept;

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
#endif

} // namespace detail

template <typename T = void>
class Task
{
public:
    using promise_type = detail::TaskPromise<T>;
    using value_type = T;

    Task() noexcept
        : coroutine_(nullptr)
    {
    }

    explicit Task(std::experimental::coroutine_handle<promise_type> coroutine) noexcept
        : coroutine_(coroutine)
    {
        //printf("task: %p %lu\n", this, sizeof(*this));
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
        struct awaitable : public awaitable_base
        {
            using awaitable_base::awaitable_base;

            decltype(auto) await_resume()
            {
                return this->coroutine_.promise().result();
            }
        };

        return awaitable{coroutine_};
    }

    auto operator co_await() const&& noexcept
    {
        struct awaitable : public awaitable_base
        {
            using awaitable_base::awaitable_base;

            decltype(auto) await_resume()
            {
                return std::move(this->coroutine_.promise()).result();
            }
        };

        return awaitable{coroutine_};
    }

    auto schedule_on(ptl::execution::Executor* ex) const&& noexcept
    {
        struct awaitable
        {
            std::experimental::coroutine_handle<promise_type> coroutine_;

            awaitable(std::experimental::coroutine_handle<promise_type> coroutine, ptl::execution::Executor* ex) noexcept
                : coroutine_(coroutine)
                , executor_(ex)
            {}

            bool await_ready() const noexcept
            {
                return false;
            }

            void await_suspend(std::experimental::coroutine_handle<> awaiting_coroutine) noexcept
            {
                //auto& promise
                coroutine_.promise().set_continuation(awaiting_coroutine);
                executor_->add([coroutine = coroutine_]() mutable { coroutine.resume(); });
            }

            decltype(auto) await_resume()
            {
                return std::move(this->coroutine_.promise()).result();
            }

        private:
            execution::Executor* executor_;
        };

        return awaitable{ coroutine_, ex };
    }

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
            coroutine_.promise().set_continuation(awaiting_coroutine);
            return coroutine_;
        }
    };
};

namespace detail {

template <typename T>
Task<T> TaskPromise<T>::get_return_object() noexcept
{
    return Task<T>{std::experimental::coroutine_handle<TaskPromise>::from_promise(*this)};
}

inline Task<void> TaskPromise<void>::get_return_object() noexcept
{
    return Task<void>{std::experimental::coroutine_handle<TaskPromise>::from_promise(*this)};
}

#if defined(TASK_PROMISE_REF)
template <typename T>
Task<T&> TaskPromise<T&>::get_return_object() noexcept
{
    return Task<T&>{std::experimental::coroutine_handle<TaskPromise>::from_promise(*this)};
}
#endif

} // namespace detail

} // namespace ptl::experimental::coroutine