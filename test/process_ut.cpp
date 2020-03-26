#include "ptl/experimental/coroutine/process/process.hpp"

#include "ptl/experimental/coroutine/task.hpp"
#include "ptl/experimental/coroutine/sync_wait.hpp"
#include "ptl/experimental/coroutine/wait_all.hpp"
#include <catch2/catch.hpp>

using namespace ptl::experimental::coroutine;
using namespace ptl::experimental::coroutine::iosvc;

TEST_CASE("launch 'false'")
{
    io_service svc;
    process::subprocess p(svc);
    int rc;

    std::vector<uint8_t> buffer(256);


    auto lambda = [&]() -> Task<void> {
        p.launch("/usr/bin/g++", {"-c", "foo.c", "-o", "foo"});

        while (true) {
            auto r = co_await p.read(buffer.data(), buffer.size());
            if (r.is_error()) {
                break;
            }

            size_t sz = r.value();
            if (sz == 0) {
                break;
            }
        }

        rc = (co_await p.wait()).value();
        co_return;
    };
    static_assert(std::is_void_v<typename awaitable_traits<decltype(lambda())>::await_result_t>, "");

    sync_wait(wait_all(
        [&]() -> Task<> {
            SCOPE_EXIT({ svc.stop(); });

            co_await wait_all(
                std::move(lambda())
            );
            co_return;
        }(),
        [&svc]() -> Task<> {
            svc.run();
            co_return;
        }()
    ));
    REQUIRE(rc != 0);
}
