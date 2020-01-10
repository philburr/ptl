#pragma once
#include "wait_all_counter.hpp"

namespace ptl::experimental::coroutine::detail {

struct void_value {};

template<typename RESULT>
class wait_all_task;

template<typename DERIVED>
class wait_all_promise_base
{
public:
    using coroutine_handle = std::experimental::coroutine_handle<DERIVED>;

    wait_all_promise_base() noexcept {}

    std::experimental::suspend_always initial_suspend() noexcept
    {
        return {};
    }

    auto final_suspend() noexcept
    {
        struct notifier_awaitable {
            bool await_ready() const noexcept { return false; }
            void await_suspend(coroutine_handle coroutine) const noexcept
            {
                coroutine.promise().counter_->notify_complete();
            }
            void await_resume() const noexcept {}

        };

        return notifier_awaitable{};
    }

    void unhandled_exception() noexcept
    {
        exception_ = std::current_exception();
    }

    void rethrow_exception()
    {
        if (exception_) {
            std::rethrow_exception(exception_);
        }
    }

protected:
    wait_all_counter* counter_;
    std::exception_ptr exception_;
};

template<typename RESULT>
class wait_all_promise final : public wait_all_promise_base<wait_all_promise<RESULT>>
{
public:
    using coroutine_handle = std::experimental::coroutine_handle<wait_all_promise<RESULT>>;

    wait_all_promise() noexcept
    {}

    auto get_return_object() noexcept
    {
        return coroutine_handle::from_promise(*this);
    }

    void return_void() noexcept
    {
        assert(false);
    }

    auto yield_value(RESULT&& result) noexcept
    {
        result_ = std::addressof(result);
        return this->final_suspend();
    }

    void start(wait_all_counter& counter) noexcept
    {
        this->counter_ = &counter;
        coroutine_handle::from_promise(*this).resume();
    }

    RESULT& result() &
    {
        this->rethrow_exception();
        return *result_;
    }

    RESULT&& result() &&
    {
        this->rethrow_exception();
        return std::forward<RESULT>(*result_);
    }

private:
    std::add_pointer_t<RESULT> result_;
};

template<>
class wait_all_promise<void> final : public wait_all_promise_base<wait_all_promise<void>>
{
public:
    wait_all_promise() noexcept
    {}

    auto get_return_object() noexcept
    {
        return coroutine_handle::from_promise(*this);
    }

    void return_void() noexcept
    {
    }

    void start(wait_all_counter& counter) noexcept
    {
        this->counter_ = &counter;
        coroutine_handle::from_promise(*this).resume();
    }

    void result()
    {
        this->rethrow_exception();
    }

};

template<typename RESULT>
class wait_all_task final
{
public:
    using promise_type = wait_all_promise<RESULT>;
    using coroutine_handle = typename promise_type::coroutine_handle;

    wait_all_task(coroutine_handle coroutine) noexcept
        : coroutine_(coroutine)
    {}
    wait_all_task(wait_all_task&& other) noexcept
        : coroutine_(std::exchange(other.coroutine_, coroutine_handle{}))
    {}

    ~wait_all_task() {
        if (coroutine_) {
            coroutine_.destroy();
        }
    }
    wait_all_task(const wait_all_task&) = delete;
    wait_all_task& operator=(const wait_all_task&) = delete;

    decltype(auto) result() &
    {
        return coroutine_.promise().result();
    }
    decltype(auto) result() &&
    {
        return std::move(coroutine_.promise()).result();
    }
    decltype(auto) non_void_result() &
    {
        if constexpr(std::is_void_v<decltype(this->resuult())>)
        {
            this->result();
            return void_value{};
        }
        else
        {
            return this->result();
        }
    }
    decltype(auto) non_void_result() &&
    {
        if constexpr(std::is_void_v<decltype(this->resuult())>)
        {
            std::move(*this).result();
            return void_value{};
        }
        else
        {
            return std::move(*this).result();
        }
    }

private:
    template<typename T>
    friend class wait_all_awaitable;

    void start(wait_all_counter& counter) noexcept
    {
        coroutine_.promise().start(counter);
    }

    coroutine_handle coroutine_;
};

template<typename AWAITABLE,
         typename RESULT = typename coroutine::awaitable_traits<AWAITABLE&&>::await_result_t,
         std::enable_if_t<!std::is_void_v<RESULT>, int> = 0>
wait_all_task<RESULT> make_wait_all_task(AWAITABLE awaitable)
{
    co_yield co_await static_cast<AWAITABLE&&>(awaitable);
}

template<typename AWAITABLE,
         typename RESULT = typename coroutine::awaitable_traits<AWAITABLE&&>::await_result_t,
         std::enable_if_t<std::is_void_v<RESULT>, int> = 0>
wait_all_task<RESULT> make_wait_all_task(AWAITABLE awaitable)
{
    co_await static_cast<AWAITABLE&&>(awaitable);
}


}