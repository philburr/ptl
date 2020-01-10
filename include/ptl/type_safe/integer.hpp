#pragma once
#include "ptl/compiler.hpp"
#include <type_traits>

namespace ptl {
namespace detail {

template <typename T>
struct is_integer
    : std::integral_constant<bool, std::is_integral_v<T> && !std::is_same_v<T, bool> && !std::is_same_v<T, char>>
{};

template <typename From, typename To>
struct is_safe_integer_conversion
    : std::integral_constant<bool,
                             is_integer<From>::value && is_integer<To>::value &&
                                 ((sizeof(From) <= sizeof(To) && std::is_signed_v<From> == std::is_signed_v<To>) ||
                                  (sizeof(From) < sizeof(To) && std::is_unsigned_v<From> && std::is_signed_v<To>))>
{};

template <typename From, typename To>
using enable_if_safe_integer_conversion = typename std::enable_if<is_safe_integer_conversion<From, To>::value>::type;

template <typename From, typename To>
using enable_if_not_safe_integer_conversion =
    typename std::enable_if<!is_safe_integer_conversion<From, To>::value>::type;

} // namespace detail

template <typename TYPE> class integer
{
    static_assert(detail::is_integer<TYPE>::value, "must be an integer type");

public:
    using type = TYPE;
    integer() = delete;

    // Constructor for converting types
    template <typename T, std::enable_if_t<detail::is_safe_integer_conversion<T, TYPE>::value, int> = 0>
    constexpr integer(T const &value)
        : value_(value)
    {}
    template <typename T, std::enable_if_t<detail::is_safe_integer_conversion<T, TYPE>::value, int> = 0>
    constexpr integer(integer<T> const &value)
        : value_(static_cast<T>(value))
    {}

    // Constructor for non-converting types (deleted)
    template <typename T, std::enable_if_t<!detail::is_safe_integer_conversion<T, TYPE>::value, int> = 0>
    constexpr integer(T) = delete;
    template <typename T, std::enable_if_t<!detail::is_safe_integer_conversion<T, TYPE>::value, int> = 0>
    constexpr integer(integer<T> const &) = delete;

    // Copy for converting types
    template <typename T, std::enable_if_t<detail::is_safe_integer_conversion<T, TYPE>::value, int> = 0>
    PTL_ERASE constexpr integer &operator=(T const &value) noexcept
    {
        value_ = value;
        return *this;
    }
    template <typename T, std::enable_if_t<detail::is_safe_integer_conversion<T, TYPE>::value, int> = 0>
    PTL_ERASE constexpr integer &operator=(integer<T> const &value) noexcept
    {
        value_ = static_cast<T>(value);
        return *this;
    }

    // Copy for non-converting types (deleted)
    template <typename T, std::enable_if_t<!detail::is_safe_integer_conversion<T, TYPE>::value, int> = 0>
    integer &operator=(T) = delete;
    template <typename T, std::enable_if_t<!detail::is_safe_integer_conversion<T, TYPE>::value, int> = 0>
    integer &operator=(integer<T> const &) = delete;

    // Accessors
    PTL_ERASE explicit constexpr operator type() const noexcept
    {
        return value_;
    }
    PTL_ERASE constexpr type get() const noexcept
    {
        return value_;
    }

    // Unary Operators
    PTL_ERASE constexpr integer operator+() const noexcept
    {
        return *this;
    }
    template <typename T = TYPE, std::enable_if_t<std::is_signed_v<T>, int> = 0>
    PTL_ERASE constexpr integer operator-() const noexcept
    {
        return -value_;
    }

private:
    type value_;
};

} // namespace ptl