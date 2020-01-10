#pragma once
#include <type_traits>
#include "ptl/type_safe/crtp.hpp"
#include "ptl/packed_ptr.hpp"

namespace ptl {
namespace detail {

template <typename DERIVED>
struct expected_base
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
struct expected : public detail::expected_base<expected<T, E>>
{
    static_assert(!std::is_reference<T>::value, "Expected cannot wrap a reference");

public:
    typedef T type;

    expected() noexcept
        : contains_(Contains::NOTHING)
    {}

    explicit expected(const T &v) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : contains_(Contains::EXPECTED)
        , value_(v)
    {}
    explicit expected(T &&v) noexcept(std::is_nothrow_move_constructible_v<T>)
        : contains_(Contains::EXPECTED)
        , value_(std::move(v))
    {}
    template <typename... ARGS>
    explicit expected(std::in_place_t, ARGS &&... args) noexcept(std::is_nothrow_constructible_v<T, ARGS...>)
        : contains_(Contains::EXPECTED)
        , value_(std::forward<ARGS>...)
    {}

    explicit expected(const E &e) noexcept(std::is_nothrow_copy_constructible_v<E>)
        : contains_(Contains::UNEXPECTED)
        , unexpected_(e)
    {}
    explicit expected(E &&e) noexcept(std::is_nothrow_move_constructible_v<E>)
        : contains_(Contains::UNEXPECTED)
        , unexpected_(std::move(e))
    {}

    expected(expected &&other) noexcept(std::is_nothrow_move_constructible_v<T>)
        : contains_(other.contains_)
    {
        if (contains_ == Contains::EXPECTED) {
            new (value_) T(std::move(other.value_));
        }
        else if (contains_ == Contains::UNEXPECTED) {
            new (unexpected_) E(std::move(other.unexpected_));
        }
    }
    expected &operator=(expected &&other) noexcept(std::is_nothrow_move_constructible<T>::value)
    {
        if (&other == this) {
            return *this;
        }
        destroy();
        contains_ = other.contains_;
        if (contains_ == Contains::EXPECTED) {
            new (value_) T(std::move(other.value_));
        }
        else if (contains_ == Contains::UNEXPECTED) {
            new (unexpected_) E(std::move(other.unexpected_));
        }
        return *this;
    }

    expected(const expected &other) noexcept(std::is_nothrow_copy_constructible<T>::value)
    {
        static_assert(std::is_copy_constructible<T>::value, "T must be copyable for Try<T> to be copyable");
        contains_ = other.contains_;
        if (contains_ == Contains::EXPECTED) {
            new (&value_) T(other.value_);
        }
        else if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(other.unexpected_);
        }
    }
    expected &operator=(const expected &other) noexcept(std::is_nothrow_copy_constructible<T>::value)
    {
        static_assert(std::is_copy_constructible<T>::value, "T must be copyable for Try<T> to be copyable");
        if (&other == this) {
            return *this;
        }
        destroy();
        contains_ = other.contains_;
        if (contains_ == Contains::EXPECTED) {
            new (&value_) T(other.value_);
        }
        else if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(other.unexpected_);
        }
        return *this;
    }

    ~expected()
    {
        if (contains_ == Contains::EXPECTED) {
            value_.~T();
        }
        else if (contains_ == Contains::UNEXPECTED) {
            unexpected_.~E();
        }
    }

    bool is_expected() const noexcept
    {
        return contains_ == Contains::EXPECTED;
    }
    bool is_unexpected() const noexcept
    {
        return contains_ == Contains::UNEXPECTED;
    }

    T& value() & noexcept
    {
        assert (contains_ == Contains::EXPECTED);
        return value_;
    }

    T&& value() && noexcept
    {
        assert(contains_ == Contains::EXPECTED);
        contains_ = Contains::NOTHING;
        return std::move(value_);
    }

    const T& value() const &  noexcept
    {
        assert(contains_ == Contains::EXPECTED);
        return value_;
    }

    E& unexpected() noexcept
    {
        assert(contains_ == Contains::UNEXPECTED);
        return unexpected_;
    }

    const E& unexpected() const noexcept
    {
        assert(contains_ == Contains::UNEXPECTED);
        return unexpected_;
    }

protected:
    void destroy() noexcept
    {
        Contains contained = std::exchange(contains_, Contains::NOTHING);
        if (contained == Contains::EXPECTED) {
            value_.~T();
        }
        else if (contained == Contains::UNEXPECTED) {
            unexpected_.~E();
        }
    }

private:
    using Contains = typename detail::expected_base<expected<T, E>>::Contains;

    Contains contains_;
    union {
        T value_;
        E unexpected_;
    };
};

#if 0
// This is the most optimal form, we store the enum and the 16-bit error_code in unused bits in the
// pointer.  So it can simultaneously hold all information in a single 64-bit register
template <>
struct expected<int32_t, std::exception_ptr> : public detail::expected_base<expected<int32_t, std::exception_ptr>>
{
public:
    typedef int32_t type;

    expected() noexcept
    {
        set_nothing();
    }
    explicit expected(int32_t v) noexcept
    {
        set_expected(v);
    }
    explicit expected(std::exception_ptr ex) noexcept
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
    friend struct detail::expected_base<expected<int32_t, std::exception_ptr>>;

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
    int32_t _expected() const noexcept
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
#endif

} // namespace ptl