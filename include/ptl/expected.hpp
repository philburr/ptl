#pragma once
#include <type_traits>
#include "ptl/type_safe/crtp.hpp"
#include "ptl/packed_ptr.hpp"
#include "ptl/error.hpp"

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

struct error_policy_assert {};
struct error_policy_throw {};

template <typename T, typename E, typename EPOLICY = error_policy_assert>
struct expected : public detail::expected_base<expected<T, E>>
{
    //static_assert(!std::is_reference<T>::value, "Expected cannot wrap a reference");
    static_assert(!std::is_same_v<E, std::exception_ptr> || !std::is_same_v<EPOLICY, error_policy_assert>, "std::exception_ptr requires error_policy_throw");

public:
    using type = std::decay_t<T>;

    expected() noexcept
        : contains_(Contains::NOTHING)
    {}

    expected(const type &v) noexcept(std::is_nothrow_copy_constructible_v<type>)
        : contains_(Contains::EXPECTED)
        , value_(v)
    {}
    expected(type &&v) noexcept(std::is_nothrow_move_constructible_v<type>)
        : contains_(Contains::EXPECTED)
        , value_(std::move(v))
    {}
    template <typename... ARGS>
    expected(std::in_place_t, ARGS &&... args) noexcept(std::is_nothrow_constructible_v<type, ARGS...>)
        : contains_(Contains::EXPECTED)
        , value_(std::forward<ARGS>...)
    {}

    expected(const E &e) noexcept(std::is_nothrow_copy_constructible_v<E>)
        : contains_(Contains::UNEXPECTED)
        , unexpected_(e)
    {}
    expected(E &&e) noexcept(std::is_nothrow_move_constructible_v<E>)
        : contains_(Contains::UNEXPECTED)
        , unexpected_(std::move(e))
    {}

    expected(expected &&other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<type>, std::is_nothrow_move_constructible<E>>)
        : contains_(other.contains_)
    {
        if (contains_ == Contains::EXPECTED) {
            new (&value_) type(std::move(other.value_));
        }
        else if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(std::move(other.unexpected_));
        }
    }
    expected &operator=(expected &&other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<type>, std::is_nothrow_move_constructible<E>>)
    {
        if (&other == this) {
            return *this;
        }
        destroy();
        contains_ = other.contains_;
        if (contains_ == Contains::EXPECTED) {
            new (&value_) type(std::move(other.value_));
        }
        else if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(std::move(other.unexpected_));
        }
        return *this;
    }

    expected(const expected &other) noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<type>, std::is_nothrow_copy_constructible<E>>)
    {
        static_assert(std::is_copy_constructible<type>::value, "T must be copyable for Try<T> to be copyable");
        contains_ = other.contains_;
        if (contains_ == Contains::EXPECTED) {
            new (&value_) type(other.value_);
        }
        else if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(other.unexpected_);
        }
    }
    expected &operator=(const expected &other) noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<type>, std::is_nothrow_copy_constructible<E>>)
    {
        static_assert(std::is_copy_constructible<type>::value, "T must be copyable for Try<T> to be copyable");
        if (&other == this) {
            return *this;
        }
        destroy();
        contains_ = other.contains_;
        if (contains_ == Contains::EXPECTED) {
            new (&value_) type(other.value_);
        }
        else if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(other.unexpected_);
        }
        return *this;
    }

    ~expected()
    {
        if (contains_ == Contains::EXPECTED) {
            value_.~type();
        }
        else if (contains_ == Contains::UNEXPECTED) {
            unexpected_.~E();
        }
    }

    bool is_value() const noexcept
    {
        return contains_ == Contains::EXPECTED;
    }
    bool is_error() const noexcept
    {
        return contains_ == Contains::UNEXPECTED;
    }

    type& value() & noexcept(!error_policy_throws())
    {
        enforce(Contains::EXPECTED);
        return value_;
    }

    type&& value() && noexcept(!error_policy_throws())
    {
        enforce(Contains::EXPECTED);
        contains_ = Contains::NOTHING;
        return std::move(value_);
    }

    const type& value() const &  noexcept(!error_policy_throws())
    {
        enforce(Contains::EXPECTED);
        return value_;
    }

    E& error() noexcept(!error_policy_throws())
    {
        enforce(Contains::UNEXPECTED);
        return unexpected_;
    }

    const E& error() const noexcept(!error_policy_throws())
    {
        enforce(Contains::UNEXPECTED);
        return unexpected_;
    }

protected:
    void destroy() noexcept
    {
        Contains contained = std::exchange(contains_, Contains::NOTHING);
        if (contained == Contains::EXPECTED) {
            value_.~type();
        }
        else if (contained == Contains::UNEXPECTED) {
            unexpected_.~E();
        }
    }

private:
    using Contains = typename detail::expected_base<expected<T, E>>::Contains;

    Contains contains_;
    union {
        type value_;
        E unexpected_;
    };

    static constexpr bool error_policy_throws() noexcept {
        return std::is_same_v<EPOLICY, error_policy_throw>;
    }

    void enforce(const Contains kind) const noexcept(!error_policy_throws())
    {
        if constexpr (error_policy_throws()) {
            if (contains_ != kind) {
                if (contains_ == Contains::UNEXPECTED) {
                    if constexpr(std::is_base_of_v<std::exception_ptr, E>) {
                        std::rethrow_exception(unexpected_);
                    }
                }
                throw std::logic_error("invalid access");
            }
        } else {
            assert(contains_ == kind);
        }
    }
};

template <typename T, typename E, typename EPOLICY>
struct expected<T&, E, EPOLICY> : public detail::expected_base<expected<T, E>>
{
    //static_assert(!std::is_reference<T>::value, "Expected cannot wrap a reference");
    static_assert(!std::is_same_v<E, std::exception_ptr> || !std::is_same_v<EPOLICY, error_policy_assert>, "std::exception_ptr requires error_policy_throw");

public:
    typedef T* type;

    expected() noexcept
        : contains_(Contains::NOTHING)
    {}

    expected(T &v) noexcept(std::is_nothrow_copy_constructible_v<T>)
        : contains_(Contains::EXPECTED)
        , value_(std::addressof(v))
    {}
    expected(T &&v) noexcept(std::is_nothrow_move_constructible_v<T>)
        : contains_(Contains::EXPECTED)
        , value_(std::move(v))
    {}

    expected(const E &e) noexcept(std::is_nothrow_copy_constructible_v<E>)
        : contains_(Contains::UNEXPECTED)
        , unexpected_(e)
    {}
    expected(E &&e) noexcept(std::is_nothrow_move_constructible_v<E>)
        : contains_(Contains::UNEXPECTED)
        , unexpected_(std::move(e))
    {}

    expected(expected &&other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<T>, std::is_nothrow_move_constructible<E>>)
        : contains_(other.contains_)
    {
        if (contains_ == Contains::EXPECTED) {
            new (&value_) T(std::move(other.value_));
        }
        else if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(std::move(other.unexpected_));
        }
    }
    expected &operator=(expected &&other) noexcept(std::conjunction_v<std::is_nothrow_move_constructible<T>, std::is_nothrow_move_constructible<E>>)
    {
        if (&other == this) {
            return *this;
        }
        destroy();
        contains_ = other.contains_;
        if (contains_ == Contains::EXPECTED) {
            new (&value_) T*(std::move(other.value_));
        }
        else if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(std::move(other.unexpected_));
        }
        return *this;
    }

    expected(const expected &other) noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<T>, std::is_nothrow_copy_constructible<E>>)
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
    expected &operator=(const expected &other) noexcept(std::conjunction_v<std::is_nothrow_copy_constructible<T>, std::is_nothrow_copy_constructible<E>>)
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
        if (contains_ == Contains::UNEXPECTED) {
            unexpected_.~E();
        }
    }

    bool is_value() const noexcept
    {
        return contains_ == Contains::EXPECTED;
    }
    bool is_error() const noexcept
    {
        return contains_ == Contains::UNEXPECTED;
    }

    T& value() & noexcept(!error_policy_throws())
    {
        enforce(Contains::EXPECTED);
        return *value_;
    }

    T&& value() && noexcept(!error_policy_throws())
    {
        enforce(Contains::EXPECTED);
        contains_ = Contains::NOTHING;
        return std::move(*value_);
    }

    const T& value() const &  noexcept(!error_policy_throws())
    {
        enforce(Contains::EXPECTED);
        return *value_;
    }

    E& error() noexcept(!error_policy_throws())
    {
        enforce(Contains::UNEXPECTED);
        return unexpected_;
    }

    const E& error() const noexcept(!error_policy_throws())
    {
        enforce(Contains::UNEXPECTED);
        return unexpected_;
    }

protected:
    void destroy() noexcept
    {
        Contains contained = std::exchange(contains_, Contains::NOTHING);
        if (contained == Contains::UNEXPECTED) {
            unexpected_.~E();
        }
    }

private:
    using Contains = typename detail::expected_base<expected<T, E>>::Contains;

    Contains contains_;
    union {
        T* value_;
        E unexpected_;
    };

    static constexpr bool error_policy_throws() noexcept {
        return std::is_same_v<EPOLICY, error_policy_throw>;
    }

    void enforce(const Contains kind) const noexcept(!error_policy_throws())
    {
        if constexpr (error_policy_throws()) {
            if (contains_ != kind) {
                if (contains_ == Contains::UNEXPECTED) {
                    if constexpr(std::is_base_of_v<std::exception_ptr, E>) {
                        std::rethrow_exception(unexpected_);
                    }
                }
                throw std::logic_error("invalid access");
            }
        } else {
            assert(contains_ == kind);
        }
    }
};

template <typename E, typename EPOLICY>
struct expected<void, E, EPOLICY> : public detail::expected_base<expected<void, E>>
{
    static_assert(!std::is_same_v<E, std::exception_ptr> || !std::is_same_v<EPOLICY, error_policy_assert>, "std::exception_ptr requires error_policy_throw");
public:
    expected() noexcept
        : contains_(Contains::NOTHING)
    {}

    expected(const E &e) noexcept(std::is_nothrow_copy_constructible_v<E>)
        : contains_(Contains::UNEXPECTED)
        , unexpected_(e)
    {}
    expected(E &&e) noexcept(std::is_nothrow_move_constructible_v<E>)
        : contains_(Contains::UNEXPECTED)
        , unexpected_(std::move(e))
    {}

    expected(expected &&other) noexcept(std::is_nothrow_move_constructible_v<E>)
        : contains_(other.contains_)
    {
        if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(std::move(other.unexpected_));
        }
    }
    expected &operator=(expected &&other) noexcept(std::is_nothrow_move_constructible_v<E>)
    {
        if (&other == this) {
            return *this;
        }
        destroy();
        contains_ = other.contains_;
        if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(std::move(other.unexpected_));
        }
        return *this;
    }

    expected(const expected &other) noexcept(std::is_nothrow_copy_constructible_v<E>)
    {
        contains_ = other.contains_;
        if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(other.unexpected_);
        }
    }
    expected &operator=(const expected &other) noexcept(std::is_nothrow_copy_constructible_v<E>)
    {
        if (&other == this) {
            return *this;
        }
        destroy();
        contains_ = other.contains_;
        if (contains_ == Contains::UNEXPECTED) {
            new (&unexpected_) E(other.unexpected_);
        }
        return *this;
    }

    ~expected()
    {
        if (contains_ == Contains::UNEXPECTED) {
            unexpected_.~E();
        }
    }

    bool is_error() const noexcept
    {
        return contains_ == Contains::UNEXPECTED;
    }

    void value() const noexcept(!error_policy_throws())
    {
        enforce(Contains::NOTHING);
    }

    E& error() noexcept(!error_policy_throws())
    {
        enforce(Contains::UNEXPECTED);
        return unexpected_;
    }

    const E& error() const noexcept(!error_policy_throws())
    {
        enforce(Contains::UNEXPECTED);
        return unexpected_;
    }

protected:
    void destroy() noexcept
    {
        Contains contained = std::exchange(contains_, Contains::NOTHING);
        if (contained == Contains::UNEXPECTED) {
            unexpected_.~E();
        }
    }

private:
    using Contains = typename detail::expected_base<expected<void, E>>::Contains;

    Contains contains_;
    union {
        E unexpected_;
    };

    static constexpr bool error_policy_throws() noexcept {
        return std::is_same_v<EPOLICY, error_policy_throw>;
    }

    void enforce(const Contains kind) const noexcept(!error_policy_throws())
    {
        if constexpr (error_policy_throws()) {
            if (contains_ != kind) {
                if (contains_ == Contains::UNEXPECTED) {
                    if constexpr(std::is_base_of_v<std::exception_ptr, E>) {
                        std::rethrow_exception(unexpected_);
                    }
                }
                throw std::logic_error("invalid access");
            }
        } else {
            assert(contains_ == kind);
        }
    }
};

#if 0
// This is the most optimal form, we store the enum and the 16-bit error_code in unused bits in the
// pointer.  So it can simultaneously hold all information in a single 64-bit register
// It would be nice to make this a bit more generic
template <>
struct expected<int32_t, std::exception_ptr> : public detail::expected_base<expected<int16_t, std::exception_ptr>>
{
public:
    using type = int32_t;

    expected() noexcept
    {
        set_nothing();
    }
    explicit expected(type v) noexcept
    {
        set_value(v);
    }
    explicit expected(std::exception_ptr ex) noexcept
    {
        set_error(ex);
    }

    bool is_value() const noexcept
    {
        return discriminant() == 1;
    }
    bool is_error() const noexcept
    {
        return discriminant() == 0;
    }

    int16_t value() const
    {
        if (is_error()) {
            std::rethrow_exception(error());
        }
        uintptr_t v = reinterpret_cast<uintptr_t>(data_ & ~discriminant_bit);
        return static_cast<type>(v);
    }
    std::exception_ptr error() const noexcept
    {
        assert(is_error());
        // avoid conversion constructor
        return *(std::exception_ptr *)(&data_);
    }

private:
    friend struct detail::expected_base<expected<int32_t, std::exception_ptr>>;

    // friendly
    unsigned discriminant() const noexcept
    {
        // Store in highest bit.  Cheap for most compilers to test
        return static_cast<unsigned>(data_ >> discriminant_bit_position) & 1;
    }
    void set_error(std::exception_ptr ptr) noexcept
    {
        uintptr_t data = *(uintptr_t *)&ptr;
        data_ = data;
    }
    void set_value(int16_t v) noexcept
    {
        data_ = (uintptr_t)v | discriminant_bit;
    }
    void set_nothing() noexcept
    {
        data_ = 0 | discriminant_bit;
    }

    static constexpr int discriminant_bit_position = sizeof(uintptr_t) * 8 - 1;
    static constexpr uintptr_t discriminant_bit = 1ULL << discriminant_bit_position;

    static_assert((discriminant_bit & int_bit_traits<type>::MASK) == 0);
    static_assert((discriminant_bit & int_bit_traits<std::exception_ptr>::MASK) == 0);
    static_assert(std::is_trivially_copyable_v<type>);

    //static_assert(std::is_trivially_copyable_v<std::exception_ptr>);
    uintptr_t data_;
};
#endif

} // namespace ptl