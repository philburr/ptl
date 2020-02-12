#pragma once
#if !defined(__clang__)
#error Unsupported compiler
#endif
#include <system_error>
#include <experimental/coroutine>


namespace ptl::experimental::coroutine::asio::detail {

template<typename T>
using expected = ptl::expected<T, ptl::error_code, ptl::error_policy_assert>;

template <typename DERIVED>
struct io_operation : public ptl::experimental::coroutine::asio::io_service_operation
{
private:
    std::experimental::coroutine_handle<> coroutine_ = nullptr;

    void get_return_value()
    {
        return static_cast<DERIVED*>(this)->get_return();
    }

protected:
    ptl::error_code ec_;

public:
    bool await_ready() {
        return static_cast<DERIVED*>(this)->begin();
    }

    void await_suspend(std::experimental::coroutine_handle<> awaiting_coroutine) {
        coroutine_ = awaiting_coroutine;
    }

    decltype(auto) await_resume() {
        using derived_return_type = decltype(std::declval<DERIVED>().get_return());
        using return_type = expected<derived_return_type>;

        if (ec_.value() != 0) {
            return_type{ ec_ };
        }
        if constexpr(std::is_same_v<derived_return_type, void>) {
            // in case there is side effect
            static_cast<DERIVED*>(this)->get_return();
            return return_type{};
        } else {
            return return_type{ static_cast<DERIVED*>(this)->get_return() };
        }
    }

    void resume() {
        coroutine_.resume();
    }

};

template <typename DERIVED>
struct io_xfer_operation : public io_operation<DERIVED>
{
    size_t get_return() {
        return transferred_;
    }


protected:
    size_t transferred_;
};

} // namespace ptl::experimental::coroutine::asio::detail