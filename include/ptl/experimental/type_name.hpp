#pragma once
#include "static_string.hpp"

template <class T>
constexpr static_string type_name()
{
#ifdef __clang__
    static_string p = __PRETTY_FUNCTION__;
    return static_string(p.data() + 31, p.size() - 31 - 1);
#elif defined(__GNUC__)
    static_string p = __PRETTY_FUNCTION__;
#if __cplusplus < 201402
    return static_string(p.data() + 36, p.size() - 36 - 1);
#else
    return static_string(p.data() + 46, p.size() - 46 - 1);
#endif
#elif defined(_MSC_VER)
    static_string p = __FUNCSIG__;
    return static_string(p.data() + 38, p.size() - 38 - 7);
#endif
}