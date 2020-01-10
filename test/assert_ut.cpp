#include "catch2/catch.hpp"
#include "ptl/type_safe/assert.hpp"
#include <stdexcept>

template<typename ... Args>
std::string string_format( const std::string& format, Args ... args )
{
    size_t size = snprintf( nullptr, 0, format.c_str(), args ... ) + 1;
    std::vector<char> buf(size);
    snprintf(&buf[0], size, format.c_str(), args ... );
    return std::string(&buf[0], &buf[0] + size - 1 );
}

struct throwing_handler
{
    static constexpr bool throwing_exception_is_allowed = true;

    template <typename... Args>
    PTL_NORETURN
    static void handle(const ptl::assert::source_location &loc, const char *expression, const char *message = nullptr)
    {
        if (message)
            throw std::logic_error(string_format("[debug assert] %s:%u: Assertion '%s' failed - %s.\n", loc.file_name, loc.line_number,
                        expression, message));
        else
            throw std::logic_error(string_format("[debug assert] %s:%u: Assertion '%s' failed.\n", loc.file_name, loc.line_number,
                        expression));
    };
};

enum {
    ASSERT_LEVEL_NONE = 0,
    ASSERT_LEVEL_CRITICAL = 1,
    ASSERT_LEVEL_NORMAL = 2,
    ASSERT_LEVEL_VERBOSE = 3,
    ASSERT_LEVEL_ALL = 99,
};

struct throwing_assertion_handler : throwing_handler, ptl::assert::set_level<ASSERT_LEVEL_NORMAL>
{};

#define ASSERT_CRITICAL(EXPR) ASSERT(EXPR, ptl::assert::level<ASSERT_LEVEL_CRITICAL>{}, throwing_assertion_handler{})
#define ASSERT_NORMAL(EXPR) ASSERT(EXPR, ptl::assert::level<ASSERT_LEVEL_NORMAL>{}, throwing_assertion_handler{})
#define ASSERT_VERBOSE(EXPR) ASSERT(EXPR, ptl::assert::level<ASSERT_LEVEL_VERBOSE>{}, throwing_assertion_handler{})


TEST_CASE("assertion")
{
    volatile bool is_true = true;
    REQUIRE_THROWS(ASSERT_CRITICAL(!is_true));
    REQUIRE_NOTHROW(ASSERT_CRITICAL(is_true));

    REQUIRE_NOTHROW(ASSERT_VERBOSE(false));
    REQUIRE_NOTHROW(ASSERT_VERBOSE(true));
    
    REQUIRE_THROWS(ASSERT(false, throwing_assertion_handler{}));
    REQUIRE_NOTHROW(ASSERT(true, throwing_assertion_handler{}));
}