#pragma once

#include <utility>

// Comments from https://eel.is/c++draft/unique.ptr

namespace detail
{

// Simplified equivalent of boost::compressed_pair for empty base optimization
template <typename T, typename D>
class Compressed_pair
{
public:
  constexpr Compressed_pair() = default;

  // Pass by value since T is a pointer
  constexpr explicit Compressed_pair(T first) : first_(first) {}

  template <typename U>
  constexpr Compressed_pair(T first, U &&second)
      : first_(first), second_(std::forward<U>(second))
  {
  }

  constexpr T &first()
  {
    return first_;
  }

  constexpr const T &first() const
  {
    return first_;
  }

  constexpr D &second()
  {
    return second_;
  }

  constexpr D &second() const
  {
    return second_;
  }

private:
  T first_{};
  D second_{};
};

template <typename T, typename D>
requires(std::is_empty_v<D> && !std::is_final_v<D>) class Compressed_pair<T, D>
    : private D
{
public:
  constexpr Compressed_pair() = default;

  // Pass by value since T is a pointer
  constexpr explicit Compressed_pair(T first) : first_(first) {}

  template <typename U>
  constexpr Compressed_pair(T first, U &&second)
      : D(std::forward<U>(second)), first_(first)
  {
  }

  constexpr T &first()
  {
    return first_;
  }

  constexpr const T &first() const
  {
    return first_;
  }

  constexpr D &second()
  {
    return *this;
  }

  constexpr D &second() const
  {
    return *this;
  }

private:
  T first_{};
};

template <typename T, typename D>
struct Pointer {
  using type = T *;
};

template <typename T, typename D>
requires requires
{
  typename std::remove_reference_t<D>::pointer;
}
struct Pointer<T, D> {
  using type = typename std::remove_reference_t<D>::pointer;
};

} // namespace detail

// The class template Default_delete serves as the default deleter (destruction policy) for the class template Unique_ptr.
template <typename T>
struct Default_delete {
  constexpr Default_delete() noexcept = default;

  // Constraints: U* is implicitly convertible to T*.
  // Effects: Constructs a Default_delete object from another Default_delete<U> object.
  template <class U>
  constexpr Default_delete(const Default_delete<U> &other) noexcept
      requires std::is_convertible_v<U *, T *>
  {
  }

  // Mandates: T is a complete type.
  // Effects: Calls delete on ptr.
  constexpr void operator()(T *ptr) const
  {
    // Decided to use static_assert for "Mandates", see https://eel.is/c++draft/structure.specifications
    static_assert(!std::is_void_v<T>,
                  "Function call operator requires complete type");
    static_assert(sizeof(T) > 0,
                  "Function call operator requires complete type");
    delete ptr;
  }
};

// Unique_ptr for single objects
template <typename T, typename D = Default_delete<T>>
class Unique_ptr
{
public:
  // If the qualified-id remove_reference_t<D>::pointer is valid and denotes a type ([temp.deduct]), then Unique_ptr<T, D>::pointer shall be a synonym for remove_reference_t<D>::pointer. Otherwise Unique_ptr<T, D>::pointer shall be a synonym for element_type*. The type Unique_ptr<T, D>::pointer shall meet the Cpp17NullablePointer requirements (Table 35).
  using pointer = typename detail::Pointer<T, D>::type;
  using element_type = T;
  using deleter_type = D;

  //
  // Constructors
  //

  // Constraints: is_pointer_v<deleter_type> is false and is_default_constructible_v<deleter_type> is true.
  // Preconditions: D meets the Cpp17DefaultConstructible requirements (Table 29), and that construction does not throw an exception.
  // Effects: Constructs a Unique_ptr object that owns nothing, value-initializing the stored pointer and the stored deleter.
  // Postconditions: get() == nullptr. get_deleter() returns a reference to the stored deleter.
  constexpr Unique_ptr() noexcept
      requires(!std::is_pointer_v<deleter_type> &&
               std::is_default_constructible_v<deleter_type>) = default;

  constexpr Unique_ptr(std::nullptr_t) noexcept
      requires(!std::is_pointer_v<deleter_type> &&
               std::is_default_constructible_v<deleter_type>)
  {
  }

  // Constraints: is_pointer_v<deleter_type> is false and is_default_constructible_v<deleter_type> is true.
  // Mandates: This constructor is not selected by class template argument deduction ([over.match.class.deduct]).
  // Preconditions: D meets the Cpp17DefaultConstructible requirements (Table 29), and that construction does not throw an exception.
  // Effects: Constructs a Unique_ptr which owns p, initializing the stored pointer with p and value-initializing the stored deleter.
  // Postconditions: get() == p. get_deleter() returns a reference to the stored deleter.
  constexpr explicit Unique_ptr(pointer p) noexcept
      requires(!std::is_pointer_v<deleter_type> &&
               std::is_default_constructible_v<deleter_type>)
      : pair_(p)
  {
  }

  // Constraints: is_constructible_v<D, decltype(d)> is true.
  // Mandates: These constructors are not selected by class template argument deduction ([over.match.class.deduct]).
  // Preconditions: For the first constructor, if D is not a reference type, D meets the Cpp17CopyConstructible requirements and such construction does not exit via an exception. For the second constructor, if D is not a reference type, D meets the Cpp17MoveConstructible requirements and such construction does not exit via an exception.
  // Effects: Constructs a Unique_ptr object which owns p, initializing the stored pointer with p and initializing the deleter from std::forward<decltype(d)>(d).
  // Postconditions: get() == p. get_deleter() returns a reference to the stored deleter. If D is a reference type then get_deleter() returns a reference to the lvalue d.
  // Remarks: If D is a reference type, the second constructor is defined as deleted.
  constexpr Unique_ptr(pointer p, const D &d) noexcept
      requires std::is_constructible_v<D, decltype(d)> : pair_(p, d)
  {
  }

  constexpr Unique_ptr(pointer p, std::remove_reference_t<D> &&d) noexcept
      requires(std::is_constructible_v<D, decltype(d)> &&
               !std::is_reference_v<D>)
      : pair_(p, std::move(d))
  {
  }

  // clang-format off
  constexpr Unique_ptr(pointer p, std::remove_reference_t<D> &&d) noexcept
      requires std::is_constructible_v<D, decltype(d)> &&
               std::is_reference_v<D> = delete;
  // clang-format on

  // Constraints: is_move_constructible_v<D> is true.
  // Preconditions: If D is not a reference type, D meets the Cpp17MoveConstructible requirements (Table 30). Construction of the deleter from an rvalue of type D does not throw an exception.
  // Effects: Constructs a Unique_ptr from u. If D is a reference type, this deleter is copy constructed from u's deleter; otherwise, this deleter is move constructed from u's deleter.
  // Postconditions: get() yields the value u.get() yielded before the construction. u.get() == nullptr. get_deleter() returns a reference to the stored deleter that was constructed from u.get_deleter(). If D is a reference type then get_deleter() and u.get_deleter() both reference the same lvalue deleter.
  constexpr Unique_ptr(Unique_ptr &&u) noexcept
      requires std::is_move_constructible_v<D>
      : pair_(u.release(), std::forward<D>(u.get_deleter()))
  {
  }

  // Constraints:
  //  - Unique_ptr<U, E>::pointer is implicitly convertible to pointer,
  //  - U is not an array type, and
  //  - either D is a reference type and E is the same type as D, or D is not a reference type and E is implicitly convertible to D.
  // Preconditions: If E is not a reference type, construction of the deleter from an rvalue of type E is well-formed and does not throw an exception. Otherwise, E is a reference type and construction of the deleter from an lvalue of type E is well-formed and does not throw an exception.
  // Effects: Constructs a Unique_ptr from u. If E is a reference type, this deleter is copy constructed from u's deleter; otherwise, this deleter is move constructed from u's deleter.
  // Postconditions: get() yields the value u.get() yielded before the construction. u.get() == nullptr. get_deleter() returns a reference to the stored deleter that was constructed from u.get_deleter().
  template <typename U, typename E>
  constexpr Unique_ptr(Unique_ptr<U, E> &&u) noexcept requires(
      std::is_convertible_v<typename Unique_ptr<U, E>::pointer, pointer> &&
      !std::is_array_v<U> &&
      ((std::is_reference_v<D> && std::is_same_v<E, D>) ||
       (!std::is_reference_v<D> && std::is_convertible_v<E, D>)))
      : pair_(u.release(), std::forward<D>(u.get_deleter()))
  {
  }

  //
  // Destructor
  //

  // Preconditions: The expression get_deleter()(get()) is well-formed, has well-defined behavior, and does not throw exceptions.
  // Effects: If get() == nullptr there are no effects. Otherwise get_deleter()(get()).
  constexpr ~Unique_ptr()
  {
    if (get() != nullptr) {
      get_deleter()(get());
    }
  }

  //
  // Assignment
  //

  // Constraints: is_move_assignable_v<D> is true.
  // Preconditions: If D is not a reference type, D meets the Cpp17MoveAssignable requirements (Table 32) and assignment of the deleter from an rvalue of type D does not throw an exception. Otherwise, D is a reference type; remove_reference_t<D> meets the Cpp17CopyAssignable requirements and assignment of the deleter from an lvalue of type D does not throw an exception.
  // Effects: Calls reset(u.release()) followed by get_deleter() = std::forward<D>(u.get_deleter()).
  // Postconditions: If this != addressof(u), u.get() == nullptr, otherwise u.get() is unchanged.
  // Returns: *this.
  constexpr Unique_ptr &operator=(Unique_ptr &&u) noexcept
      requires std::is_move_assignable_v<D>
  {
    reset(u.release());
    get_deleter() = std::forward<D>(u.get_deleter());
    return *this;
  }

  // Constraints:
  //  - Unique_ptr<U, E>::pointer is implicitly convertible to pointer, and
  //  - U is not an array type, and
  //  - is_assignable_v<D&, E&&> is true.
  // Preconditions: If E is not a reference type, assignment of the deleter from an rvalue of type E is well-formed and does not throw an exception. Otherwise, E is a reference type and assignment of the deleter from an lvalue of type E is well-formed and does not throw an exception.
  // Effects: Calls reset(u.release()) followed by get_deleter() = std::forward<E>(u.get_deleter()).
  // Postconditions: u.get() == nullptr.
  // Returns: *this.
  template <class U, class E>
  constexpr Unique_ptr &operator=(Unique_ptr<U, E> &&u) noexcept requires(
      std::is_convertible_v<typename Unique_ptr<U, E>::pointer, pointer> &&
      !std::is_array_v<U> && std::is_assignable_v<D &, E &&>)
  {
    reset(u.release());
    get_deleter() = std::forward<E>(u.get_deleter());
    return *this;
  }

  // Effects: As if by reset().
  // Postconditions: get() == nullptr.
  // Returns: *this.
  constexpr Unique_ptr &operator=(std::nullptr_t) noexcept
  {
    reset();
    return *this;
  }

  //
  // Observers
  //

  // Preconditions: get() != nullptr.
  // Returns: *get().
  constexpr std::add_lvalue_reference_t<T> operator*() const
  {
    return *get();
  }

  // Preconditions: get() != nullptr.
  // Returns: get().
  constexpr pointer operator->() const noexcept
  {
    return get();
  }

  // Returns: The stored pointer.
  constexpr pointer get() const noexcept
  {
    return pair_.first();
  }

  // Returns: A reference to the stored deleter.
  constexpr deleter_type &get_deleter() noexcept
  {
    return pair_.second();
  }

  constexpr deleter_type &get_deleter() const noexcept
  {
    return pair_.second();
  }

  // Returns: get() != nullptr.
  constexpr explicit operator bool() const noexcept
  {
    return get() != nullptr;
  }

  //
  // Modifiers
  //

  // Postconditions: get() == nullptr.
  // Returns: The value get() had at the start of the call to release.
  constexpr pointer release() noexcept
  {
    pointer ret = pair_.first();
    pair_.first() = nullptr;
    return ret;
  }

  // Preconditions: The expression get_deleter()(get()) is well-formed, has well-defined behavior, and does not throw exceptions.
  // Effects: Assigns p to the stored pointer, and then if and only if the old value of the stored pointer, old_p, was not equal to nullptr, calls get_deleter()(old_p).
  // Postconditions: get() == p.
  constexpr void reset(pointer p = pointer()) noexcept
  {
    pointer old_p = pair_.first();
    pair_.first() = p;
    if (old_p != nullptr) {
      get_deleter()(old_p);
    }
  }

  // Preconditions: get_deleter() is swappable ([swappable.requirements]) and does not throw an exception under swap.
  // Effects: Invokes swap on the stored pointers and on the stored deleters of *this and u.
  constexpr void swap(Unique_ptr &u) noexcept
  {
    std::swap(pair_.first(), u.pair_.first());
    std::swap(pair_.second(), u.pair_.second());
  }

  // disable copy from lvalue
  Unique_ptr(const Unique_ptr &) = delete;
  Unique_ptr &operator=(const Unique_ptr &) = delete;

private:
  detail::Compressed_pair<pointer, deleter_type> pair_{};
};

//
// Creation
//

// Constraints: T is not an array type.
// Returns: Unique_ptr<T>(new T(std::forward<Args>(args)...)).
template <class T, class... Args>
constexpr Unique_ptr<T>
make_unique(Args &&...args) requires(!std::is_array_v<T>)
{
  return Unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// Constraints: T is not an array type.
// Returns: Unique_ptr<T>(new T).
template <class T>
constexpr Unique_ptr<T>
make_unique_for_overwrite() requires(!std::is_array_v<T>)
{
  return Unique_ptr<T>(new T);
}

//
// Specialized algorithms
//

// Constraints: is_swappable_v<D> is true.
// Effects: Calls x.swap(y).
template <class T, class D>
constexpr void swap(Unique_ptr<T, D> &x, Unique_ptr<T, D> &y) noexcept
    requires std::is_swappable_v<D>
{
  x.swap(y);
}

// Returns: x.get() == y.get().
template <class T1, class D1, class T2, class D2>
constexpr bool operator==(const Unique_ptr<T1, D1> &x,
                          const Unique_ptr<T2, D2> &y)
{
  return x.get() == y.get();
}

// Returns: compare_three_way()(x.get(), y.get()).
template <class T1, class D1, class T2, class D2>
requires std::three_way_comparable_with<
    typename Unique_ptr<T1, D1>::pointer,
    typename Unique_ptr<T2, D2>::pointer> constexpr std::
    compare_three_way_result_t<typename Unique_ptr<T1, D1>::pointer,
                               typename Unique_ptr<T2, D2>::pointer>
    operator<=>(const Unique_ptr<T1, D1> &x, const Unique_ptr<T2, D2> &y)
{
  return std::compare_three_way()(x.get(), y.get());
}

// Returns: !x.
template <class T, class D>
constexpr bool operator==(const Unique_ptr<T, D> &x, std::nullptr_t) noexcept
{
  return !x;
}

// Returns: compare_three_way()(x.get(), static_cast<typename Unique_ptr<T, D>::pointer>(nullptr)).
template <class T, class D>
requires std::three_way_comparable<
    typename Unique_ptr<T, D>::pointer> constexpr std::
    compare_three_way_result_t<typename Unique_ptr<T, D>::pointer>
    operator<=>(const Unique_ptr<T, D> &x, std::nullptr_t)
{
  return std::compare_three_way()(
      x.get(), static_cast<typename Unique_ptr<T, D>::pointer>(nullptr));
}
