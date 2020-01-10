#include "catch2/catch.hpp"
#include "ptl/scope_guard.hpp"

#include <atomic>
std::atomic<int> a_;
std::atomic<int> b_;

void PTL_NOINLINE lock()
{
    a_.store(1);
}
void PTL_NOINLINE unlock()
{
    a_.store(0);
}
void PTL_NOINLINE lock2()
{
    b_.store(1);
}
void PTL_NOINLINE unlock2()
{
    b_.store(0);
}
int a = 0;

int test(int *var)
{
    lock();
    SCOPE_EXIT({ unlock(); });
    *var += 4;
    lock2();
    SCOPE_EXIT({ unlock2(); });
    *var += 5;
    return *var;
}

int scope_return()
{
    int a = 3;
    SCOPE_EXIT({ a = 4; });

    // In a return statement, destructors run *after* return expression evaluation, before the actual return
    // return 3;
    return a;
}

int scope_return2()
{
    int a = 3;
    {
        SCOPE_EXIT({ a = 4; });
    }
    // return 4;
    return a;
}

TEST_CASE("Scope Guard Basic")
{
    REQUIRE(scope_return() == 3);
    REQUIRE(scope_return2() == 4);
}

TEST_CASE("Scope Guard Order")
{
    std::vector<int> results;
    {
        SCOPE_EXIT({ results.push_back(1); });
        SCOPE_EXIT({ results.push_back(2); });
    }
    REQUIRE(results.size() == 2);
    REQUIRE(results[0] == 2);
    REQUIRE(results[1] == 1);
}