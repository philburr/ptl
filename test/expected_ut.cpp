#include "catch2/catch.hpp"
#include "ptl/expected.hpp"
#include <exception>

using namespace ptl;
expected<void, error_code> returns_null()
{
    return {};
}
expected<void, error_code> returns_null_error()
{
    return error_code{ error_location, -3 };
}

TEST_CASE("error_code")
{
    SECTION("empty")
    {
        auto v = ptl::expected<int32_t, error_code>();
        REQUIRE(sizeof(v) == sizeof(uintptr_t));
    }
    SECTION("returns unexpected")
    {
        auto v = returns_null_error();
        REQUIRE(v.error().value() == -3);
        REQUIRE(v.error().extended().location().function_name() == "returns_null_error");
    }
}


template<typename T, typename... ARGS>
auto make_exception(ARGS&&... args)
{
    static_assert(std::is_base_of_v<std::exception, T>);
    return expected<int32_t, std::exception_ptr, error_policy_throw>(std::make_exception_ptr(T(std::forward<ARGS>(args)...)));
}

TEST_CASE("error_exception")
{
    using ert = expected<int32_t, std::exception_ptr, error_policy_throw>;

    SECTION("construction/size")
    {
        auto v = ert();
        REQUIRE(sizeof(v) == 2*sizeof(uintptr_t));
    }

    SECTION("does it throw")
    {
        auto v = make_exception<std::invalid_argument>("bad arg");
        REQUIRE(v.is_error() == true);
        REQUIRE(v.is_value() == false);
        REQUIRE_THROWS_AS(v.value(), std::invalid_argument);
    }

    SECTION("does it not throw")
    {
        auto v = ert(0);
        REQUIRE(v.is_error() == false);
        REQUIRE(v.is_value() == true);
        REQUIRE(v.value() == 0);
    }
}