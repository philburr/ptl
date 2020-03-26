#pragma once
//#include <mutex>
#include <list>
#include <atomic>
#include <ev.h>

#include "ptl/experimental/coroutine/io_service/detail/io_service_definitions.hpp"
#include "ptl/experimental/coroutine/io_service/descriptor.hpp"
#include "ptl/expected.hpp"

// FIXME: find a home:
template< class T, class M >
static inline constexpr std::ptrdiff_t offset_of( const M T::*member ) {
    static_assert(std::is_standard_layout_v<T>);
    return reinterpret_cast< std::ptrdiff_t >( &( reinterpret_cast< T* >( 0 )->*member ) );
}

template< class T, class M >
static inline constexpr T* container_of( M *ptr, const M T::*member ) {
    static_assert(std::is_standard_layout_v<T>);
    return reinterpret_cast< T* >( reinterpret_cast< intptr_t >( ptr ) - offset_of( member ) );
}

typedef struct ev_io ev_io;
struct ev_loop;
struct ev_child;

namespace ptl::experimental::coroutine::iosvc::detail {

using expected_socket = ptl::expected<descriptor::native_type, ptl::error_code>;
using expected_void = ptl::expected<void, ptl::error_code>;
using expected_size_t = ptl::expected<size_t, ptl::error_code>;

struct descriptor_service_data
{
    descriptor_service_data(descriptor d)
        : descriptor_(d)
        , registered_events_(0)
        , current_io_(io_kind::none)
        , current_op_(nullptr)
    {}

    descriptor descriptor_;
    ev_io ev_;
    int registered_events_;
    io_kind current_io_;
    io_service_operation* current_op_;
};
static_assert(std::is_standard_layout_v<descriptor_service_data>);

enum {
    shutdown_read,
    shutdown_write,
    shutdown_read_write,
};

struct process_service_data {
    int pid;
    int rc;
    io_service_operation* notification;
};

class io_service_impl
{
public:
    io_service_impl();
    ~io_service_impl();

    void stop() noexcept;
    void run();

    std::pair<descriptor::native_type, descriptor::native_type> create_pair(); 

    descriptor::native_type create_socket(int domain, int type, int protocol);

    expected_void bind(descriptor::native_type socket, const void *address, size_t address_len);
    expected_void connect(descriptor::native_type socket, const void *address, size_t address_len);
    expected_void listen(descriptor::native_type socket, int backlog);
    expected_socket accept(descriptor::native_type socket, void *address, size_t* address_len);
    expected_void shutdown(descriptor::native_type socket, int how);
    expected_void close(descriptor::native_type socket);
    expected_void getsockname(descriptor::native_type socket, void* address, size_t* address_len);
    expected_void getpeername(descriptor::native_type socket, void* address, size_t* address_len);

    ssize_t send(descriptor::native_type socket, const uint8_t* buffer, size_t sz, int flags);
    ssize_t recv(descriptor::native_type socket, uint8_t* buffer, size_t sz, int flags);

    expected_size_t write(descriptor::native_type socket, const uint8_t* buffer, size_t sz);
    expected_size_t read(descriptor::native_type socket, uint8_t* buffer, size_t sz);

    std::unique_ptr<detail::descriptor_service_data> register_descriptor(descriptor fd);
    void deregister_descriptor(descriptor fd, std::unique_ptr<detail::descriptor_service_data> data);

    void register_process_notification(int pid, detail::process_service_data& data);
    void deregister_process_notification(detail::process_service_data& data);

    void start_io(detail::descriptor_service_data &data, io_kind kind, io_service_operation *op);
    void stop_io(descriptor_service_data &data);
    void start_notification(detail::process_service_data &data, io_service_operation *op);
    void stop_notification(detail::process_service_data &data);

private:
    struct ev_loop *event_loop_;
    std::unique_ptr<ev_child> process_monitor_;
    std::atomic<bool> running_;
    std::list<std::reference_wrapper<process_service_data>> process_watchers_;

    static void ev_notification(struct ev_loop* loop, ev_io* io, int events);
    static void ev_process_change(struct ev_loop* loop, struct ev_child* child, int events);
};

} // namespace ptl::experimental::coroutine::asio::detail
