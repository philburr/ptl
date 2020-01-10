#pragma once
#include <type_traits>

namespace ptl {

template <typename T, template <typename> class = std::void_t>
struct crtp
{
    T &derived()
    {
        return static_cast<T &>(*this);
    }
    T const &derived() const
    {
        return static_cast<T const &>(*this);
    }
};

}