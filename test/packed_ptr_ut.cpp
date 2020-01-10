#include "catch2/catch.hpp"
#include "ptl/packed_ptr.hpp"
#include <exception>

#include <cstdio>

TEST_CASE("construction")
{
    auto int_ptr = ptl::PackedPtr<int>();
    auto int_bool_ptr = ptl::PackedPtr<int, bool>();
    auto exc_ptr = ptl::PackedPtr<std::exception_ptr>();
    auto exc_char_ptr = ptl::PackedPtr<std::exception_ptr, char>();
    auto exc_bool_ptr = ptl::PackedPtr<std::exception_ptr, bool>();

    exc_ptr.set(std::make_exception_ptr(std::logic_error("messed up!")));
    exc_ptr.set_value(0xffff);
    exc_ptr.set_small(0x7);
    printf("%lx\n", exc_ptr.internal());
}