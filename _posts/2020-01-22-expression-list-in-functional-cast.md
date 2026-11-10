---
layout: post
title: 'Hidden `reinterpret_cast`s'
date: 2020-01-22 00:02:00 +0000
tags:
  contrived-examples
  cpplang-slack
  pitfalls
  templates
excerpt: |
  [Here's](https://godbolt.org/z/hZYF2Q) a fun little puzzle, courtesy of Richard Hodges on Slack.

      template<class Vector>
      void test(Vector& vec) {
          using E = decltype(vec[0]);
          for (int i=0; i < 10; ++i) {
              vec.push_back(E(i));
          }
      }

      int main() {
          std::vector<double> v;
          test(v);
          for (int i=0; i < 10; ++i) {
              printf("%f\n", v[i]);
          }
      }

  On any compiler you care to name, this code compiles with no warnings or errors — and produces
  utter garbage at runtime! Can you spot the bug? Spoiler below the break.
---

[Here's](https://godbolt.org/z/hZYF2Q) a fun little puzzle, courtesy of Richard Hodges on Slack.

    template<class Vector>
    void test(Vector& vec) {
        using E = decltype(vec[0]);
        for (int i=0; i < 10; ++i) {
            vec.push_back(E(i));
        }
    }

    int main() {
        std::vector<double> v;
        test(v);
        for (int i=0; i < 10; ++i) {
            printf("%f\n", v[i]);
        }
    }

On any compiler you care to name, this code compiles with no warnings or errors — and produces
utter garbage at runtime! Can you spot the bug? Spoiler below the break.

----

The trick is inside `test()` — that explicit conversion from `int` to `E`.
If you saw this code verbatim in a real codebase, you'd probably smell something funky
on the line `vec.push_back(E(i))`. What is that explicit conversion doing there, when
`vec.push_back(i)` would have done an implicit conversion anyway? Is it possible that this
is our bug? ...But `double(i)` should accomplish the same thing whether the conversion is
explicit or implicit. Is it possible that _we're not converting `i` to `double` here?_

Right! We're converting `i` to `E`, where `E` is `decltype(vec[0])`... and since `vec[0]`
is a modifiable lvalue expression, `decltype(vec[0])` is actually `double&`. (Remember that
when we take `decltype` of the _name_ of a variable or member, we get its declared type;
but when we take `decltype` of an arbitrary non-name expression, we also get its value category.
[Stack Overflow has the details.](https://stackoverflow.com/questions/3097779/decltype-and-parentheses/3097804))

So if `E` is a type alias for `double&`, then what is `E(i)`? Well, this uses a little-known
(and reviled) quirk of C++: the functional cast notation _looks_ like you're calling
a nice safe constructor, but it's _actually_ a synonym for the bad old C-style cast, which means
that in some cases it can _actually_ be a `reinterpret_cast`!
(WG21 members can view [this EWG reflector thread from June 2019](http://lists.isocpp.org/ext/2019/06/10413.php)
for the scary details.)

When we write `E(i)`, it means the same thing as `(E)i`, which is to say `(double&)i`,
which is to say `*(double*)&i`. The result is that `vec.push_back(E(i))` ends up treating the
bit-pattern of `i` (and the four garbage bytes following it) as an eight-byte `double` value,
and pushing a copy of that nonsense value onto our vector.

But you needn't worry too much about hidden bugs of this type in your own codebase.
The original sin here was that we were using functional-cast notation with a type `E`
which was a _reference type_ — and we got that reference type via a very suspect and
unidiomatic application of `decltype`. If we had just written any of the following
saner-looking lines instead, we'd have had no bug:

    template<class Vector>
    void test(Vector& vec) {
        for (int i=0; i < 10; ++i) {
            vec.push_back(i);  // OK, no explicit cast
        }
    }

    template<class Vector>
    void test(Vector& vec) {
        using E = typename Vector::value_type;  // OK, not a reference type
        for (int i=0; i < 10; ++i) {
            vec.push_back(E(i));
        }
    }

    template<class E>
    void test(std::vector<E>& vec) {  // OK, not a reference type
        for (int i=0; i < 10; ++i) {
            vec.push_back(E(i));
        }
    }

----

GCC has one extra wrinkle here. What do you suppose happens with [this code](https://godbolt.org/z/xdCnua)?

    template<class T>
    auto allocatorize(T&& t) {
        std::allocator<int> a;
        return T(std::forward<T>(t), a);
    }

    void test2() {
        std::vector<int> v;
        std::vector<int> w = allocatorize(v);
    }

GCC in `-fpermissive` mode complains:

    warning: expression list treated as compound expression
    in functional cast [-fpermissive]
        6 |     return T(std::forward<T>(t), a);
          |            ^~~~~~~~~~~~~~~~~~~~~~~~

This code has the same bug as our `E(i)` code: `T` is deduced as a reference type, and then `T(...)` is treated as
a functional-style cast to a reference type. It definitely does _not_ call the allocator-aware constructor of
`vector<int>`!  Everyone but GCC (and GCC too, in non-permissive mode) rejects this attempt to initialize a
reference type with the list of expressions `(std::forward<T>(t), a)`. But GCC in permissive mode has an extension
that treats that expression-list as an application of the comma operator, so `(std::forward<T>(t), a)` ends up
being treated as equivalent to `a`.

> We don't get a warning about "left operand of comma operator has no effect" because no compiler
> recognizes the function call `std::forward<T>(t)` as side-effect-free. I generally write
> `std::forward<T>(t)` as `static_cast<T&&>(t)` to save compilation time, but I guess this is another
> reason you might want to write `static_cast<T&&>(t)` — to improve the compiler's ability to diagnose
> bad code!

----

It would be useful for Clang (or anyone) to implement a new warning `-Whidden-reinterpret-cast`
to diagnose any cast that acts as a `reinterpret_cast` without using that spelling. Similarly,
`-Whidden-const-cast`. Similarly, something like `-Wunsafe-functional-cast` for any functional-style cast
`T(x)` that isn't equivalent to either a `static_cast` or a call to a constructor or conversion operator.
