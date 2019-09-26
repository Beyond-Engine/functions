#include <catch2/catch.hpp>

#include <functional>

#include "unique_function.hpp"

void func() {}

TEST_CASE("unique_function benchmark")
{

  SECTION("Invocation")
  {
    auto func_ptr = +[]() {};

    std::function<void()> function{[] {}};
    beyond::unique_function<void()> unique_function{[] {}};

    BENCHMARK("function")
    {
      func();
    }

    BENCHMARK("function pointer")
    {
      func_ptr();
    }

    BENCHMARK("std::function")
    {
      function();
    }

    BENCHMARK("beyond::unique_function")
    {
      unique_function();
    }
  }
}
