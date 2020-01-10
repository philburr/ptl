#pragma once
#if !defined(__clang__)
#error Unsupported compiler
#endif
#include <system_error>
#include <experimental/coroutine>


namespace ptl::experimental::asio::detail {

template <typename DERIVED>
struct socket_operation : public ptl::asio::io_service_operation
{
    bool await_ready() {
        return static_cast<DERIVED*>(this)->begin();
    }

    void await_suspend(std::experimental::coroutine_handle<> awaiting_coroutine) {
        coroutine_ = awaiting_coroutine;
    }

    decltype(auto) await_resume() {
        if (ec_.value() != 0) {
            throw std::system_error(ec_);
        }
        return static_cast<DERIVED*>(this)->get_return();
    }

    void resume() {
        coroutine_.resume();
    }

private:
    std::experimental::coroutine_handle<> coroutine_ = nullptr;
protected:
    std::error_code ec_;
};

template <typename DERIVED>
struct socket_xfer_operation : public socket_operation<DERIVED>
{
    size_t get_return() {
        return transferred_;
    }


protected:
    size_t transferred_;
};

} // namespace ptl::experimental::asio::detail