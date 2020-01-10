#if !defined(__clang__)
#error Unsupported compiler
#endif
#include <experimental/coroutine>
#include <atomic>

#include "ptl/scope_guard.hpp"

namespace ptl::experimental::coroutine {

class async_scope
{
public:
    async_scope() noexcept
        : count_(1u)
    {}

    ~async_scope()
    {
        assert(continuation_);
    }

    template <typename AWAITABLE>
    void spawn(AWAITABLE&& awaitable)
    {
        [](async_scope* scope, std::decay_t<AWAITABLE> awaitable) -> oneway_task {
            scope->on_work_started();
            SCOPE_EXIT({ scope->on_work_finished(); });

            co_await std::move(awaitable);
        }(this, std::forward<AWAITABLE>(awaitable));
    }

    [[nodiscard]] auto join() noexcept
    {
        class awaiter
        {
            async_scope* scope_;

        public:
            awaiter(async_scope* scope) noexcept
                : scope_(scope)
            {}

            bool await_ready() noexcept
            {
                return scope_->count_.load(std::memory_order_acquire) == 0;
            }

            bool await_suspend(std::experimental::coroutine_handle<> continuation) noexcept
            {
                scope_->continuation_ = continuation;
                return scope_->count_.fetch_sub(1u, std::memory_order_acq_rel) > 1u;
            }

            void await_resume() noexcept
            {}
        };

        return awaiter{this};
    }

private:
    std::atomic<size_t> count_;
    std::experimental::coroutine_handle<> continuation_;

    struct oneway_task
    {
        struct promise_type
        {
            std::experimental::suspend_never initial_suspend()
            {
                return {};
            }
            std::experimental::suspend_never final_suspend()
            {
                return {};
            }
            void unhandled_exception()
            {
                std::terminate();
            }
            oneway_task get_return_object()
            {
                return {};
            }
            void return_void()
            {}
        };
    };

    void on_work_finished() noexcept
    {
        if (count_.fetch_sub(1u, std::memory_order_acq_rel) == 1) {
            continuation_.resume();
        }
    }

    void on_work_started() noexcept
    {
        assert(count_.load(std::memory_order_relaxed) != 0);
        count_.fetch_add(1, std::memory_order_relaxed);
    }
};

} // namespace ptl::experimental::coroutine