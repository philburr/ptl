#include "catch2/catch.hpp"
#include "ptl/exceptable.hpp"

TEST_CASE("exceptable")
{
    {
        ptl::exceptable<uint32_t> v(3);
        REQUIRE(v.value() == 3);
    }

    {
        ptl::exceptable<uint32_t> v(std::make_exception_ptr(std::logic_error("bad")));
        REQUIRE_THROWS(v.value());
    }
}