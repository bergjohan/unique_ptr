#include "unique_ptr.h"

class X;

// Test incomplete type
int main()
{
  Default_delete<X> d;
  d(nullptr);
}
