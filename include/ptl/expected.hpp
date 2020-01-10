#pragma once
#include <type_traits>
#include "ptl/type_safe/crtp.hpp"
#include "ptl/packed_ptr.hpp"

namespace ptl {
namespace detail {

template <typename DERIVED>
struct ExpectedBase
{

protected:
    enum class Contains
    {
        EXPECTED,
        UNEXPECTED,
        NOTHING
    };
};

} // namespace detail

template <typename T, typename E>
struct Expected : public detail::ExpectedBase<Expected<T, E>>
{
    static_assert(!std::is_reference<T>::value, "Expected cannot wrap a reference");

public:
    typedef T type;

    Expected() noexcept
        : contains_(Contains::NOTHING)
    {}
    ~Expected()
    {}

    bool is_expected() const noexcept
    {
        return contains_ == Contains::EXPECTED;
    }
    bool is_unexcepted() const noexcept
    {
        return contains_ == Contains::UNEXPECTED;
    }

private:
    using Contains = typename detail::ExpectedBase<Expected<T, E>>::Contains;

    Contains contains_;
    union {
        T value_;
        E unexpected_;
    };
};

// This is the most optimal form, we store the enum and the 16-bit error_code in unused bits in the
// pointer.  So it can simultaneously hold all information in a single 64-bit register
template <>
struct Expected<int32_t, std::exception_ptr> : public detail::ExpectedBase<Expected<int32_t, std::exception_ptr>>
{
public:
    typedef int32_t type;

    Expected() noexcept
    {
        set_nothing();
    }
    explicit Expected(int32_t v) noexcept
    {
        set_expected(v);
    }
    explicit Expected(std::exception_ptr ex) noexcept
    {
        set_unexpected(ex);
    }

    bool is_expected() const noexcept
    {
        return which() == Contains::EXPECTED;
    }
    bool is_unexpected() const noexcept
    {
        return which() == Contains::UNEXPECTED;
    }

private:
    friend struct detail::ExpectedBase<Expected<int32_t, std::exception_ptr>>;

    // friendly
    Contains which() const noexcept
    {
        return static_cast<Contains>(data_ >> 48);
    }
    void set_unexpected(std::exception_ptr ptr) noexcept
    {
        uintptr_t data = *(uintptr_t *)&ptr;
        data_ = (data & (intptr_t(-1) >> 16)) | ((uintptr_t)Contains::UNEXPECTED << 48);
    }
    std::exception_ptr unexpected() const noexcept
    {
        assert(which() == Contains::UNEXPECTED);
        uintptr_t ptr = reinterpret_cast<uintptr_t>(data_ & (uintptr_t(-1) >> 16));
        // avoid conversion constructor
        return *(std::exception_ptr *)(&ptr);
    }
    void set_expected(int32_t v) noexcept
    {
        data_ = (uintptr_t)v | ((uintptr_t)Contains::EXPECTED << 48);
    }
    int32_t expected() const noexcept
    {
        assert(which() == Contains::EXPECTED);
        uintptr_t v = reinterpret_cast<uintptr_t>(data_ & (uintptr_t(-1) >> 16));
        return static_cast<int32_t>(v);
    }
    void set_nothing() noexcept
    {
        data_ = (uintptr_t)Contains::NOTHING << 48;
    }

    uintptr_t data_;
};

} // namespace ptl