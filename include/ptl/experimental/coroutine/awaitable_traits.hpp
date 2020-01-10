#pragma once
#if !defined(__clang__)
#error Unsupported compiler
#endif
#include <experimental/coroutine>

#include <type_traits>
#include <any>
#include "detail/is_awaiter.hpp"

namespace ptl::experimental::coroutine {

template<typename T, typename = void>
struct awaitable_traits {};

template<typename T>
struct awaitable_traits<T, std::void_t<decltype(detail::get_awaiter(std::declval<T>()))>>
{
    using awaiter_t = decltype(detail::get_awaiter(std::declval<T>()));
    using await_result_t = decltype(std::declval<awaiter_t>().await_resume());
};

}