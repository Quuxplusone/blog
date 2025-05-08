---
layout: post
title: "Constexpr `Optional` and trivial relocation"
date: 2025-05-08 00:01:00 +0000
tags:
  proposal
  relocatability
  triviality
  wg21
---

Here's a simple C++20 constexpr-friendly `Optional` type. ([Godbolt.](https://godbolt.org/z/5T6nzY7dh))

    template<class T>
    class [[trivially_relocatable(std::is_trivially_relocatable_v<T>)]] Optional {
    public:
      constexpr explicit Optional() {}

      template<class... Args>
      constexpr void emplace(Args&&... args) {
        if (engaged_) {
          std::destroy_at(&t_);
          engaged_ = false;
        }
        std::construct_at(&t_, std::forward<Args>(args)...);
        engaged_ = true;
      }

      Optional(Optional&&)
        requires std::move_constructible<T> && std::is_trivially_move_constructible_v<T> = default;
      constexpr Optional(Optional&& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T>)
        requires std::move_constructible<T>
      {
        engaged_ = std::exchange(rhs.engaged_, false);
        if (engaged_) {
          std::relocate_at(&rhs.t_, &t_);
        }
      }

      Optional(const Optional&)
        requires std::copy_constructible<T> && std::is_trivially_copy_constructible_v<T> = default;
      constexpr Optional(const Optional& rhs)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
        requires std::copy_constructible<T>
      {
        engaged_ = rhs.engaged_;
        if (engaged_) {
          std::construct_at(&t_, rhs.t_);
        }
      }

      constexpr void swap(Optional& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>)
      {
        using std::swap;
        if (engaged_ && rhs.engaged_) {
          union U {
            char pad_;
            T t_;
            constexpr U() {}
            constexpr ~U() {}
          } temp;
          std::relocate_at(&t_, &temp.t_);
          std::relocate_at(&rhs.t_, &t_);
          std::relocate_at(&temp.t_, &rhs.t_);
        } else if (engaged_) {
          std::relocate_at(&t_, &rhs.t_);
        } else if (rhs.engaged_) {
          std::relocate_at(&rhs.t_, &t_);
        }
        swap(engaged_, rhs.engaged_);
      }

      Optional& operator=(Optional&&)
        requires std::movable<T> && std::is_trivially_copyable_v<T> = default;
      constexpr Optional& operator=(Optional&& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
        requires std::movable<T>
      {
        auto copy = std::move(rhs);
        copy.swap(*this);
        return *this;
      }

      Optional& operator=(const Optional&)
        requires std::copyable<T> && std::is_trivially_copyable_v<T> = default;
      constexpr Optional& operator=(const Optional& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
        requires std::copyable<T>
      {
        auto copy = rhs;
        copy.swap(*this);
        return *this;
      }

      ~Optional()
        requires std::is_trivially_destructible_v<T> = default;
      constexpr ~Optional() {
        if (engaged_) {
          std::destroy_at(&t_);
        }
      }
    private:
      union {
        char pad_;
        T t_;
      };
      bool engaged_ = false;
    };

This `Optional` behaves basically like
[`std::optional`](https://en.cppreference.com/w/cpp/utility/optional),
with one important semantic difference: This `Optional` is what P2786 calls "replaceable."
That simply means that instead of `std::optional`’s case-wise assignment operator:

      optional& operator=(const optional& rhs) {
        if (engaged_ && rhs.engaged_) {
          t_ = rhs.t_;
        } else if (engaged_) {
          std::destroy_at(&t_);
          engaged_ = false;
        } else if (rhs.engaged_) {
          std::construct_at(&t_, rhs.t_);
          engaged_ = true;
        }
        return *this;
      }

ours does copy-and-swap, and swaps via relocation rather than via assignment.
Thus we never call `T::operator=`, which means we don't care
if `T`'s assignment operator is wonky (for example, if `T` is `std::tuple<int&>`).
This behavioral difference is easily observable ([Godbolt](https://godbolt.org/z/df3b43asf)),
but defensible in a third-party `Optional` type.

As written above, it is invariably true
that `is_trivially_relocatable_v<T> == is_trivially_relocatable_v<Optional<T>>`.
This invariant is not only nice to have, but should be intuitively obvious:
If you can shuffle around objects of type `T` by copying their bytes, then you must
be able to shuffle around objects of type `Optional<T>` the same way, because
`Optional<T>`’s memory footprint contains nothing but a `T` and a `bool`.

What about types with "weird" assignment operators, like `tuple<int&>`? Under P1144,
`is_trivially_relocatable_v<tuple<int&>>` == `false`. However, we could safely
warrant `Optional<tuple<int&>>` as trivially relocatable, since it never uses `tuple`'s
wonky assignment or swap.
P1144's "sharp knife" lets us cut out an exception for `tuple<int&>` if we want to,
either generically:

    template<class T>
    class [[trivially_relocatable(std::is_trivially_relocatable_v<T> ||
        (std::is_trivially_move_constructible_v<T> && std::is_trivially_destructible_v<T>))]]
        optional {
      ~~~~

or by name:

    template<class T>
    inline constexpr bool optional_be_trivially_relocatable =
      std::is_trivially_relocatable_v<T>;

    template<class... Ts>
    inline constexpr bool optional_be_trivially_relocatable<std::tuple<Ts...>> =
      ((std::is_reference_v<Ts> || std::is_trivially_relocatable_v<Ts>) && ...);

    template<class T>
    class [[trivially_relocatable(optional_be_trivially_relocatable<T>)]] optional {
      ~~~~

Either way, we accomplish our goal: we make the compiler to understand that
`Optional` achieves greater regularity than the `tuple` it contains.

    static_assert(!std::is_trivially_relocatable_v<std::tuple<int&>>);
    static_assert(std::is_trivially_relocatable_v<Optional<std::tuple<int&>>>);

## C++26 will make this a little weirder and a little harder

Unfortunately for C++ users, there are two competing models of "trivial relocation."
There's the "P1144" model everyone uses in practice, and then there's the "P2786" model
that was voted into C++26 in Hagenberg in February despite
[loud technical objections from the userbase](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3236r1.html)
about problems which [remain unaddressed](https://quuxplusone.github.io/draft/d1144-object-relocation.html#intro).

Here's the same `Optional` written in P2786's syntax: [Godbolt.](https://godbolt.org/z/W7M83ceTr)
It's almost exactly the same; the only _syntactic_ difference is in the class-head,
where instead of an attribute with an explicit condition, P2786 asks us to use
a pair of keywords:

    template<class T>
    class optional trivially_relocatable_if_eligible
                   replaceable_if_eligible {
        ~~~~

P2786 (the rising C++26) requires us to juggle *two* traits, and each of them
has a little hiccup when we look closely.

First: Under P2786 it is _not_ true that `Optional<T>` is trivially relocatable if-and-only-if `T` is
trivially relocatable. That's because P2786 defines "trivially relocatable" more broadly, as
a type-specific operation not necessarily tantamount to memcpy. P2786
says that polymorphic types are trivially relocatable, but unions of polymorphic types
aren't necessarily trivially relocatable. Thus:

    struct Poly { virtual int f(); };
    struct Holder { int i; Poly p; };
    static_assert(std::is_trivially_relocatable_v<Holder>); // P2786 makes this true
    IMPL_DEFINED(std::is_trivially_relocatable_v<Optional<Holder>>); // may be true or false

Second: We remarked above that our `Optional` is invariably what P2786 calls "replaceable."
In C++26 we can mark it as `replaceable_if_eligible`; but that keyword has ["dull knife" semantics](/blog/2023/03/10/sharp-knife-dull-knife/):
it doesn't always cut straight. The compiler will look at the type inside the union —
say, `tuple<int&>` — and if it discovers that that type is not known to be replaceable, then
the `Optional` class won't be considered replaceable either. We have no path to accomplish our goal:
the C++26 compiler _cannot_ be made to understand that `Optional` achieves any greater regularity
than its contained `tuple` type.

    static_assert(!std::is_replaceable_v<std::tuple<int&>>);
    static_assert(!std::is_replaceable_v<Optional<std::tuple<int&>>>); // P2786 forces this
      // to false, even though conceptually it should be true

We can turn *off* either of these traits (and we'll see an example tomorrow where that's required
for correctness), but we can't turn them *on* if the compiler decides suboptimally.
This means that our `Optional<tuple<int&>>` cannot with P2786 get the optimizations
for `vector::erase`, `rotate`, and the like, which it can get with P1144.

Tomorrow we'll see how we can gain "replaceability" for `Optional`, even under P2786,
by sacrificing our `constexpr` support.

----

See also:

* ["Type-erasure, trivial relocation, and lethal sirens"](/blog/2025/05/01/siren-song-of-p2786-keywords/) (2025-05-01)
