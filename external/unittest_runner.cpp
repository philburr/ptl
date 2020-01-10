#define CATCH_CONFIG_RUNNER
#include "catch2/catch.hpp"
#include <cstdio>
#include <string>

int main(int argc, char *argv[])
{
    return Catch::Session().run(argc, argv);
}