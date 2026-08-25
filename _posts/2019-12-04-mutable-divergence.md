---
layout: post
title: 'Implementation divergence with `const int i;` and `mutable`'
date: 2019-12-04 00:01:00 +0000
tags:
  implementation-divergence
  training
---

While teaching the other day, I had just finished talking about one place in which
C++ departs from its usual no-nannying philosophy: C++ prevents the programmer from writing

    const int i;

"I just want an entity with decltype `const int`! I don't care about its value. I'm
just trying to test if `f(i)` compiles when `i` is const. I'm not going to _read_ from `i`."
Unfortunately, both the paper standard and every implementation — Clang, GCC, MSVC, and ICC
respectively — forbid this construction.

    error: default initialization of an object of const type
    'const int'

    error: uninitialized 'const i' [-fpermissive]

    error C2734: 'i': 'const' object must be initialized
    if not 'extern'

    error: const variable "i" requires an initializer

(Incidentally, GCC, what happened to `const i`'s type in that error message?)

----

The same thing applies to types with default-initialized int fields. For example,

    struct A { int i; };
    const A a;

Clang, GCC, MSVC, ICC?

    error: default initialization of an object of const type
    'const A' without a user-provided default constructor

    error: uninitialized 'const a' [-fpermissive]

    (MSVC: contented silence)

    error: const variable "a" requires an initializer --
    class "A" has no user-provided default constructor

We went on to talk about the opposite of `const`, which is `mutable`.
And then a student asked me a question I hadn't thought of before.

> Since `mutable` makes the field non-`const`, does that mean it's okay again
> to define it without an initializer?

That is:

    struct B { mutable int i; };
    const B b;

Clang, GCC, MSVC, ICC?

    (Clang: contented silence)

    error: uninitialized 'const b' [-fpermissive]

    (MSVC: contented silence)

    error: const variable "b" requires an initializer --
    class "B" has no user-provided default constructor

It is not clear to me what the intended behavior of the core language is, here.

(Of course what I would _like_ to happen is for C++ to stop its overparenting
and eliminate the error even in the simplest case. This would also simplify the
C++17 standard to the tune of one sentence. In fact, C++2a instead [balloons
that one sentence into a whole paragraph](https://github.com/cplusplus/draft/commit/c6a936326699032b2c925b3875ef660164b2aca0),
with a cross-reference to [CWG 253](https://cplusplus.github.io/CWG/issues/253.html).
WG21 never met a simplification they couldn't turn into an opportunity for expansion!
See also [P1155 "More Implicit Moves" suggested wording](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1155r3.html#wording)
versus [what actually happened in C++2a](https://github.com/cplusplus/draft/commit/68aa22bc73235829d946394b7e97a5613b4d575f).)

----

Intel ICC (which reports `__EDG_VERSION__ == 500`) actually has another
interesting quirk: it seems to treat constructors implicitly generated due
to non-static data member initializers as fundamentally different animals
from regular defaulted constructors. That is, EDG produces a _warning_
(but not a hard error) for this code:

    struct C { int i = 42; };
    const C c;

Clang, GCC, MSVC, ICC?

    (Clang: contented silence)

    (GCC: contented silence)

    (MSVC: contented silence)

    warning #854: const variable "c" requires an initializer --
    class "C" has no user-provided default constructor

Adding `C() = default;` shuts up EDG's warning.
