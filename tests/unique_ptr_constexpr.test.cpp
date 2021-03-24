#include "unique_ptr.h"

// Example from https://wg21.link/p2273r0

constexpr auto fun()
{
  auto p = make_unique<int>(4);

  return *p;
}

int main()
{
  constexpr auto i = fun();

  static_assert(4 == i);
}
