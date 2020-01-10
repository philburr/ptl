#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "catch2/catch.hpp"
#include "ptl/scope_guard.hpp"
#include "ptl/experimental/asio/socket.hpp"
#include "ptl/experimental/coroutine/task.hpp"
#include "ptl/experimental/coroutine/sync_wait.hpp"
#include "ptl/experimental/coroutine/wait_all.hpp"
#include "ptl/experimental/coroutine/async_scope.hpp"

#include "ptl/asio/ip_endpoint.hpp"

using ptl::experimental::asio::socket;
using ptl::experimental::coroutine::sync_wait;
using ptl::experimental::coroutine::Task;
using ptl::experimental::coroutine::wait_all;
using ptl::experimental::coroutine::async_scope;

TEST_CASE("socket pair")
{
    ptl::asio::io_service srv;

    auto [read_socket, write_socket] = socket::create_pair(srv);

    sync_wait(wait_all(
        [&, read_socket{std::move(read_socket)}]() mutable -> Task<> {
            char output[3];

            auto res = co_await read_socket.recv(output, 2);
            output[2] = 0;

            REQUIRE(res == 2);
            REQUIRE(strcmp(output, "hi") == 0);
            srv.stop();
            co_return;
        }(),
        [&, write_socket{std::move(write_socket)}]() mutable -> Task<> {
            char input[] = "hi";

            co_await write_socket.send(input, 2);
        }(),
        [&]() -> Task<> {
            srv.run();
            co_return;
        }()));
}

TEST_CASE("socket connection")
{
    using namespace ptl::asio;
    io_service srv;

    auto server = socket::create_tcpv4(srv);
    server.bind(ipv4_endpoint{ ipv4_address::loopback(), 9099 });
    server.listen();

    bool running = true;
    auto server_connection = [](struct socket s) -> Task<>
    {
        const char* hello = "hello, world";
        co_await s.send(hello, strlen(hello));
        co_await s.shutdown();
    };

    auto server_handler = [&running, &srv, server_connection](struct socket s) -> Task<>
    {
        async_scope scope;

        while (running) {
            auto c = co_await s.accept();
            scope.spawn(server_connection(std::move(c)));
        }
        co_await scope.join();
        co_return;
    };

    sync_wait(wait_all(
        [&srv]() -> Task<> {
            co_return;
        }()

    ));
}