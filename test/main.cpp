#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this
                          // in one cpp file
#include <catch2/catch.hpp>

#include <type_traits>

#include "unique_function.hpp"

TEST_CASE("unique_ptr does not support copies")
{
  STATIC_REQUIRE(
      !std::is_copy_constructible_v<beyond::unique_function<void()>>);
  STATIC_REQUIRE(!std::is_copy_assignable_v<beyond::unique_function<void()>>);
}

TEST_CASE("Default constructor")
{
  beyond::unique_function<void()> f;
  REQUIRE(!f);

  SECTION("Invoking empty unique_function throws std::bad_function_call")
  {
    REQUIRE_THROWS_AS(f(), std::bad_function_call);
  }
}

TEST_CASE("unique_function with a capturless lambda")
{
  beyond::unique_function<int()> f{[]() { return 1; }};
  REQUIRE(f);
  REQUIRE(f() == 1);
}

TEST_CASE("unique_function with a captured lambda")
{
  int x = 1;
  beyond::unique_function<int()> f{[x]() { return x; }};
  beyond::unique_function<int()> f2{[&x]() { return x; }};

  REQUIRE(f);
  REQUIRE(f() == 1);

  x = 2;
  REQUIRE(f() == 1);
  REQUIRE(f2() == 2);
}

TEST_CASE("unique_function can move")
{
  const int x = 1;
  beyond::unique_function<int()> f{[&]() { return x; }};
  auto f2 = std::move(f);
  CHECK(!f);
  REQUIRE(f2);
  REQUIRE(f2() == x);
}

struct Small {
  Small(int& counter) : counter_ptr{&counter} {}
  ~Small()
  {
    ++(*counter_ptr);
  }

  void operator()()
  {
    ++(*counter_ptr);
  }

  int* counter_ptr;
};

struct Large {
  Large(int& counter) : counter_ptr{&counter} {}
  ~Large()
  {
    ++(*counter_ptr);
  }

  void operator()()
  {
    ++(*counter_ptr);
  }

  int* counter_ptr;
  char x[128];
};

TEST_CASE("unique_function clean-up")
{
  int x = 1;
  {
    beyond::unique_function<void()> f{Small{x}};
    f();
  }
  REQUIRE(x == 3);

  {
    beyond::unique_function<void()> f{Large{x}};
    f();
  }
  REQUIRE(x == 5);
}
