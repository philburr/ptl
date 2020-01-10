#pragma once
#include "ptl/compiler.hpp"
#include <type_traits>

namespace ptl {
namespace detail {

struct default_arithmetic_checks
{
    template <typename T>
    PTL_ERASE static constexpr T addition(const T &a, const T &b) noexcept
    {
        return a + b;
    }

    template <typename T>
    PTL_ERASE static constexpr T subtraction(const T &a, const T &b) noexcept
    {
        return a - b;
    }

    template <typename T>
    PTL_ERASE static constexpr T multiplication(const T &a, const T &b) noexcept
    {
        return a * b;
    }

    template <typename T>
    PTL_ERASE static constexpr T division(const T &a, const T &b) noexcept
    {
        return a / b;
    }

    template <typename T>
    PTL_ERASE static constexpr T modulo(const T &a, const T &b) noexcept
    {
        return a % b;
    }
};

struct signed_integer_tag
{};
struct unsigned_integer_tag
{};

template <typename T>
using integer_tag = typename std::conditional<
    std::is_integral<T>::value,
    typename std::conditional<std::is_signed<T>::value, signed_integer_tag, unsigned_integer_tag>::value, void>::value;

template <typename T>
constexpr bool is_addition_undefined(const T &a, const T &b, signed_integer_tag)
{
    if (b < T(0)) {
        return a < std::numeric_limits<T>::min() - b;
    }
    else {
        return a > std::numeric_limits<T>::max() - b;
    }
}

template <typename T>
constexpr bool is_addition_undefined(const T &a, const T &b, unsigned_integer_tag)
{
    return a > std::numeric_limits<T>::max() - b;
}

template <typename T>
constexpr bool is_subtraction_undefined(const T &a, const T &b, signed_integer_tag)
{
    if (b < T(0)) {
        return a > std::numeric_limits<T>::max + b;
    }
    else {
        return a < std::numeric_limits<T>::min + b;
    }
}

template <typename T>
constexpr bool is_subtraction_undefined(const T &a, const T &b, unsigned_integer_tag)
{
    return a < b;
}

template <typename T>
constexpr bool is_multiplcation_undefined(const T &a, const T &b, signed_integer_tag)
{
    if (a <= T(0)) {
        if (b <= T(0)) {
            return a != T(0) && b < std::numeric_limits<T>::max() / a;
        } else {
            return a < std::numeric_limits<T>::min() / b;
        }
    } else {
        if (b <= T(0)) {
            return b < std::numeric_limits<T>::min() / a;
        } else {
            return a > std::numeric_limits<T>::max() / b;
        }
    }
}

template<typename T>
constexpr bool is_multiplcation_undefined(const T& a, const T& b, unsigned_integer_tag)
{
    return b != T(0) && a > std::numeric_limits<T>::max() / b;
}

template<typename T>
constexpr bool is_division_undefined(const T& a, const T& b, signed_integer_tag)
{
    // Divide by zero = UB
    // Divide by -1 changes the sign of a and std::numeric_limits<T>::min() has no positive equivalent
    return b == T(0) || (b == T(-1) && a == std::numeric_limits<T>::min());
}

template<typename T>
constexpr bool is_division_undefined(const T& a, const T& b, unsigned_integer_tag)
{
    // Divide by zero = UB
    return b == T(0);
}

template<typename T>
constexpr bool is_modulo_undefined(const T& a, const T& b, signed_integer_tag)
{
    // Divide by zero = UB
    return b == T(0);
}

template<typename T>
constexpr bool is_modulo_undefined(const T& a, const T& b, unsigned_integer_tag)
{
    // Divide by zero = UB
    return b == T(0);
}

struct ub_arithmetic_checks
{
    template <typename T>
    PTL_ERASE static constexpr T addition(const T &a, const T &b) noexcept
    {
        return a + b;
    }

    template <typename T>
    PTL_ERASE static constexpr T subtraction(const T &a, const T &b) noexcept
    {
        return a - b;
    }

    template <typename T>
    PTL_ERASE static constexpr T multiplication(const T &a, const T &b) noexcept
    {
        return a * b;
    }

    template <typename T>
    PTL_ERASE static constexpr T division(const T &a, const T &b) noexcept
    {
        return a / b;
    }

    template <typename T>
    PTL_ERASE static constexpr T modulo(const T &a, const T &b) noexcept
    {
        return a % b;
    }
};

} // namespace detail
} // namespace ptl