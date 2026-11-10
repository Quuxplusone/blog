---
layout: post
title: "`operator auto`"
date: 2018-07-12 00:01:00 +0000
tags:
  contrived-examples
  implementation-divergence
  metaprogramming
  proposal
  rant
  today-i-learned
---

In [a previous post on parameter-only types](/blog/2018/03/28/borrow-types-round-2),
I mentioned in passing the idea of `operator auto`. For the past couple of years,
when I've used that phrase, I've meant something like what's proposed in
Joël Falcou, Peter Gottschling, and Herb Sutter's [P0672 "Implicit Evaluation
of `auto` Variables"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0672r0.pdf) (2017).
That is, we have some class with a bunch of operators that return
[expression templates](https://en.wikipedia.org/wiki/Expression_templates),
or perhaps just proxies (like `vector<bool>::iterator`). Anyway, they return
objects of some ugly temporary type, probably containing dangling references.

    template<class Left, class Right, other, stuff>
    class ExpressionTemplate {
        Left *left; Right *right;
        other stuff;
    };

    class Matrix {
        Matrix(ExpressionTemplate&&);

        friend ExpressionTemplate<some, stuff>
        operator+(const Matrix&, const Matrix&);

        friend ExpressionTemplate<different, stuff>
        operator+(const Matrix&, ExpressionTemplate<some, stuff>);

        friend ExpressionTemplate<different, stuff>
        operator+(ExpressionTemplate<some, stuff>, const Matrix&);
    };

So now when we write

    Matrix a, b;

    Matrix c = a + b + Matrix(1,0, 0,1);
    
it temporarily generates a bunch of intermediate objects
of type `ExpressionTemplate<some, complicated, stuff>`, each
of which contains pointers to other objects in the expression.
The top-level `ExpressionTemplate` object contains a `left` pointer
to the temporary `ExpressionTemplate` object representing `a + b`,
and a `right` pointer to our temporary (rvalue) `Matrix` object.
But then that `ExpressionTemplate` is implicitly converted back into a
`Matrix`, which does the math and reifies the result with no more
pointers, and we're all good.

But if we accidentally write

    auto c = a + b + Matrix(1,0, 0,1);

this is suddenly not fine! Now the type of `c` is `ExpressionTemplate`,
and it contains pointers to temporary objects whose lifetime has
ended — they're gone! Now when we try to use `c` as a `Matrix`,
either it won't compile (the lucky case) or it will implicitly convert
to `Matrix` which will try to do the math (on objects which have
disappeared) and it'll segfault (the semi-lucky case) or just give the
wrong answer (the worst case).

Falcou et al. want to solve this by letting the `ExpressionTemplate` class
indicate, "When I am captured by `auto`, here is the type I *actually* want
to be captured as." In this case, we want `ExpressionTemplate` to be
captured as `Matrix`.

As mentioned in [my previous post](/blog/2018/03/28/borrow-types-round-2),
C++'s native reference types already behave as if they have an implicit
`operator auto`. When you `auto` a reference type, you don't get the actual
reference type — you get the *referenced* type!

    template<class T>
    class NativeReferenceTo {
        T *ptr;

        // P0672 proposed this syntax, among others
        T operator auto() const { return *ptr; }
    };

----

Anyway, at the [ACCU Bay Area C++ meetup](https://www.meetup.com/ACCU-Bay-Area/events/251996844/)
last night, in a conversation afterward I mentioned `operator auto`, and
Bryce Adelstein Lelbach informed me that `operator auto` was already in the language!
"That's a deduction context," he said. And he was right!

    struct S {
        operator auto() { return 42; }
    };

    int main() {
        S s;
        double x = s;
    }

This is [UPDATE: “was”] a valid C++14 program. But it doesn't do what you'd expect (even if you expected it
to compile, which I certainly didn't!). Here the word `auto` is essentially standing in for
the *return type* of the function, and then C++14's "return type deduction" kicks in to deduce
the return type of the function as `decltype(42)` i.e. `int`. So this `S` is precisely equivalent
to

    struct S {
        operator int() { return 42; }
    };

Notice that this is grossly different from

    struct S2 {
        template<class T>
        operator T() { return 42; }
    };

In case `S2`, the conversion operator's return type is deduced from its *call site*, not from
its body. Converting `S2` to `X` will always work, and always yield `X{42}`.

And then, if we combine both...

    struct S3 {
        template<class T>
        operator auto() { return 42; }
    };

...GCC helpfully warns us that this member function cannot ever be called, because it needs to
deduce `T` from its call site, and its call site needs to consider all possible `operator X`s
to perform overload resolution, and its call site isn't allowed to instantiate *any* of the
`operator X`s until it's picked the right one. In other words, this seems to be the one place
in C++ where the caller performs overload resolution based on a function's *return type* instead
of based on its *parameter types*. So naturally this does not play well with C++14 return type
deduction.

[If we give `T` a default](https://godbolt.org/g/8iYJNt), so that it doesn't
*need* to be deduced, then GCC and Clang disagree: GCC says "I can't figure out which `operator X`
you want me to instantiate first," whereas Clang somehow eagerly instantiates enough of the function
body to find out its return type, and thereafter treats the `auto` as equivalent to `int`.

Modest proposal: let's remove `operator auto` from C++, thus freeing up more real estate for P0672,
and also eliminating the source of blog posts like this one.

---

UPDATE: In November 2025, we successfully removed `operator auto` from C++! This was done as
a [DR](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#dr)
(namely [CWG 1670](https://cplusplus.github.io/CWG/issues/1670.html)). No vendor has yet implemented the
fix as of November 2026; but once they do, you should expect them to reject the above "valid C++14 program"
even in `-std=c++14` mode.
