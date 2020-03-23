#include "catch2/catch.hpp"
#include <thread>
#include "ptl/execution/basic_executor.hpp"
#include "ptl/experimental/coroutine/task.hpp"
#include "ptl/experimental/coroutine/simple_task.hpp"
#include "ptl/experimental/coroutine/sync_wait.hpp"
#include "ptl/experimental/coroutine/scheduling/thread_pool.hpp"

using namespace ptl::experimental::coroutine;

#define TEST_MULTITHREADED

#if defined(TEST_MULTITHREADED)
void thread_executor(ptl::execution::QueuedExecutor** ex, ptl::sync::manual_reset_event* ev)
{
    ptl::execution::QueuedExecutor executor;
    *ex = &executor;
    ev->set();
    executor.run();
}
#endif

int simple_test() {
    auto f = []() -> IntTask {
        co_return 1;
    }();
    return sync_wait(f);
}

#if 1
TEST_CASE("task")
{
    bool started = false;
    auto func = [&]() -> Task<int> {
        started = true;
        co_return 1;
    };

    REQUIRE(!started);
    sync_wait([&]() -> Task<int> {
        REQUIRE(!started);

        auto f = func();
        auto i = co_await f;

        co_return i;
    }());
    REQUIRE(started);
}

TEST_CASE("task return void")
{
    bool run = false;
    auto lambda = [&]() -> Task<void> {
        run = true;
        co_return;
    };
    static_assert(std::is_void_v<typename awaitable_traits<decltype(lambda())>::await_result_t>, "");

    sync_wait(lambda());
    REQUIRE(run);
}

TEST_CASE("task return ref")
{
    int value = 0;
    auto lambda = [&]() -> Task<int&> {
        value = 1;
        co_return value;
    };
    static_assert(!std::is_void_v<typename awaitable_traits<decltype(lambda())>::await_result_t>, "");
    static_assert(std::is_lvalue_reference_v<typename awaitable_traits<decltype(lambda())>::await_result_t>, "");

    auto& r = sync_wait(lambda());
    REQUIRE(r == 1);
    REQUIRE(value == 1);
    r = 2;
    REQUIRE(value == 2);
}

TEST_CASE("task return value")
{
    auto lambda = []() -> Task<int> {
        co_return 42;
    };
    static_assert(!std::is_void_v<typename awaitable_traits<decltype(lambda())>::await_result_t>, "");
    static_assert(!std::is_lvalue_reference_v<typename awaitable_traits<decltype(lambda())>::await_result_t>, "");

    auto r = sync_wait(lambda());
    REQUIRE(r == 42);
}

TEST_CASE("task with exception")
{
    auto lambda = []() -> Task<void> {
        throw std::logic_error("bad");
    };

    REQUIRE_THROWS(sync_wait(lambda()));
}

TEST_CASE("task<int> with exception")
{
    auto lambda = []() -> Task<int> {
        throw std::logic_error("bad");
    };

    REQUIRE_THROWS(sync_wait(lambda()));
}

TEST_CASE("task<int&> with exception")
{
    auto lambda = []() -> Task<int&> {
        throw std::logic_error("bad");
    };

    REQUIRE_THROWS(sync_wait(lambda()));
}


#endif

#if defined(TEST_MULTITHREADED)
static
Task<int> task_on_thread1(ptl::execution::Executor* ex1, ptl::execution::Executor* ex2)
{
    REQUIRE(ptl::execution::Executor::current == ex1);
    co_await ptl::experimental::coroutine::detail::SchedulerAwaitable { ex1 };
    REQUIRE(ptl::execution::Executor::current == ex1);
    co_await ptl::experimental::coroutine::detail::SchedulerAwaitable { ex2 };
    REQUIRE(ptl::execution::Executor::current == ex2);
    co_await ptl::experimental::coroutine::detail::SchedulerAwaitable { ex2 };
    REQUIRE(ptl::execution::Executor::current == ex2);

    co_return 3;
}

Task<int&> thread_return_ref(ptl::execution::Executor* ex1)
{
    int x = 3;

    REQUIRE(ptl::execution::Executor::current == ex1);
    co_return x;
}

TEST_CASE("task multithread")
{
    ptl::execution::QueuedExecutor* ex1;
    ptl::execution::QueuedExecutor* ex2;
    ptl::sync::manual_reset_event ev1;
    ptl::sync::manual_reset_event ev2;

    std::thread first(thread_executor, &ex1, &ev1);
    std::thread second(thread_executor, &ex2, &ev2);
    ev1.wait(); ev2.wait();

    auto r = async_wait(task_on_thread1(ex1, ex2).schedule_on(ex1));
    assert(r == 3);

    auto rr = async_wait(thread_return_ref(ex1).schedule_on(ex1));
    assert(rr == 3);

    ex1->stop();
    ex2->stop();

    first.join();
    second.join();
}
#endif

#if 0
task<int, 2> task2(ptl::execution::Executor* ex2)
{
    printf("3\n");
    //REQUIRE(ptl::execution::Executor::current == ex2);
    co_return 3;
}

task<int, 1> task1(ptl::execution::Executor* ex1, ptl::execution::Executor* ex2)
{
    REQUIRE(ptl::execution::Executor::current == ex1);
    printf("1\n");
    int r = co_await task2(ex2).schedule(ex2);
    printf("2\n");
    //REQUIRE(ptl::execution::Executor::current == ex1);
    co_return r;
}

TEST_CASE("task multithread")
{
    ptl::execution::QueuedExecutor* ex1;
    ptl::execution::QueuedExecutor* ex2;
    ptl::sync::manual_reset_event ev1;
    ptl::sync::manual_reset_event ev2;

    std::thread first(thread1, &ex1, &ev1);
    std::thread second(thread1, &ex2, &ev2);
    ev1.wait(); ev2.wait();
    printf("ex1: %p\nex2: %p\n", ex1, ex2);

    auto r = sync_wait(task1(ex1, ex2).schedule(ex1));
    REQUIRE(r == 3);

    ex1->stop();
    ex2->stop();

    first.join();
    second.join();
}

#endif