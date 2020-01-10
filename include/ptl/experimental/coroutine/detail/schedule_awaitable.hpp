#pragma once

#if !defined(__clang__)
#error Unsupported compiler
#endif
#include <experimental/coroutine>
#include "ptl/scope_guard.hpp"
#include "ptl/execution/basic_executor.hpp"

namespace ptl::experimental::coroutine::detail {

struct SchedulerAwaitable
{
    SchedulerAwaitable(ptl::execution::Executor* ex)
        : executor_(ex)
    {}

    bool await_ready() const noexcept
    {
        if (ptl::execution::Executor::current == executor_) {
            return true;
        }
        return false;
    }

    void await_suspend(std::experimental::coroutine_handle<> coroutine) noexcept
    {
        //auto& promise
        executor_->add([coroutine = coroutine]() mutable { coroutine.resume(); });
    }

    void await_resume()
    {}

private:
    execution::Executor* executor_;
};


} // namespace ptl::experimental::coroutine::detail