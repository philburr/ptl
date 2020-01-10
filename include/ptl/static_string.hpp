#pragma once

// https://stackoverflow.com/a/20170989

#include <cstddef>
#include <stdexcept>
#include <cstring>
#include <ostream>

class static_string
{
    const char *const p_;
    const std::size_t sz_;

public:
    typedef const char *const_iterator;

    template <std::size_t N>
    constexpr static_string(const char (&a)[N]) noexcept
        : p_(a)
        , sz_(N - 1)
    {}

    constexpr static_string(const char *p, std::size_t N) noexcept
        : p_(p)
        , sz_(N)
    {}

    constexpr const char *data() const noexcept
    {
        return p_;
    }
    constexpr std::size_t size() const noexcept
    {
        return sz_;
    }

    constexpr const_iterator begin() const noexcept
    {
        return p_;
    }
    constexpr const_iterator end() const noexcept
    {
        return p_ + sz_;
    }

    constexpr char operator[](std::size_t n) const
    {
        return n < sz_ ? p_[n] : throw std::out_of_range("static_string");
    }
};

inline std::ostream &operator<<(std::ostream &os, static_string const &s)
{
    return os.write(s.data(), s.size());
}
