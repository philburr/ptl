#pragma once
#include <type_traits>

template<typename T, size_t Offset, size_t Bits>
struct bit_field_member {
    T value_;


    static_assert(std::is_standard_layout_v<T>);
    static_assert(Bits < sizeof(T) * 8);
    static_assert(Bits + Offset <= sizeof(T) * 8);

    static constexpr T mask0() { return (T(1) << Bits) - 1; }
    static constexpr T mask() { return mask0 << Offset; }

    constexpr operator T() const
    {
        return (value_ >> Offset) & mask0;
    }

    constexpr bit_field_member& operator=(T v)
    {
        assert(v <= mask0 && "Value must fit within bitfield");
        value_ = (value_ & ~mask) | (v << Offset);
        return *this;
    }

    constexpr bit_field_member& operator+=(T v)
    {
        assert(T(*this) + v <= mask0 && "Value must not overflow bitfield");
        value_ += v << Offset;
        return *this;
    }

    constexpr bit_field_member& operator-=(T v)
    {
        assert(T(*this) >= v && "Value must not underflow bitfield");
        value_ -= v << Offset;
        return *this;
    }

    constexpr bit_field_member& operator++()
    {
        return *this += 1;
    }

    constexpr bit_field_member& operator++(int)
    {
        bit_field_member tmp(*this);
        operator++();
        return tmp;
    }

    constexpr bit_field_member& operator--()
    {
        return *this -= 1;
    }

    constexpr bit_field_member& operator--(int)
    {
        bit_field_member tmp(*this);
        operator--();
        return tmp;
    }
};
static_assert(std::is_standard_layout_v<bit_field_member<int, 0, 2> >);

#define BEGIN_BITFIELD_TYPE(NAME, T) \
    union NAME \
    { \
        struct { T value; } wrapper; \
        NAME(T v = 0) { wrapper.value = v; } \
        NAME& operator=(T v) { wrapper.value = v; return *this; } \
        operator T&() { return wrapper.value; } \
        operator T() const { return wrapper.value; } \
        using type = T;

#define ADD_BITFIELD_MEMBER(memberName, offset, bits) \
        bit_field_member<type, offset, bits> memberName;

#define END_BITFIELD_TYPE() \
    };
