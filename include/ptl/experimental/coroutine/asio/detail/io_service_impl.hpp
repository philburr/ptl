#pragma once
#include <mutex>
#include <atomic>

#include "ptl/experimental/coroutine/asio/detail/io_service_definitions.hpp"
#include "ptl/experimental/coroutine/asio/descriptor.hpp"
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

namespace ptl::experimental::coroutine::asio::detail {

using expected_socket = ptl::expected<descriptor::native_type, ptl::error_code>;
using expected_void = ptl::expected<void, ptl::error_code>;

struct descriptor_service_data;
enum {
    shutdown_read,
    shutdown_write,
    shutdown_read_write,
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

    void register_descriptor(descriptor fd, detail::descriptor_service_data *&data);
    void deregister_descriptor(descriptor fd, detail::descriptor_service_data *&data);

    void start_io(detail::descriptor_service_data &data, io_kind kind, io_service_operation *op);
    void stop_io(descriptor_service_data &data);

    void write(descriptor::native_type d, const uint8_t* buffer, size_t sz);
    ssize_t read(descriptor::native_type d, uint8_t* buffer, size_t sz);

private:
    struct ev_loop *event_loop_;
    std::atomic<bool> running_;

    static void ev_notification(struct ev_loop* loop, ev_io* io, int events);
};

} // namespace ptl::experimental::coroutine::asio::detail
