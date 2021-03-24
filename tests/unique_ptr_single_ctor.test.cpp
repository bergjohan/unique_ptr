#include "unique_ptr.h"

struct D {
  void operator()(int *p) const
  {
    delete p;
  }
};

int main()
{
  // error: rvalue deleter object combined with reference deleter type
  Unique_ptr<int, const D &> p4(new int, D());
}
