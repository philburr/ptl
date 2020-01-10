#pragma once

namespace ptl::experimental::coroutine::detail {

template<typename T>
struct is_coroutine_handle : std::false_type {};

template<typename PROMISE>
struct is_coroutine_handle<std::experimental::coroutine_handle<PROMISE>> : std::true_type {};

template<typename T>
struct is_valid_await_suspend_return_type : std::disjunction<std::is_void<T>, std::is_same<T, bool>, is_coroutine_handle<T>> {};

template<typename T, typename = std::void_t<>>
struct is_awaiter : std::false_type {};

template<typename T>
struct is_awaiter<T, std::void_t<decltype(std::declval<T>().await_ready()),
                                 decltype(std::declval<T>().await_suspend(std::declval<std::experimental::coroutine_handle<>>())),
                                 decltype(std::declval<T>().await_resume())>> :
                    std::disjunction<
                        std::is_constructible<bool, decltype(std::declval<T>().await_ready())>,
                        detail::is_valid_await_suspend_return_type<decltype(std::declval<T>().await_suspend(std::declval<std::experimental::coroutine_handle<>>()))>>
{};

template<typename T>
auto get_awaiter_impl(T&& value, int) noexcept(noexcept(static_cast<T&&>(value).operator co_await()))
    -> decltype(static_cast<T&&>(value).operator co_await())
{
    return static_cast<T&&>(value).operator co_await();
}

template<typename T>
auto get_awaiter_impl(T&& value, long) noexcept(noexcept(operator co_await(static_cast<T&&>(value))))
    -> decltype(operator co_await(static_cast<T&&>(value)))
{
    return operator co_await(static_cast<T&&>(value));
}

template<typename T, std::enable_if_t<is_awaiter<T&&>::value, int> = 0>
T&& get_awaiter_impl(T&& value, std::any) noexcept {
    return static_cast<T&&>(value);
}

template<typename T>
auto get_awaiter(T&& value) noexcept(noexcept(get_awaiter_impl(static_cast<T&&>(value), 123)))
    -> decltype(get_awaiter_impl(static_cast<T&&>(value), 123))
{
    return get_awaiter_impl(static_cast<T&&>(value), 123);
}


template<typename T, typename = std::void_t<>>
struct is_awaitable : std::false_type {};

template<typename T>
struct is_awaitable<T, std::void_t<decltype(get_awaiter(std::declval<T>()))>> : std::true_type {};

template<typename T>
constexpr bool is_awaitable_v = is_awaitable<T>::value;


}