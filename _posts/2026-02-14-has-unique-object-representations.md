---
layout: post
title: "`has_unique_object_representations` versus `__is_trivially_equality_comparable`"
date: 2026-02-14 00:01:00 +0000
tags:
  cpplang-slack
  operator-spaceship
  triviality
  type-traits
  value-semantics
---

Yesterday on the [cpplang Slack](https://cppalliance.org/slack/),
someone asked the purpose of `std::has_unique_object_representations_v<T>`,
and someone else pointed to an example given in
Steve Dewhurst's ["Back to Basics: Class Layout"](https://www.youtube.com/watch?v=SShSV_iV1Ko) (2020).
Sadly, that example is incorrect. Let's take a look.

Steve presents this code snippet ([@28m58s](https://www.youtube.com/watch?v=SShSV_iV1Ko&t=1738s)):

    class NarrowLand {
      unsigned long long x_;
      unsigned long long y_;
      unsigned long long z_;
      friend bool operator==(const NarrowLand& a, const NarrowLand& b) {
        return memcmp(&a, &b, sizeof(a)) == 0;
      }
    };

The programmer knows this implementation of `operator==` is "safe" because `unsigned long long` itself
is trivially equality-comparable and because `NarrowLand` contains no padding bits in between
its `unsigned long long` fields, nor at the end of the class. (It isn't allowed to contain
padding at the _start_ of the class, because it is a [standard-layout class type](https://eel.is/c++draft/class.prop#def:class,standard-layout).)
Therefore comparing two `NarrowLand` objects is tantamount to comparing their bytes —
their _object representations_ — and can be done with a simple memcmp.

But you should never write that memcmp yourself! Since C++20, you should simply _default_
your comparison operator, like this:

    class BetterLand {
      unsigned long long x_;
      unsigned long long y_;
      unsigned long long z_;
      friend bool operator==(const BetterLand&, const BetterLand&) = default;
    };

On Clang, `BetterLand`’s `operator==` produces exactly the same codegen as `NarrowLand`’s;
on GCC, `BetterLand`’s codegen is better. ([Godbolt.](https://godbolt.org/z/PEbE3KMd7))

But the real reason to prefer `BetterLand` — to let the compiler default the comparison for you,
instead of user-defining it — is that defaulted operations are more legible to the compiler.
In the Godbolt above, notice that Clang (since Clang 17) understands that
`__is_trivially_equality_comparable(BetterLand)`, whereas Clang refuses to "crack open the curly braces"
of `NarrowLand`’s `operator==` to prove anything useful about that one. This type-trait feeds into
(among other places) libc++'s implementation of `std::equal`, so that we can write ([Godbolt](https://godbolt.org/z/cGd358Yr4))

    bool test(std::vector<BetterLand>& a, std::vector<BetterLand>& b) {
      return a == b;
    }

`vector`’s `operator==` dispatches to `std::equal`, which sees that we're comparing two contiguous
arrays of trivially equality-comparable types and so it generates a single `bcmp` for the entire comparison.
Contrast to the opaque, hand-coded `NarrowLand`, which for this same snippet generates a loop over many
short (12-byte) `memcmp`s. `NarrowLand` is _vastly_ worse than `BetterLand` in this example!

## Okay, but `has_unique_object_representations`...?

Right. In Steve's 2020 talk, he shows two ways that the hand-optimized `NarrowLand` could (under maintenance)
become not only inefficient but actually wrong. First, you could accidentally introduce padding bytes:

    class BadLand1 {
      unsigned char x_; // oops, 7 bytes of padding introduced here
      unsigned long long y_;
      unsigned long long z_;
      friend bool operator==(const NarrowLand& a, const NarrowLand& b) {
        // oops, the outcome now depends on indeterminate values
        return memcmp(&a, &b, sizeof(a)) == 0;
      }
    };

Second, you could accidentally change `unsigned long long` to a type that isn't itself trivially equality-comparable:

    class BadLand2 {
      double x_;
      double y_;
      double z_;
      friend bool operator==(const NarrowLand& a, const NarrowLand& b) {
        // oops, +0.0 and -0.0 now compare unequal instead of equal
        return memcmp(&a, &b, sizeof(a)) == 0;
      }
    };

Given these correctness pitfalls, Steve's first recommendation is admirably in line with
[Michael A. Jackson's](https://en.wikipedia.org/wiki/Michael_A._Jackson_(computer_scientist)) famous
"Rules of Program Optimization." In [_Principles of Program Design_](https://archive.org/details/principlesofprog00jack/page/n10) (1975),
Jackson writes:

> [In] the examples used throughout the book [...] optimization is avoided. We follow two rules in the matter of optimization:
>
> Rule 1. Don't do it.  
> Rule 2 (for experts only): Don't do it yet. [...]   
>
> Two points should always be remembered: first, optimization makes a system less reliable and harder to maintain,
> and therefore more expensive to build and operate; second, because optimization obscures structure it is difficult
> to improve the efficiency of a system which is already partly optimized.

Arguably we've already seen an example of Jackson's second point: libc++ was unable to give us the best codegen for `vector<NarrowLand>`
because the structure of `NarrowLand`'s own `operator==` was "obscured" by its "partly optimized" implementation.

Steve then suggests ([@32m15s](https://www.youtube.com/watch?v=SShSV_iV1Ko&t=1935s)) that, if for some reason you
_must_ do this partial optimization, you could improve its reliability under maintenance by adding:

    static_assert(std::has_unique_object_representations_v<NarrowLand>);

Indeed this `static_assert` would fail for either `BadLand1` or `BadLand2`. But, I cannot stress enough,
_this is not a correct use of `has_unique_object_representations`!_  In fact I don't think there is any "correct"
use of `has_unique_object_representations`; I think it is a trait without a meaningful purpose.

"But," you ask, "if it rejects `BadLand1` and `BadLand2`, and accepts `NarrowLand`, then isn't
it basically the same as the trait you and Clang call `__is_trivially_equality_comparable`? Isn't it kind of like
the 'portable spelling' of `__is_trivially_equality_comparable`?" Sadly, no. [[meta.unary.prop]/10](https://eel.is/c++draft/meta.unary#prop-10):

> The predicate condition for a template specialization `has_unique_object_representations<T>`
> shall be satisfied if and only if
> - `T` is trivially copyable, and
> - any two objects of type `T` with the same value have the same object representation, where
>     - two objects of array or non-union class type are considered to have the same value
>       if their respective sequences of direct subobjects have the same values, and
>     - two objects of union type are considered to have the same value if they have the
>       same active member and the corresponding members have the same value.
>
> The set of scalar types for which this condition holds is implementation-defined.


Here are five examples where the two traits give different answers.

## 1. libc++'s `optional<char>` (false positive)

    using T = std::optional<char>;
    static_assert(std::has_unique_object_representations_v<T>);
    static_assert(!__is_trivially_equality_comparable(T));

([Godbolt.](https://godbolt.org/z/vcxG47cKK)) This is the simplest example of how `has_unique_object_representations`
doesn't care about comparison semantics. All it looks at is trivial copyability (libc++'s `optional<char>` is trivially
copyable) and the object representation, which for `optional` is simply:

    union {
      char dummy_;
      char value_;
    };
    bool has_value_;

(You might ask why `has_unique_object_representations_v<bool>` is considered `true` instead of `false`,
given that `bool` has seven padding bits. I'm hazy on the details myself, but I believe it has to do
with what Clang will soon call [`-fstrict-bool`](https://github.com/llvm/llvm-project/pull/160790): by default
Clang assumes that all seven of those padding bits are zero, permitting it to codegen the optimal trivial
comparison; only with `-fstrict-bool=truncate` or `-fstrict-bool=nonzero` will Clang ever generate non-trivial
code for bool equality-comparison.)

If you assume `optional<char>`’s comparison can be lowered to memcmp, you'll sometimes think
that equal (disengaged) optionals are unequal.


## 2. `optional<int&>` (false positive)

C++26 `optional<int&>` has the same issue ([Godbolt](https://godbolt.org/z/8e3EeKod1)), and here the behavior
doesn't depend on your STL vendor: `optional<int&>` is guaranteed to be trivially copyable on all implementations,
and vice versa its `operator==` is guaranteed to compare the value of the _referred-to_ `int` object, rather
than the value of the `optional`’s pointer data member.

If you assume `optional<int&>`’s comparison can be lowered to memcmp, you'll sometimes think
that equal optionals (referring to distinct `int` objects with equal values) are unequal.


## 3. `pmr::polymorphic_allocator<int>` (false positive)

    using T = std::pmr::polymorphic_allocator<int>;
    static_assert(std::has_unique_object_representations_v<T>);
    static_assert(!__is_trivially_equality_comparable(T));

([Godbolt.](https://godbolt.org/z/cndG1xczn)) `polymorphic_allocator` is trivially copyable,
and it has only a single `memory_resource*` data member. But its `operator==` doesn't compare
the value of that pointer; it does a "deep comparison" via virtual dispatch to `memory_resource::do_equal`.

If you assume `polymorphic_allocator`’s comparison can be lowered to memcmp, you'll sometimes think
that equal (compatible) allocators are unequal.


## 4. `span<int>` (false positive)

    using T = std::span<int>;
    static_assert(std::has_unique_object_representations_v<T>);
    static_assert(!__is_trivially_equality_comparable(T));

([Godbolt.](https://godbolt.org/z/o9vjM6zhh)) This is a silly trivial example, included for completeness.
`std::span` simply doesn't implement equality-comparison at all — `equality_comparable<span<int>>` is
`false` — so obviously it is not trivially equality-comparable either.
But `has_unique_object_representations` doesn't care about equality comparison! `span` is trivially copyable,
and has only scalar data members with no padding, so `has_unique_object_representations` yields `true`.


## 5. libstdc++'s `exception_ptr` (false negative)

    using T = std::exception_ptr;
    static_assert(!std::has_unique_object_representations_v<T>);
    static_assert(__is_trivially_equality_comparable(T));

([Godbolt.](https://godbolt.org/z/EKb7xx6os))
We saw above that [[meta.unary.prop]/10](https://eel.is/c++draft/meta.unary#prop-10) tells `has_unique_object_representations`
to reject any type that's not trivially copyable. This rules out many resource-owning types which
are nevertheless trivially equality-comparable. The simplest example is `exception_ptr`, which holds
merely a pointer to a heap-allocated exception object; two `exception_ptr`s are equal if and only if
they hold the same pointer. Yet, because it's an _owning_ pointer, `exception_ptr` is not trivially copyable:
therefore `has_unique_object_representations` rejects it.


### Rabbit hole: `unique_ptr<int>`

Another example, you might think, would be `unique_ptr<int>`: it's isomorphic to `exception_ptr`
in all important respects (resource-owning, otherwise just a pointer).
But in fact `unique_ptr` is a complicated case, because `unique_ptr` doesn't hold _just_ a pointer;
it also holds a deleter. `unique_ptr<int>`'s layout is really more like:

    [[no_unique_address]] default_delete<int> deleter_;
    int *ptr_;

And `default_delete<int>` has no `operator==`. Therefore `unique_ptr` cannot default its own `operator==`;
therefore it must have a _user-defined_ `operator==`; therefore the compiler refuses to "crack open the
curly braces" and reports that `unique_ptr` is not (known to be) trivially equality-comparable.

Could Clang make `unique_ptr` trivially equality-comparable? Theoretically, yes, in either of two ways.
The easy, but politically fraught, way would be for Clang to add a "warranting" attribute similar
to P1144's `[[trivially_relocatable]]`, and for libc++ to use it:

    template<
      class T, class Deleter = default_delete<T>,
      bool TEC = is_empty_v<Deleter> && __is_trivially_equality_comparable(Deleter::pointer)
    >
    class [[fantasy::trivially_equality_comparable(TEC)]] unique_ptr {
      [[no_unique_address]] Deleter d_;
      Deleter::pointer p_;
      friend bool operator==(const unique_ptr& a, const unique_ptr& b) {
        return a.p_ == b.p_;
      }
    };

The more general-purpose, but harder, way would be for Clang's implementation of
`__is_trivially_equality_comparable` to understand the following convoluted idiom,
and for libc++ to use it:

    struct EmptyComparable {
      bool operator==(const EmptyComparable&) const = default;
    };

    template<class T, class Deleter = default_delete<T>>
    class unique_ptr {
      struct Helper : EmptyComparable {
        [[no_unique_address]] Deleter deleter_;
      };
      [[no_unique_address]] Helper d_;
      Deleter::pointer p_;
      friend bool operator==(const unique_ptr& a, const unique_ptr& b) = default;
    };

Here we have produced correct behavior for `operator==` entirely from defaulted
definitions. So, in theory, the compiler has enough information to understand our
`operator==`’s behavior; someone just has to teach the compiler to disentangle that information
and use it effectively. [Clang doesn't do this today.](https://godbolt.org/z/j3qxEjbEs)

This latter approach, however, is vastly complicated by `unique_ptr`’s
[many overloads of `==`](https://en.cppreference.com/w/cpp/memory/unique_ptr/operator_cmp.html).


## Conclusion and postscript

In short: `has_unique_object_representations` is not a suitable substitute for
`__is_trivially_equality_comparable`. You _must not_ use `has_unique_object_representations`
to mean "trivially equality-comparable" in generic code. It often reports `true` for
types that aren't trivially equality-comparable; it sometimes reports `false` for
types that are trivially equality-comparable.

I claim that `has_unique_object_representations` has _absolutely no valid use-case._

P.S. — "Could `has_unique_object_representations` have something to do with hashing?" Nope!
Hashing is tightly coupled to equality-comparison: if `a == b` then we must have `h(a) == h(b)`.
Nobody can say whether a particular (perhaps trivial) hash function `h` is appropriate for
a `T` unless they understand `T`’s `operator==`, and as we've seen, `has_unique_object_representations`
doesn't know or care about `==`. In fact, no part of the compiler knows anything about `std::hash`
at all; unlike `==`, `std::hash` is a pure library facility. Luckily, logic dictates
that any trivially equality-comparable `T` can be hashed by hashing its object representation:
this might produce a suboptimal hash function, but it will never distribute equal `T`s into
unequal hash buckets. So there is no need for an `__is_trivially_hashable` trait; it winds up
identical to `__is_trivially_equality_comparable`, just as
[`__is_trivially_swappable` winds up identical to P1144 `__is_trivially_relocatable`](/blog/2018/06/29/trivially-swappable/).

However, thanks to Howard Hinnant for pointing out that the paper trail behind
`has_unique_object_representations` did indeed cite "hashing" as its original motivation: see
[Howard's trait `is_uniquely_represented`](https://github.com/HowardHinnant/hash_append/blob/17a19a2/hash_append.h#L115-L120),
which became [P0029](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0029r0.html),
[P0258R0 `is_contiguous_layout`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0258r0.html),
and finally [P0258R2 `has_unique_object_representations`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0258r2.html).
It's just too bad that `has_unique_object_representations` completely failed to achieve its goal
and is now useless.
