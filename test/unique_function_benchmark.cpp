#include <catch2/catch.hpp>

#include <functional>
#include <memory>

#include "unique_function.hpp"

void func() {}

struct A {
  virtual ~A() = default;
  virtual auto operator()() -> void = 0;
};

struct B : A {
  auto operator()() -> void override {}
};

TEST_CASE("unique_function benchmark")
{

  SECTION("Invocation")
  {
    auto func_ptr = +[]() {};

    std::function<void()> function{[] {}};
    beyond::unique_function<void()> unique_function{[] {}};
    std::unique_ptr<A> b = std::make_unique<B>();

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

    BENCHMARK("virtual function")
    {
      (*b)();
    }

    BENCHMARK("beyond::unique_function")
    {
      unique_function();
    }
  }
}
