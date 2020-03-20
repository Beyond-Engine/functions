#include <iostream>

#include <beyond/unique_function.hpp>

int main()
{
  beyond::unique_function<void()> f{[] { std::cout << 42 << '\n'; }};
}
