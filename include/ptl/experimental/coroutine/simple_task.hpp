#pragma once

#if !defined(__clang__)
#error Unsupported compiler
#endif
#include <variant>
#include <experimental/coroutine>
#include "ptl/scope_guard.hpp"
#include "ptl/exceptable.hpp"

namespace ptl::experimental::coroutine {

struct IntTask
{
    struct promise_type
    {
        void return_value(int v)
        {
            //result_.emplace<1>(v);
            result_ = v;
        }
        void unhandled_exception()
        {
            //result_.emplace<2>(std::current_exception());
        }
        IntTask get_return_object()
        {
            return IntTask{std::experimental::coroutine_handle<promise_type>::from_promise(*this)};
        }

        std::experimental::suspend_always initial_suspend()
        {
            return {};
        }
        auto final_suspend()
        {
            struct continuation_awaiter
            {
                promise_type* self;

                bool await_ready()
                {
                    return false;
                }

                /*std::experimental::coroutine_handle<>*/void await_suspend(
                    std::experimental::coroutine_handle<> coroutine) noexcept
                {
                    //return self->continuation_;
                    self->continuation_.resume();
                }
                void await_resume()
                {}
            };
            return continuation_awaiter{this};
        }

    private:
        friend struct IntTask;
        int result_;
        std::experimental::coroutine_handle<> continuation_;
    };

    IntTask(std::experimental::coroutine_handle<promise_type> coro)
        : coroutine_(coro)
    {}

    ~IntTask()
    {
        if (coroutine_) {
            coroutine_.destroy();
        }
    }

    bool await_ready()
    {
        return false;
    }

    /*std::experimental::coroutine_handle<>*/ void await_suspend(
        std::experimental::coroutine_handle<> awaiting_coroutine) noexcept
    {
        coroutine_.promise().continuation_ = awaiting_coroutine;
        //return coroutine_;
        coroutine_.resume();
    }

    int await_resume()
    {
        auto& promise = coroutine_.promise();
        //if (promise.result_.index() == 2) {
        //    std::rethrow_exception(std::get<2>(promise.result_));
        //}
        //return std::get<1>(promise.result_);
        return promise.result_;
    }

private:
    std::experimental::coroutine_handle<promise_type> coroutine_;
};

} // namespace ptl::experimental::coroutine