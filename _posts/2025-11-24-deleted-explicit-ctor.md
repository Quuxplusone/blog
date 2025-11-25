---
layout: post
title: "When should a `=delete`’d constructor be `explicit`?"
date: 2025-11-24 00:01:00 +0000
tags:
  constructors
  cpplang-slack
  library-design
  overload-resolution
---

C++20 added `atomic_ref<T>` to the Standard Library (see
["Guard nouns or guard verbs?"](/blog/2023/11/11/guard-nouns-or-guard-verbs/) (2023-11-11)).
But somehow C++20 overlooked `atomic_ref<const T>`; it's finally permitted in C++26,
thanks to Gonzalo Brito Gadeschi's [P3323](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3323r1.html).
But then there was a new problem: since `const T&` [binds to both lvalues and rvalues](/blog/2022/02/02/look-what-they-need/),
suddenly it is legal to write

    auto ar = std::atomic_ref<const int>(42);

which will create a dangling reference.

Therefore, [LWG 4472](https://cplusplus.github.io/LWG/issue4472) proposes to add a deleted constructor to
`atomic_ref`:

     template<class T>
     struct atomic_ref {
       constexpr explicit atomic_ref(T&);
       explicit atomic_ref(T&&) = delete; // newly added
       ~~~~
     };

Notice that the conversion from `T&` to `atomic_ref<T>` is already `explicit`, so we're not worried here about
people _accidentally_ binding an `atomic_ref<const X>` to a temporary `X`; they could only hit this deleted codepath
on purpose. So `atomic_ref` is fundamentally different from types like `function_ref` and `string_view`, which are
designed as implicit conversion targets and as "parameter-only types"
(see ["Value category is not lifetime"](/blog/2019/03/11/value-category-is-not-lifetime/) (2019-03-11)).
Deleting this constructor of `atomic_ref` doesn't really hurt anyone, although I question whether it _helps_ anyone
either.

But there's something interesting about this constructor: it is the Standard's first deleted `explicit` constructor!
I wondered: If we're `=delete`’ing it anyway, does it really matter whether it's marked `explicit` or not?

> That is, I wondered if deleted functions end up in a similar situation to CTAD deduction guides.
> C++'s grammar permits `explicit` on CTAD deduction guides, but the keyword has no useful effect there.
> You should omit `explicit` from deduction guides.
> But then, [you shouldn't use CTAD](/blog/2018/12/09/wctad/), either.

A correspondent on [the cpplang Slack](https://cppalliance.org/slack/) set me right: `explicit` _can_ matter
to a deleted constructor. [Godbolt](https://godbolt.org/z/ooMc5bPhv):

    template<class T>
    struct AtomicRef {
      explicit AtomicRef(T&);
      MAYBE_EXPLICIT AtomicRef(T&&) = delete;
    };

    struct X {
      X(int);
    };

    void f(AtomicRef<const int>);
    void f(X);

    void test() {
      f(42);
    }

Here, if `MAYBE_EXPLICIT` is defined away, the call to `f` becomes ambiguous: did we mean to call `f(X)`
(which would work) or `f(AtomicRef<const int>)` (which would be a case of
["I know what you're trying to do, and you're wrong"](/blog/2021/10/17/equals-delete-means/))?
Whereas if it's defined as `explicit`, the call to `f` is unambiguous and OK: `f(AtomicRef<const int>)` isn't a candidate
and `f(X)` is the only viable interpretation.

Notice that if `AtomicRef(T&)` were non-`explicit` ([Godbolt](https://godbolt.org/z/51cPebYx5)),
then the call to `f` would be ambiguous no matter what, and this example would be irrelevant.

So yes, `explicit` can matter even to deleted constructors. It seems to me that a good rule of thumb is:

> When you `=delete` a constructor, you're usually doing it to "overrule" another, greedier constructor.
> Craft the `explicit`-ness of your deleted constructor to match the `explicit`-ness of the constructor
> you intend to overrule.
