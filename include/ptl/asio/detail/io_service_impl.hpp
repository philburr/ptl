#pragma once
#include <mutex>
#include <atomic>

#include "ptl/asio/detail/io_service_definitions.hpp"
#include "ptl/asio/descriptor.hpp"

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

namespace ptl::asio::detail {

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

    int bind(descriptor::native_type socket, const void *address, size_t address_len);
    int connect(descriptor::native_type socket, const void *address, size_t address_len);
    int listen(descriptor::native_type socket, int backlog);
    int accept(descriptor::native_type socket, void *address, size_t* address_len);
    int shutdown(descriptor::native_type socket, int how);
    int close(descriptor::native_type socket);

    ssize_t send(descriptor::native_type socket, const uint8_t* buffer, size_t sz, int flags);
    ssize_t recv(descriptor::native_type socket, uint8_t* buffer, size_t sz, int flags);

    void register_descriptor(descriptor fd, detail::descriptor_service_data *&data);
    void deregister_descriptor(descriptor fd, detail::descriptor_service_data *&data);

    void start_io(detail::descriptor_service_data &data, io_kind kind, io_service_operation *op);
    void stop_io(descriptor_service_data &data);

private:
    struct ev_loop *event_loop_;
    std::atomic<bool> running_;

    static void ev_notification(struct ev_loop* loop, ev_io* io, int events);
};

} // namespace ptl::asio::detail
