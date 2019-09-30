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

struct C {
  virtual ~C() = default;
  virtual auto f1() const -> void = 0;
  virtual auto f2() const -> void = 0;
  virtual auto f3() const -> void = 0;
};

struct D : C {
  auto f1() const -> void override {}
  auto f2() const -> void override
  {
    f1();
  }
  auto f3() const -> void override
  {
    f2();
  }
};

void func1() {}
void func2()
{
  func1();
}
void func3()
{
  func2();
}

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

    BENCHMARK("virtual function")
    {
      (*b)();
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

  SECTION("Multiple (3) indirections")
  {
    std::unique_ptr<C> d = std::make_unique<D>();

    std::function<void()> f{[] {}};
    std::function<void()> f2{[&f] { f(); }};
    std::function<void()> f3{[&f2] { f2(); }};

    beyond::unique_function<void()> u{[] {}};
    beyond::unique_function<void()> u2{[&u] { u(); }};
    beyond::unique_function<void()> u3{[&u2] { u2(); }};

    BENCHMARK("function pointer")
    {
      (+[]() { (+[]() { (+[]() {})(); })(); })();
    }

    BENCHMARK("virtual function")
    {
      d->f3();
    }

    BENCHMARK("std::function")
    {
      f3();
    }

    BENCHMARK("beyond::unique_function")
    {
      u3();
    }
  }
}
