#pragma once
#include "compiler.hpp"
#include <type_traits>

namespace ptl {
namespace detail {

template <typename F, typename... A>
PTL_NOINLINE PTL_COLD auto invoke_cold(F &&f, A &&... a) -> decltype(static_cast<F &&>(f)(static_cast<A &&>(a)...))
{
    return static_cast<F &&>(f)(static_cast<A &&>(a)...);
}

} // namespace detail

template <typename E, typename Try, typename Catch, typename... CatchA>
PTL_ERASE_TRYCATCH auto catch_exception(Try &&t, Catch &&c, CatchA &&... a) ->
    typename std::common_type<decltype(static_cast<Try &&>(t)()),
                              decltype(static_cast<Catch &&>(c)(std::declval<E>(), static_cast<CatchA &&>(a)...))>::type
{
#if PTL_HAS_EXCEPTIONS
    try {
        return static_cast<Try &&>(t)();
    }
    catch (E e) {
        return invoke_cold(static_cast<Catch &&>(c), e, static_cast<CatchA &&>(a)...);
    }
#else
    [](auto &&...) {}(c, a...); // ignore
    return static_cast<Try &&>(t)();
#endif
}

template <typename Try, typename Catch, typename... CatchA>
PTL_ERASE_TRYCATCH auto catch_exception(Try &&t, Catch &&c, CatchA &&... a) ->
    typename std::common_type<decltype(static_cast<Try &&>(t)()),
                              decltype(static_cast<Catch &&>(c)(static_cast<CatchA &&>(a)...))>::type
{
#if PTL_HAS_EXCEPTIONS
    try {
        return static_cast<Try &&>(t)();
    }
    catch (...) {
        return invoke_cold(static_cast<Catch &&>(c), static_cast<CatchA &&>(a)...);
    }
#else
    [](auto &&...) {}(c, a...); // ignore
    return static_cast<Try &&>(t)();
#endif
}
} // namespace ptl
