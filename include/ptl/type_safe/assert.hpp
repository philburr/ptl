#pragma once
#include "ptl/compiler.hpp"
#include <utility>
#include <cstdlib>

namespace ptl {
namespace assert {

struct source_location
{
    const char *file_name; ///< The file name.
    unsigned line_number;  ///< The line number.
};

#define ASSERT_SOURCE_LOCATION                                                                                         \
    ptl::assert::source_location                                                                                       \
    {                                                                                                                  \
        __FILE__, static_cast<unsigned>(__LINE__)                                                                      \
    }

struct no_handler
{
    template <typename... Args>
    static void handle(const source_location &, const char *, Args &&...) noexcept
    {}
};

struct default_handler
{
    static void handle(const source_location &loc, const char *expression, const char *message = nullptr) noexcept;
};

template <unsigned Level>
struct level
{};

template <unsigned Level>
struct set_level
{
    static constexpr unsigned level = Level;
};

template <class Handler, typename = void>
struct allows_exception
{
    static const bool value = false;
};

template <class Handler>
struct allows_exception<Handler, typename std::enable_if<Handler::throwing_exception_is_allowed>::type>
{
    static constexpr bool value = Handler::throwing_exception_is_allowed;
};

template <typename Handler, typename... Args>
PTL_NORETURN void assertion_failed(const source_location &loc, const char *expression, Args &&... args)
{
    Handler::handle(loc, expression, std::forward<Args>(args)...);
    std::abort();
}

template <typename Expr, typename Handler, unsigned Level, typename... Args>
constexpr auto check_assertion(const Expr &expr, const source_location &loc, const char *expression,
                               level<Level>, Handler,
                               Args &&... args) noexcept(!allows_exception<Handler>::value ||
                                                         noexcept(Handler::handle(loc, expression,
                                                                                  std::forward<Args>(args)...))) ->
    typename std::enable_if<Level <= Handler::level, void>::type
{
    static_assert(Level > 0, "level of an assertion must not be 0");
    if (!expr()) {
        assertion_failed<Handler>(loc, expression, std::forward<Args>(args)...);
    }
}

template <typename Expr, typename Handler, unsigned Level, typename... Args>
PTL_FORCE_INLINE constexpr auto check_assertion(const Expr &, const source_location &, const char *, level<Level>,
                                                Handler, Args &&...) noexcept ->
    typename std::enable_if<(Level > Handler::level), void>::type
{
    return;
}

template <typename Expr, typename Handler, typename... Args>
constexpr auto check_assertion(const Expr &expr, const source_location &loc, const char *expression, Handler,
                               Args &&... args) noexcept(!allows_exception<Handler>::value ||
                                                         noexcept(Handler::handle(loc, expression,
                                                                                  std::forward<Args>(args)...))) ->
    typename std::enable_if<Handler::level != 0, void>::type
{
    if (!expr()) {
        assertion_failed<Handler>(loc, expression, std::forward<Args>(args)...);
    }
}

template <class Expr, class Handler, typename... Args>
PTL_FORCE_INLINE constexpr auto check_assertion(const Expr &, const source_location &, const char *, Handler,
                                                Args &&...) noexcept ->
    typename std::enable_if<Handler::level == 0, void>::type
{
    return;
}

} // namespace assert
} // namespace ptl

#ifndef NO_ASSERT
#define ASSERT(EXPR, ...)                                                                                              \
    ptl::assert::check_assertion([&]() noexcept -> bool { return EXPR; }, ASSERT_SOURCE_LOCATION, #EXPR, __VA_ARGS__)

#else
#define ASSERT(EXPR, ...) static_cast<void>(0)
#endif