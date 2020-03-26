#pragma once
#include <cstdint>
#include <type_traits>
#include <exception>
#include <string>
#include "source_location.hpp"

namespace ptl
{
static struct error_location_tag {} error_location;

struct error_code {
    using type = int32_t;

    error_code() noexcept : value_(0) 
    {}
    
    explicit error_code(const type v) noexcept : value_(v)
    {}

    template<typename STR, std::enable_if_t<std::is_convertible_v<std::remove_cv_t<std::remove_reference_t<STR>>, std::string>, void> = 0>
    error_code(const type v, STR&& reason) noexcept : value_(v)
    {
        ext_ = { {}, std::forward<STR>(reason) };
    }

    error_code(error_location_tag, const type v, source_location loc = source_location::current())
        : value_(v)
    {
        ext_ = { std::move(loc) };
    }
    template<typename STR, std::enable_if_t<std::is_convertible_v<std::remove_cv_t<std::remove_reference_t<STR>>, std::string>, void> = 0>
    error_code(error_location_tag, const type v, STR&& reason, source_location loc = source_location::current())
        : value_(v)
    {
        ext_ = { std::move(loc), std::forward<STR>(reason) };
    }


    explicit operator type() const noexcept {
        return value_;
    }
    bool operator==(const error_code& o) const noexcept {
        return value_ == o.value_;
    }
    constexpr type value() const noexcept {
        return value_;
    }

    struct error_code_ext {
        error_code_ext() = default;

        template<typename STR, std::enable_if_t<std::is_convertible_v<std::remove_cv_t<std::remove_reference_t<STR>>, std::string>, void> = 0>
        error_code_ext(const source_location& loc, STR&& reason) : loc_ { loc }, reason_ { std::forward<STR>(reason) } {}
        error_code_ext(const source_location& loc) : loc_{ loc }, reason_{} {}

        const source_location& location() const noexcept
        {
            return loc_;
        }

    private:
        source_location loc_;
        std::string reason_;
    };

    const error_code_ext& extended() const {
        return ext_;
    }
private:
    type value_;

    thread_local static inline error_code_ext ext_;
};
static_assert(std::is_trivially_copyable_v<error_code>);

template<typename T>
struct int_bit_traits {};
template<>
struct int_bit_traits<int32_t>
{
    static constexpr size_t BITS = 32;
    static constexpr size_t SHIFT = 0;
    static constexpr uintptr_t MASK = ((1ULL << BITS) - 1) << SHIFT;
};
template<>
struct int_bit_traits<uint32_t> : int_bit_traits<int32_t> {};
template<>
struct int_bit_traits<error_code> : int_bit_traits<error_code::type> {};

template<>
struct int_bit_traits<void*>
{
    static constexpr size_t BITS = 48;
    static constexpr size_t SHIFT = 0;
    static constexpr uintptr_t MASK = ((1ULL << BITS) - 1) << SHIFT;
};

template<>
struct int_bit_traits<std::exception_ptr> : int_bit_traits<void*> {};

}