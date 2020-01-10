#include "ptl/type_safe/assert.hpp"
#include <cstdio>

PTL_WEAK
void ptl::assert::default_handler::handle(const source_location &loc, const char *expression,
                                          const char *message) noexcept
{
    if (*expression == '\0') {
        if (message)
            std::fprintf(stderr, "[debug assert] %s:%u: Unreachable code reached - %s.\n", loc.file_name,
                         loc.line_number, message);
        else
            std::fprintf(stderr, "[debug assert] %s:%u: Unreachable code reached.\n", loc.file_name, loc.line_number);
    }
    else if (message)
        std::fprintf(stderr, "[debug assert] %s:%u: Assertion '%s' failed - %s.\n", loc.file_name, loc.line_number,
                     expression, message);
    else
        std::fprintf(stderr, "[debug assert] %s:%u: Assertion '%s' failed.\n", loc.file_name, loc.line_number,
                     expression);
}