#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "catch2/catch.hpp"
#include "ptl/scope_guard.hpp"
#include "ptl/experimental/coroutine/asio/socket.hpp"
#include "ptl/experimental/coroutine/task.hpp"
#include "ptl/experimental/coroutine/sync_wait.hpp"
#include "ptl/experimental/coroutine/wait_all.hpp"
#include "ptl/experimental/coroutine/async_scope.hpp"
#include "ptl/experimental/coroutine/asio/ip_endpoint.hpp"

#include "ptl/mpmc_queue.hpp"

using ptl::experimental::coroutine::asio::socket;
using ptl::experimental::coroutine::sync_wait;
using ptl::experimental::coroutine::Task;
using ptl::experimental::coroutine::wait_all;
using ptl::experimental::coroutine::async_scope;

TEST_CASE("socket pair")
{
    ptl::experimental::coroutine::asio::io_service srv;

    auto [read_socket, write_socket] = socket::create_pair(srv);

    sync_wait(wait_all(
        [&, read_socket{std::move(read_socket)}]() mutable -> Task<> {
            char output[3];

            auto res = co_await read_socket.recv(output, 2);
            output[2] = 0;

            REQUIRE(res.is_value());
            REQUIRE(res.value() == 2);
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
    using namespace ptl::experimental::coroutine::asio;
    io_service svc;

    auto server = socket::create_tcpv4(svc);
    auto ep = ipv4_endpoint{ ipv4_address::loopback(), 0 };
    server.bind(ep);
    server.listen();
    ep = server.local_address().to_ipv4();

    bool running = true;
    auto server_connection = [](struct socket s) -> Task<>
    {
        const char* hello = "hello, world";
        co_await s.send(hello, strlen(hello));
        co_await s.shutdown();
    };

    auto client_connection = [](struct socket& s) -> Task<>
    {
        std::array<uint8_t, 64> buffer;

        auto received = co_await s.recv(buffer.data(), buffer.size());
        REQUIRE(received.is_value());
        REQUIRE(std::string((char*)buffer.data(), received.value()) == "hello, world");

        co_await s.shutdown();
    };

    auto server_handler = [&running, &svc, server_connection = std::move(server_connection)](struct socket s) -> Task<>
    {
        async_scope scope;

        {
            auto c = co_await s.accept();
            REQUIRE(c.is_value());
            scope.spawn(server_connection(std::move(c.value())));
        }
        co_await scope.join();
        co_return;
    };

    auto client_handler = [&running, &svc, &ep, client_connection = std::move(client_connection)]() -> Task<>
    {
        auto socket = socket::create_tcpv4(svc);
        co_await socket.connect(ep);
        co_await client_connection(socket);
    };

    sync_wait(wait_all(
        [&]() -> Task<> {
            SCOPE_EXIT({ svc.stop(); });

            co_await wait_all(
                std::move(server_handler(std::move(server))),
                std::move(client_handler())
            );
            co_return;
        }(),
        [&svc]() -> Task<> {
            svc.run();
            co_return;
        }()
    ));
}