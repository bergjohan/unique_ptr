#include <catch2/catch.hpp>

#include "unique_ptr.h"

struct S {
  S()
  {
    SUCCEED("S ctor");
  }

  S(const S &)
  {
    SUCCEED("S copy ctor");
  }

  S(S &&)
  {
    SUCCEED("S move ctor");
  }

  ~S()
  {
    SUCCEED("~S dtor");
  }

  int ret42()
  {
    return 42;
  }
};

struct D {
  D() = default;

  D(const D &)
  {
    SUCCEED("D copy ctor");
  }

  D(D &&)
  {
    SUCCEED("D move ctor");
  }

  void operator()(S *p) const
  {
    SUCCEED("D is deleting S");
    delete p;
  };
};

TEST_CASE("Default deleter"
          "[unique.ptr.dltr.dflt]")
{
  Default_delete<S> d3;
  d3(new S);
}

struct D1 {
  using pointer = void *;
};

TEST_CASE("General"
          "[unique.ptr.single.general]")
{
  REQUIRE(std::is_same_v<Unique_ptr<int, D1>::pointer, void *>);
  REQUIRE(std::is_same_v<Unique_ptr<int, D>::pointer, int *>);
  REQUIRE(std::is_same_v<Unique_ptr<int>::element_type, int>);
  REQUIRE(std::is_same_v<Unique_ptr<int, D>::deleter_type, D>);
}

TEST_CASE("Constructors 1"
          "[unique.ptr.single.ctor1]")
{
  Unique_ptr<S> up1;
  REQUIRE(up1.get() == nullptr);
  REQUIRE(std::is_same_v<decltype(up1.get_deleter()), Default_delete<S> &>);

  Unique_ptr<S> up2(nullptr);
  REQUIRE(up2.get() == nullptr);
  REQUIRE(std::is_same_v<decltype(up2.get_deleter()), Default_delete<S> &>);
}

TEST_CASE("Constructors 2"
          "[unique.ptr.single.ctor2]")
{
  S *p = new S;
  Unique_ptr<S> up(p);
  REQUIRE(up.get() == p);
  REQUIRE(std::is_same_v<decltype(up.get_deleter()), Default_delete<S> &>);
  // Empty base optimization
  REQUIRE(sizeof(up) == sizeof(p));
}

TEST_CASE("Constructors 3"
          "[unique.ptr.single.ctor3]")
{
  D d;

  S *p1 = new S;
  Unique_ptr<S, D> up1(p1, d); // D must be Cpp17CopyConstructible
  REQUIRE(up1.get() == p1);
  REQUIRE(std::is_same_v<decltype(up1.get_deleter()), D &>);

  S *p2 = new S;
  Unique_ptr<S, D &> up2(p2, d); // p2 holds a reference to d
  REQUIRE(up2.get() == p2);
  REQUIRE(std::is_same_v<decltype(up2.get_deleter()), D &>);
  REQUIRE(&up2.get_deleter() == &d);
}

TEST_CASE("Constructors 4"
          "[unique.ptr.single.ctor4]")
{
  S *p = new S;
  Unique_ptr<S, D> up(p, D()); // D must be Cpp17MoveConstructible
  REQUIRE(up.get() == p);
  REQUIRE(std::is_same_v<decltype(up.get_deleter()), D &>);
}

TEST_CASE("Constructors 5"
          "[unique.ptr.single.ctor5]")
{
  Unique_ptr<S> up1(new S);
  S *p1 = up1.get();
  Unique_ptr<S> up2(std::move(up1));
  REQUIRE(up2.get() == p1);
  REQUIRE(up1.get() == nullptr);
  REQUIRE(std::is_same_v<decltype(up2.get_deleter()), Default_delete<S> &>);

  D d;
  Unique_ptr<S, D &> up3(new S, d);
  S *p2 = up3.get();
  Unique_ptr<S, D &> up4(std::move(up3));
  REQUIRE(up4.get() == p2);
  REQUIRE(up3.get() == nullptr);
  REQUIRE(&up3.get_deleter() == &d);
  REQUIRE(&up4.get_deleter() == &d);
}

TEST_CASE("Constructors 6"
          "[unique.ptr.single.ctor6]")
{
  D d;
  Unique_ptr<S, D &> up1(new S, d);
  S *p = up1.get();
  Unique_ptr<S, D> up2(std::move(up1));
  REQUIRE(up2.get() == p);
  REQUIRE(up1.get() == nullptr);
  REQUIRE(std::is_same_v<decltype(up2.get_deleter()), D &>);

  // TODO: D is not a reference type and E is implicitly convertible to D.
}

TEST_CASE("Destructor"
          "[unique.ptr.single.dtor]")
{
  auto deleter = [](S *ptr) {
    SUCCEED("deleter called");
    delete ptr;
  };

  Unique_ptr<S, decltype(deleter)> up(new S, deleter);
}

TEST_CASE("Assignment"
          "[unique.ptr.single.asgn]")
{
  S *p = new S;
  Unique_ptr<S> up1(new S);
  Unique_ptr<S> up2(p);
  up1 = std::move(up2);
  REQUIRE(up2.get() == nullptr);
  REQUIRE(up1.get() == p);

  // TODO: operator=(unique_ptr<U, E>&& u)

  up1 = nullptr;
  REQUIRE(up1.get() == nullptr);
}

TEST_CASE("Observers"
          "[unique.ptr.single.observers]")
{
  S *p = new S;
  D d;
  Unique_ptr<S, D> up(p, d);
  REQUIRE(std::is_same_v<decltype(*up), S &>);
  REQUIRE(up->ret42() == 42);
  REQUIRE(up.get() == p);
  REQUIRE(std::is_same_v<decltype(up.get_deleter()), D &>);
  REQUIRE(up);
  up = nullptr;
  REQUIRE(!up);
}

TEST_CASE("Modifiers 1"
          "[unique.ptr.single.modifiers1]")
{
  S *p1 = new S;
  Unique_ptr<S> up1(p1);
  S *p2 = up1.release();
  REQUIRE(p1 == p2);
  REQUIRE(up1.get() == nullptr);
}

TEST_CASE("Modifiers 2"
          "[unique.ptr.single.modifiers2]")
{
  Unique_ptr<S> up(new S);
  S *p = new S;
  up.reset(p);
  REQUIRE(up.get() == p);
  up.reset();
  REQUIRE(up.get() == nullptr);
}

TEST_CASE("Modifiers 3"
          "[unique.ptr.single.modifiers3]")
{
  S *p1 = new S;
  Unique_ptr<S> up1(p1);
  S *p2 = new S;
  Unique_ptr<S> up2(p2);
  REQUIRE(up1.get() == p1);
  REQUIRE(up2.get() == p2);
  up1.swap(up2);
  REQUIRE(up1.get() == p2);
  REQUIRE(up2.get() == p1);
}

TEST_CASE("Creation"
          "[unique.ptr.create]")
{
  Unique_ptr<int> up = make_unique<int>(42);
  REQUIRE(*up == 42);
}

TEST_CASE("Specialized algorithms"
          "[unique.ptr.special]")
{
  Unique_ptr<int> up1(new int(42));
  Unique_ptr<int> up2(new int(42));
  REQUIRE(up1 == up1);
  REQUIRE(((up1 <=> up1) == 0));
  REQUIRE(up1 != up2);
}
