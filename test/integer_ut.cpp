#include "catch2/catch.hpp"
#include "ptl/type_safe/integer.hpp"
#include "ptl/type_safe/assert.hpp"

TEST_CASE("Integer", "Construction")
{
    auto i = ptl::integer<int32_t>(3);
    auto j = -i;
    CHECK((int)j == -3);
}