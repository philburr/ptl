#pragma once
#include "ptl/type_safe/crtp.hpp"

namespace ptl {
namespace traits {

template <typename T> struct Addable : crtp<T, Addable>
{
    constexpr T operator+(T const &other) const
    {
        return T(this->derived().get() + other.get());
    }
    constexpr T operator-(T const &other) const
    {
        return T(this->derived().get() - other.get());
    }
};

template <typename T> struct Multiplicable : crtp<T, Multiplicable>
{
    constexpr T operator*(T const &other) const
    {
        return T(this->derived().get() * other.get());
    }
    constexpr T operator/(T const &other) const
    {
        return T(this->derived().get() / other.get());
    }
    constexpr T operator%(T const &other) const
    {
        return T(this->derived().get() % other.get());
    }
};

template <typename T> struct Negatable : crtp<T, Negatable>
{
    constexpr T operator-() const
    {
        return T(-this->derived().get());
    }
};

template <typename T> struct EqualityComparable : crtp<T, EqualityComparable>
{
    constexpr bool operator==(T const &other) const
    {
        return this->derived().get() == other.get();
    }
    constexpr bool operator!=(T const &other) const
    {
        return !(*this == other);
    }
};

template <typename T> struct Comparable : EqualityComparable<T>
{
    constexpr bool operator>(T const &other) const
    {
        return this->derived().get() > other.get();
    }
    constexpr bool operator<(T const &other) const
    {
        return this->derived().get() < other.get();
    }
    constexpr bool operator<=(T const &other) const
    {
        return !(*this > other);
    }
    constexpr bool operator>=(T const &other) const
    {
        return !(*this < other);
    }
};

template <typename T> struct Binopable : crtp<T, Binopable>
{
    constexpr T operator&(T const &other) const
    {
        return T(this->derived().get() & other.get());
    }
    constexpr T operator|(T const &other) const
    {
        return T(this->derived().get() | other.get());
    }
    constexpr T operator^(T const &other) const
    {
        return T(this->derived().get() ^ other.get());
    }
};

} // namespace traits

template <typename T, typename TAG, template <typename> class... INTERFACES>
struct StrongType : public INTERFACES<StrongType<T, TAG, INTERFACES...>>...
{
    using underlying_type = T;
    using self_type = StrongType<T, TAG, INTERFACES...>;
    using self_ref_type = StrongType<T &, TAG, INTERFACES...>;

    constexpr StrongType()
        : _value(underlying_type())
    {}
    explicit constexpr StrongType(underlying_type const &v)
        : _value(v)
    {}

    explicit constexpr operator underlying_type() const
    {
        return _value;
    }
    explicit operator self_ref_type()
    {
        return self_ref_tpe(_value);
    }
    constexpr strong_type &operator=(underlying_type v)
    {
        _value = v;
        return *this;
    }

    constexpr underlying_type &get()
    {
        return _value;
    }
    constexpr underlying_type const &get() const
    {
        return _value;
    }

private:
    underlying_type _value;
};

} // namespace ptl