#pragma once

#include <cstdint>
#include <cstdlib>

namespace ptl {
namespace detail {

// FNV1a constants
constexpr uint32_t val_32_const = 0x811c9dc5;
constexpr uint32_t prime_32_const = 0x1000193;
constexpr uint64_t val_64_const = 0xcbf29ce484222325;
constexpr uint64_t prime_64_const = 0x100000001b3;

} //namespace detail

inline constexpr uint32_t fnv1a_32(const char* const str, const uint32_t value = detail::val_32_const) noexcept {
    return (str[0] == '\0') ? value : fnv1a_32(&str[1], (value ^ uint32_t(str[0])) * detail::prime_32_const);
}

inline constexpr uint64_t fnv1a_64(const char* const str, const uint64_t value = detail::val_64_const) noexcept {
    return (str[0] == '\0') ? value : fnv1a_64(&str[1], (value ^ uint64_t(str[0])) * detail::prime_64_const);
}


} //namespace ptl