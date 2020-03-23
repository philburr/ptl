#include "ptl/experimental/asio/io_service.hpp"
#include "ptl/experimental/asio/descriptor.hpp"
#include <cassert>
#include <system_error>
#include <mutex>
#include <ev.h>

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

namespace ptl::experimental::asio::detail {

std::mutex libev_guard_;

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

io_service_impl::io_service_impl()
{
    static std::atomic<int> count__ = 0;

    struct ev_loop* loop;
    if (count__++ == 0) {
        loop = ev_default_loop();
    }
    else {
        loop = ev_loop_new(0);
    }
    if (loop == nullptr) {
        throw std::bad_alloc();
    }

    running_ = false;
    event_loop_ = loop;
}

io_service_impl::~io_service_impl()
{
    ev_loop_destroy(event_loop_);
}

void io_service_impl::stop() noexcept
{
    running_ = false;
    ev_break(event_loop_, EVBREAK_ALL);
}

void io_service_impl::run()
{
    running_ = true;
    while (running_) {
        ev_run(event_loop_, EVRUN_ONCE);
    }
}

void io_service_impl::register_descriptor(descriptor fd, detail::descriptor_service_data*& data)
{
    data = new descriptor_service_data(fd);
    data->registered_events_ = EV_READ;
    ev_io_init(&data->ev_, ev_notification, fd.native_descriptor(), EV_READ);
}

void io_service_impl::deregister_descriptor(descriptor fd, detail::descriptor_service_data*& data)
{
    ev_io_stop(event_loop_, &data->ev_);
    delete data;
    data = nullptr;
}

void io_service_impl::start_io(detail::descriptor_service_data& data, io_kind kind, io_service_operation* op)
{
    data.current_io_ = kind;
    data.current_op_ = op;

    if (kind == io_kind::write && (data.registered_events_ & EV_WRITE) == 0) {
        ev_io_set(&data.ev_, data.descriptor_.native_descriptor(), EV_READ | EV_WRITE);
        data.registered_events_ |= EV_WRITE;
    }
    ev_io_start(event_loop_, &data.ev_);
}

void io_service_impl::stop_io(descriptor_service_data& data)
{
    ev_io_stop(event_loop_, &data.ev_);
    data.current_io_ = io_kind::none;
    data.current_op_ = nullptr;
}

void io_service_impl::ev_notification(struct ev_loop* loop, ev_io* io, int events)
{
    descriptor_service_data& data = *container_of(io, &descriptor_service_data::ev_);
    if ((events & (int)data.current_io_) == (int)data.current_io_) {
        data.current_op_->work();
    }
}

std::pair<descriptor::native_type, descriptor::native_type> io_service_impl::create_pair()
{
    int fds[2];
    int r = ::socketpair(AF_UNIX, SOCK_STREAM | SOCK_NONBLOCK, 0, fds);
    if (r < 0) {
        throw std::system_error(r, std::system_category());
    }
    return {fds[0], fds[1]};
}

descriptor::native_type io_service_impl::create_socket(int domain, int type, int protocol)
{
    int r = ::socket(domain, type | SOCK_NONBLOCK, protocol);
    if (r < 0) {
        throw std::system_error(r, std::system_category());
    }
    return r;
}

expected_void io_service_impl::bind(descriptor::native_type socket, const void *address, size_t address_len)
{
    if (0 > ::bind(socket, (const sockaddr*)address, address_len)) {
        return { ptl::error_code{ errno } };
    }
    return {};
}

expected_void io_service_impl::connect(descriptor::native_type socket, const void *address, size_t address_len)
{
    if (0 > ::connect(socket, (const sockaddr*)address, address_len)) {
        return { ptl::error_code{ errno } };
    }
    return {};
}

expected_void io_service_impl::listen(descriptor::native_type socket, int backlog)
{
    if (0 > ::listen(socket, backlog)) {
        return { ptl::error_code{ errno } };
    }
    return {};
}

expected_socket io_service_impl::accept(descriptor::native_type socket, void* address, size_t* address_len)
{
    unsigned int len = *address_len;
    int r = ::accept(socket, (sockaddr*)address, &len);
    if (0 > r) {
        return { ptl::error_code{ errno } };
    }
    *address_len = len;
    return { r };
}

ssize_t io_service_impl::send(descriptor::native_type fd, const uint8_t* buffer, size_t sz, int flags)
{
    return ::send(fd, buffer, sz, flags);
}

ssize_t io_service_impl::recv(descriptor::native_type fd, uint8_t* buffer, size_t sz, int flags)
{
    return ::recv(fd, buffer, sz, flags);
}

expected_void io_service_impl::shutdown(descriptor::native_type fd, int how)
{
    if (0 > ::shutdown(fd, how)) {
        return { ptl::error_code{ errno } };
    }
    return {};
}

expected_void io_service_impl::close(descriptor::native_type fd)
{
    if (0 > ::close(fd)) {
        return { ptl::error_code{ errno } };
    }
    return {};
}

expected_void io_service_impl::getsockname(descriptor::native_type socket, void* address, size_t* address_len)
{
    unsigned int len = *address_len;
    int r = ::getsockname(socket, (sockaddr*)address, &len);
    if (0 > r) {
        return { ptl::error_code{ errno } };
    }
    *address_len = len;
    return {};
}

expected_void io_service_impl::getpeername(descriptor::native_type socket, void* address, size_t* address_len)
{
    unsigned int len = *address_len;
    int r = ::getpeername(socket, (sockaddr*)address, &len);
    if (0 > r) {
        return { ptl::error_code{ errno } };
    }
    *address_len = len;
    return {};
}

} // namespace ptl::experimental::asio::detail

#if 0
io_service::io_service() {

}

io_service::~io_service() {

}

void io_service_internal::stop() noexcept {
    running_ = false;
    interrupt();
}

io_service_private::io_service_private() 
    : running_(true)
{
    epoll_fd_ = ::epoll_create1(0);
    signal_ = ::eventfd(0, EFD_NONBLOCK);

    epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLERR | EPOLLET | EPOLLONESHOT;
    ev.data.ptr = &signal_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, signal_, &ev);

    auto counter = uint64_t(1);
    int result = ::write(signal_, &counter, sizeof(counter));
    (void)result;
}

io_service_private::~io_service_private() {
    ::close(epoll_fd_);
    ::close(signal_);
}

void io_service_private::interrupt() {
    epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLERR | EPOLLET | EPOLLONESHOT;
    ev.data.ptr = &signal_;
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, signal_, &ev);
}

void io_service_internal::register_descriptor(descriptor fd, descriptor_service_data*& data) {
    data = new descriptor_service_data(fd);

    epoll_event ev = {0};
    ev.events = EPOLLIN | EPOLLERR | EPOLLHUP | EPOLLPRI | EPOLLET;
    ev.data.ptr = data;

    data->registered_events = ev.events;

    int result = epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd.native_descriptor(), &ev);
    if (result != 0) {
         throw std::system_error(errno, std::system_category());
    }
}

void io_service_internal::deregister_descriptor(descriptor fd, descriptor_service_data*& data) {
    if (data->registered_events != 0) {
        epoll_event ev = {0};
        epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, fd.native_descriptor(), &ev);
    }
    delete data;
    data = nullptr;
}

void io_service_internal::start_io(descriptor fd, descriptor_service_data& data, io_kind kind, io_service_operation* op) {
    data.op[(int)kind] = op;

    if (kind == io_kind::write && (data.registered_events & EPOLLOUT) == 0) {
        epoll_event ev = {0};
        ev.events = data.registered_events | EPOLLOUT;
        ev.data.ptr = &data;
        if (epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, fd.native_descriptor(), &ev) == 0) {
            data.registered_events |= EPOLLOUT;
        } else {
            assert(false && "promote error");
            return;
        }
    }
}

size_t io_service_private::process_events() {
    if (is_stopping()) {
        return 0;
    }

    epoll_event events[128];
    int event_count = epoll_wait(epoll_fd_, events, 128, 0);
    for (int i = 0; i < event_count; i++) {
        void* ptr = events[i].data.ptr;
        if (ptr == &signal_) {
            // the purpose of this is just to wake us up via interrrupt()
            continue;
        }

        descriptor_service_data* data = static_cast<descriptor_service_data*>(ptr);


        io_kind kind;
        if (events[i].events & (EPOLLERR | EPOLLHUP)) {
            kind = io_kind::error;
        } else if ((events[i].events & EPOLLOUT) == EPOLLOUT) {
            kind = io_kind::write;

            epoll_event ev = {0};
            ev.events = data->registered_events & ~EPOLLOUT;
            ev.data.ptr = data;
            epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, data->descriptor_.native_descriptor(), &ev);
        } else if ((events[i].events & EPOLLIN) == EPOLLIN) {
            kind = io_kind::read;
        }
        if (data->op[(int)kind] != nullptr) {
            data->op[(int)kind]->work();
            data->op[(int)kind] = nullptr;
        }
    }

    return event_count;
}

bool io_service_private::is_stopping() const {
    return !running_;
}

void io_service::run() {
    while (!is_stopping()) {
        process_events();
    }
}
#endif