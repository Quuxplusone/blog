---
layout: post
title: "Non-constexpr `Optional` and trivial relocation"
date: 2025-05-09 00:01:00 +0000
tags:
  proposal
  relocatability
  sum-types
  triviality
  wg21
excerpt: |
  Here's a simple non-constexpr-friendly `Optional` type. ([Godbolt.](https://godbolt.org/z/zavvzhKWY))

      template<class T>
      class [[trivially_relocatable(std::is_trivially_relocatable_v<T>)]] Optional {
      ~~~~
        alignas(T) char data_[sizeof(T)];
        bool engaged_ = false;
      };
---

Previously on this blog:

* ["Constexpr `Optional` and trivial relocation"](/blog/2025/05/08/siren-song-2-optional/) (2025-05-08)

---

Here's a simple non-constexpr-friendly `Optional` type. ([Godbolt.](https://godbolt.org/z/zavvzhKWY))

    template<class T>
    class [[trivially_relocatable(std::is_trivially_relocatable_v<T>)]] Optional {
    public:
      explicit Optional() {}

      template<class... Args>
      void emplace(Args&&... args) {
        if (engaged_) {
          std::destroy_at(t());
          engaged_ = false;
        }
        std::construct_at(t(), std::forward<Args>(args)...);
        engaged_ = true;
      }

      Optional(Optional&&)
        requires std::move_constructible<T> && std::is_trivially_move_constructible_v<T> = default;
      Optional(Optional&& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T>)
        requires std::move_constructible<T>
      {
        engaged_ = std::exchange(rhs.engaged_, false);
        if (engaged_) {
          std::relocate_at(rhs.t(), t());
        }
      }

      Optional(const Optional&)
        requires std::copy_constructible<T> && std::is_trivially_copy_constructible_v<T> = default;
      Optional(const Optional& rhs)
        noexcept(std::is_nothrow_copy_constructible_v<T>)
        requires std::copy_constructible<T>
      {
        engaged_ = rhs.engaged_;
        if (engaged_) {
          std::construct_at(t(), rhs.value());
        }
      }

      void swap(Optional& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_swappable_v<T>)
      {
        using std::swap;
        if (engaged_ && rhs.engaged_) {
          union U {
            char pad_;
            T t_;
            U() {}
            ~U() {}
          } temp;
          std::relocate_at(t(), &temp.t_);
          std::relocate_at(rhs.t(), t());
          std::relocate_at(&temp.t_, rhs.t());
        } else if (engaged_) {
          std::relocate_at(t(), rhs.t());
        } else if (rhs.engaged_) {
          std::relocate_at(rhs.t(), t());
        }
        swap(engaged_, rhs.engaged_);
      }

      Optional& operator=(Optional&&)
        requires std::movable<T> && std::is_trivially_copyable_v<T> = default;
      Optional& operator=(Optional&& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
        requires std::movable<T>
      {
        auto copy = std::move(rhs);
        copy.swap(*this);
        return *this;
      }

      Optional& operator=(const Optional&)
        requires std::copyable<T> && std::is_trivially_copyable_v<T> = default;
      Optional& operator=(const Optional& rhs)
        noexcept(std::is_nothrow_move_constructible_v<T> && std::is_nothrow_move_assignable_v<T>)
        requires std::copyable<T>
      {
        auto copy = rhs;
        copy.swap(*this);
        return *this;
      }

      ~Optional()
        requires std::is_trivially_destructible_v<T> = default;
      ~Optional() {
        if (engaged_) {
          std::destroy_at(t());
        }
      }

      T& value() { assert(engaged_); return *reinterpret_cast<T*>(data_); }
      const T& value() const { assert(engaged_); return *reinterpret_cast<const T*>(data_); }

    private:
      T *t() { return reinterpret_cast<T*>(data_); }

      alignas(T) char data_[sizeof(T)];
      bool engaged_ = false;
    };

As in [yesterday's post](/blog/2025/05/08/siren-song-2-optional/), this `Optional`
differs from `std::optional` in doing copy-and-swap instead of delegating to `T::operator=`;
this makes it what P2786 calls "replaceable."
This behavioral difference is easily observable ([Godbolt](https://godbolt.org/z/4Y1zdnv1E)),
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
        Optional {
      ~~~~

or by name:

    template<class T>
    inline constexpr bool optional_be_trivially_relocatable =
      std::is_trivially_relocatable_v<T>;

    template<class... Ts>
    inline constexpr bool optional_be_trivially_relocatable<std::tuple<Ts...>> =
      ((std::is_reference_v<Ts> || std::is_trivially_relocatable_v<Ts>) && ...);

    template<class T>
    class [[trivially_relocatable(optional_be_trivially_relocatable<T>)]] Optional {
      ~~~~

Either way, we accomplish our goal: we make the compiler to understand that
`Optional` achieves greater regularity than the `tuple` it contains.

    static_assert(!std::is_trivially_relocatable_v<std::tuple<int&>>);
    static_assert(std::is_trivially_relocatable_v<Optional<std::tuple<int&>>>);

## C++26 will make this a little more dangerous

Unfortunately for C++ users, there are two competing models of "trivial relocation."
There's the "P1144" model everyone uses in practice, and then there's the "P2786" model
that was voted into C++26 in Hagenberg in February despite
[loud technical objections from the userbase](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3236r1.html)
about problems which [remain unaddressed](https://quuxplusone.github.io/draft/d1144-object-relocation.html#intro).

One of the deficiencies of P2786 is that it asks us to juggle two traits (each with an associated
keyword), neither of which expresses _quite_ the property we care about in practice. In yesterday's
post, we migrated from P1144's explicitly conditional ("sharp-knife") syntax to P2786's _implicitly_
conditional ("dull-knife") syntax by changing our class-head from:

    template<class T>
    class [[trivially_relocatable(std::is_trivially_relocatable<T>)]] Optional {

to:

    template<class T>
    class Optional trivially_relocatable_if_eligible
                   replaceable_if_eligible {

Yesterday, our `Optional` contained a data member of type `T`, so the implicit condition on
`trivially_relocatable_if_eligible` successfully prevented the compiler from seeing `Optional<T>`
as trivially relocatable when in fact `T` was not trivially relocatable.

Today's `Optional`, though, doesn't contain a `T` member visible to the compiler — it contains
only an aligned array of bytes. So the "dull knife" will slip, and cut where we didn't intend.

    template<class T>
    class Optional trivially_relocatable_if_eligible
                   replaceable_if_eligible {
          // BUG: This Optional is invariably "eligible"!
      ~~~~
      alignas(T) char data_[sizeof(T)];
      bool engaged_ = false;
    };
    static_assert(std::is_trivially_relocatable_v<Optional<int>>); // OK, good
    static_assert(std::is_trivially_relocatable_v<Optional<std::set<int>>>); // Buggy, bad

## This solves one problem and creates another

Yesterday I wrote:

> Tomorrow we'll see how we can gain "replaceability" for `Optional`, even under P2786,
> by sacrificing our `constexpr` support.

Indeed that's what we've done: `Optional<T>` is now _invariably_ P2786-replaceable,
which is semantically correct. Problem solved! But we have created a bigger problem
(besides losing constexpr-friendliness): `Optional<T>` is now also _invariably_
P2786-trivially-relocatable, when we need it to be only conditionally trivially relocatable.
The version of P2786 adopted for C++26 lacks any way to provide an explicitly conditional
warrant.

> Permitting optional parentheses after the keyword would have produced
> ambiguity in the grammar. P1144's attribute, on the other hand, added no new grammar
> and thus no potential for ambiguity.

There are two ways to make our `Optional` safe again. (By "safe," I mean "make sure that
no downstream users, such as `vector::erase`, _think_ it's trivially relocatable and end
up memcpy'ing it when that would have the wrong physical behavior.")

First: We could simply stop using the `trivially_relocatable_if_eligible` keyword. If you
don't use the keyword, then sure, `std::is_trivially_relocatable_v<Optional<std::vector<int>>>`
will become `false`; but at least `std::is_trivially_relocatable_v<Optional<std::set<int>>>`
will *also* become `false`, which is our safety goal here. This is the "sink the ship to
kill the captain" approach.

Second: We could use template metaprogramming. This is the approach recommended
[by P2786R13](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2786r13.html#optimizing-stdoptional-to-be-trivially-relocatable-and-replaceable).
This isn't completely crazy; after all, this was the way we did
["conditionally trivial member functions"](https://devblogs.microsoft.com/cppblog/conditionally-trivial-special-member-functions/)
pre-C++20, so STL implementors are generally familiar with the technique. (Mind you, third-party library
authors might not be familiar with it; and even STL vendors might not be _happy_ to be forced
back into the desert after six years in the promised land; but...) Here's how it works:

    template<bool=true>
    struct ConditionallyTriviallyRelocatable {};

    template<>
    struct ConditionallyTriviallyRelocatable<false> {
      // deliberately not =default; do not edit
      constexpr ~ConditionallyTriviallyRelocatable() {}
    };

    template<class T>
    class Optional trivially_relocatable_if_eligible
                   replaceable_if_eligible {
      ~~~~
    private:
      T *t() { return reinterpret_cast<T*>(data_); }

      [[no_unique_address]] ConditionallyTriviallyRelocatable<std::is_trivially_relocatable_v<T>> dummy_;
      alignas(T) char data_[sizeof(T)];
      bool engaged_ = false;
    };
    static_assert(std::is_trivially_relocatable_v<Optional<int>>); // OK, good
    static_assert(!std::is_trivially_relocatable_v<Optional<std::set<int>>>); // OK, fixed!

Now `Optional<T>` has a data member of type `ConditionallyTriviallyRelocatable`
which is trivially relocatable if-and-only-if `T` is trivially relocatable. This gives us
the conditionality we want. Because the member is `[[no_unique_address]]`, it doesn't
affect the physical layout of our `Optional`.

This technique works. But it does have its downsides. Two are strictly aesthetic: (1) It's ugly.
(2) It's fragile under maintenance: see that code comment? If a maintainer ignores it
and refactors `{}` into `=default`, you'll suddenly have a bug downstream. So the
comment is load-bearing.

One downside is technical, albeit very minor: (3) This makes `Optional<T>` for certain `T`
not just non-trivially relocatable but also non-trivially destructible, even in cases where
`T` is trivially destructible. (An example of a type that is non-trivially relocatable
but trivially destructible is [`boost::interprocess::offset_ptr`](/blog/2024/08/23/boost-interprocess-tutorial/).)
It would be nice to find a template-metaprogramming solution that would _simply_ disable
trivial relocation for certain `Optional<T>`, without also affecting its ABI in other ways.

> P1144's opt-in attribute, besides being easily conditionalized, deliberately
> doesn't affect the type's ABI at all. This makes it easy to drop in to existing
> codebases without worrying about unintended side effects; and as a bonus, it
> will be more-or-less quietly ignored on compilers that don't support it, instead
> of being a hard syntax error like `trivially_relocatable_if_eligible`.

## Now that you've read this far...

I intend to bring a revision of P1144 in the May mailing with (for the first time)
a number of coauthors, including some of the signatories of P3236. Since P2786 has
been merged into the Working Draft, this new version of P1144 will (for the first time)
be expressed as
[a diff against](https://quuxplusone.github.io/draft/d1144-object-relocation.html#scope)
what is now P2786-in-the-Working-Draft, showing exactly what parts of the C++26 wording
should change in order to make what remains both correct and performant.

If you'd like to be listed among the supporters of P1144R13, please
[send me an email](mailto:arthur.j.odwyer@gmail.com).

---

See also:

* ["Type-erasure, trivial relocation, and lethal sirens"](/blog/2025/05/01/siren-song-of-p2786-keywords/) (2025-05-01)
* ["Constexpr `Optional` and trivial relocation"](/blog/2025/05/08/siren-song-2-optional/) (2025-05-08)
