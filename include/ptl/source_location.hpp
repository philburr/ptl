#pragma once
#include "compiler.hpp"
#include <cstdint>

namespace ptl {

// Remove when standardized
struct source_location {
    static constexpr source_location current(const char* file = __builtin_FILE(),
                                             const char* func = __builtin_FUNCTION(),
                                             int line = __builtin_LINE(),
                                             int col = __builtin_COLUMN()) noexcept
    {
        source_location loc;
        loc.file_ = file;
        loc.func_ = func;
        loc.line_ = line;
        loc.col_ = col;
        return loc;
    }

    constexpr source_location() noexcept
        : file_("unknown")
        , func_("unknown")
        , line_(0)
        , col_(0)
    {}

    constexpr std::uint_least32_t line() const noexcept
    {
        return line_;
    }
    constexpr std::uint_least32_t column() const noexcept
    {
        return col_;
    }
    constexpr const char* file_name() const noexcept
    {
        return file_;
    }
    constexpr const char* function_name() const noexcept
    {
        return func_;
    }

private:
    const char* file_;
    const char* func_;
    std::uint_least32_t line_;
    std::uint_least32_t col_;
};

}