#include "catch2/catch.hpp"
#include "ptl/expected.hpp"
#include <exception>

TEST_CASE("construction")
{
    SECTION("empty")
    {
        auto v = ptl::expected<int32_t, std::exception_ptr>();
    }
}