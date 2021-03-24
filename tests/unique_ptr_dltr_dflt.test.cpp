#include "unique_ptr.h"

// Test void type
int main()
{
  Default_delete<void> d;
  d(nullptr);
}
