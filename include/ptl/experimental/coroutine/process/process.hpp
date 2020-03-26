#pragma once
#include "ptl/experimental/coroutine/io_service/io_service.hpp"
#include "ptl/experimental/coroutine/io_service/detail/io_operation.hpp"
#include "ptl/expected.hpp"

#include <vector>
#include <unistd.h>

namespace ptl::experimental::coroutine::process
{

struct process_read_operation;
struct process_write_operation;
struct process_terminate_operation;

class subprocess
{
public:
    subprocess(iosvc::io_service& service)
        : service_(service.impl())
        , read_pipe_{0}
        , write_pipe_{0}
    {
    }
    subprocess(const subprocess&) = delete;
    subprocess(subprocess&&) = delete;
    subprocess& operator=(const subprocess&) = delete;
    subprocess& operator=(subprocess&&) = delete;

    ~subprocess()
    {
        service_.deregister_process_notification(ps_data_);

        if (read_pipe_.native_descriptor() != 0) {
            service_.close(read_pipe_.native_descriptor());
        }
        if (write_pipe_.native_descriptor() != 0) {
            service_.close(write_pipe_.native_descriptor());
        }
    }


    process_terminate_operation wait() noexcept;
    process_read_operation read(uint8_t* buffer, size_t sz) noexcept;
    process_write_operation write(const uint8_t* buffer, size_t sz) noexcept;

    iosvc::detail::expected<void> launch(std::string_view process, const std::vector<std::string_view>& args)
    {
        int stdin_fds[2];
        int stdout_fds[2];
    
        if (::pipe(stdin_fds) == -1) {
            return ptl::error_code{ errno };
        }
        if (::pipe(stdout_fds) == -1) {
            ::close(stdin_fds[0]); ::close(stdin_fds[1]);
            return ptl::error_code{ errno };
        }

        std::vector<char*> arguments;
        // use ranges
        arguments.push_back((char*)process.data());
        for (auto& element : args) {
            arguments.push_back((char*)element.data());
        }
        arguments.push_back(nullptr);

        auto pid = fork();
        if (pid == -1) {
            ::close(stdin_fds[0]); ::close(stdin_fds[1]);
            ::close(stdout_fds[0]); ::close(stdout_fds[1]);
            return ptl::error_code{ errno };
        }

        if (pid == 0) {
            // Child process, attach to stdout
            while (::dup2(stdin_fds[1], STDIN_FILENO) == -1 && errno == EINTR) {
            }
            while (::dup2(stdout_fds[1], STDOUT_FILENO) == -1 && errno == EINTR) {
            }
            while (::dup2(stdout_fds[1], STDERR_FILENO) == -1 && errno == EINTR) {
            }

            ::close(stdin_fds[0]); ::close(stdin_fds[1]);
            ::close(stdout_fds[0]); ::close(stdout_fds[1]);
            ::execv(process.data(), arguments.data());
            std::terminate();
        }
        // Parent process
        ::close(stdin_fds[1]);
        ::close(stdout_fds[1]);

        read_pipe_ = stdout_fds[0];
        write_pipe_ = stdin_fds[0];
        service_.register_process_notification(pid, ps_data_);
        return {};
    }

private:
    friend struct process_terminate_operation;
    friend struct process_read_operation;
    friend struct process_write_operation;

    bool terminated() const noexcept
    {
        return terminated_;
    }

    void start_io(iosvc::io_kind kind, iosvc::io_service_operation* op)
    {
        //service_.start_io(*data_.get(), kind, op);
    }

    void start_notification(iosvc::io_service_operation* op)
    {
        service_.start_notification(ps_data_, op);
    }

    iosvc::detail::expected_size_t read_(uint8_t* buffer, size_t sz)
    {
        return service_.read(read_pipe_.native_descriptor(), buffer, sz);
    }

    iosvc::detail::expected_size_t write_(const uint8_t* buffer, size_t sz)
    {
        return service_.write(write_pipe_.native_descriptor(), buffer, sz);
    }

    iosvc::detail::io_service_impl& service_;
    iosvc::detail::process_service_data ps_data_;

    iosvc::descriptor read_pipe_;
    iosvc::descriptor write_pipe_;

    bool terminated_ = false;
};

struct process_read_operation : iosvc::detail::io_xfer_operation<process_read_operation>, iosvc::io_service_operation
{
    process_read_operation(subprocess& p, void* buffer, size_t sz) noexcept
        : io_xfer_operation<process_read_operation>()
        , process_(p), buffer_(static_cast<uint8_t*>(buffer)), size_(sz), received_(0)
    {
    }

private:
    friend class iosvc::detail::io_operation<process_read_operation>;
    bool begin()
    {
        auto r = process_.read_(buffer_, size_);
        if (r.is_error()) {
            if (r.error().value() == EAGAIN || r.error().value() == EWOULDBLOCK) {
                // we need notification
                process_.start_io(iosvc::io_kind::read, this);
                return false;
            }
            ec_ = r.error();
            return true;
        }
        received_ = r.value();
        return true;
    }

    // called when io_service says there is something to do
    void work() override
    {
        auto r = process_.read_(buffer_, size_);
        if (r.is_error()) {
            ec_ = r.error();
        } else {
            received_ = r.value();
        }
        resume();
    }

    size_t get_return() const noexcept
    {
        return received_;
    }

    subprocess& process_;
    uint8_t* buffer_;
    size_t size_;
    size_t received_;
};

struct process_write_operation : iosvc::detail::io_xfer_operation<process_write_operation>, iosvc::io_service_operation
{
    process_write_operation(subprocess& p, void* buffer, size_t sz) noexcept
        : io_xfer_operation<process_write_operation>()
        , process_(p), buffer_(static_cast<uint8_t*>(buffer)), size_(sz), written_(0)
    {
    }

private:
    friend class iosvc::detail::io_operation<process_write_operation>;
    bool begin()
    {
        auto r = process_.write_(buffer_, size_);
        if (r.is_error()) {
            if (r.error().value() == EAGAIN || r.error().value() == EWOULDBLOCK) {
                // we need notification
                process_.start_io(iosvc::io_kind::write, this);
                return false;
            }
            ec_ = r.error();
            return true;
        }
        written_ = r.value();
        return true;
    }

    // called when io_service says there is something to do
    void work() override
    {
        auto r = process_.write_(buffer_, size_);
        if (r.is_error()) {
            ec_ = r.error();
        } else {
            written_ = r.value();
        }
        resume();
    }

    size_t get_return() const noexcept
    {
        return written_;
    }

    subprocess& process_;
    uint8_t* buffer_;
    size_t size_;
    size_t written_;
};



struct process_terminate_operation : iosvc::detail::io_operation<process_terminate_operation>, iosvc::io_service_operation
{
    process_terminate_operation(subprocess& p)
        : process_(p)
    {
    }

private:
    friend class iosvc::detail::io_operation<process_terminate_operation>;
    bool begin()
    {
        if (!process_.terminated()) {
            process_.start_notification(this);
            return false;
        }
        return true;
    }

    int get_return() const noexcept
    {
        return process_.ps_data_.rc;
    }

    void work() override
    {
        resume();
    }

    subprocess& process_;
};

inline
process_terminate_operation subprocess::wait() noexcept
{
    return { *this };
}

inline
process_read_operation subprocess::read(uint8_t* buffer, size_t sz) noexcept
{
    return { *this, buffer, sz };
}

}