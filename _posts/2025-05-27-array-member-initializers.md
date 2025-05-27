---
layout: post
title: 'Array member-initializers in GCC'
date: 2025-05-27 00:01:00 +0000
tags:
  antipatterns
  compiler-diagnostics
  implementation-divergence
  initialization
  language-design
  war-stories
excerpt: |
  The other day I ran across some code like this:

      template<class T>
      struct Holder {
        T t_;
        explicit Holder() : t_() {}
        Holder(const Holder& rhs) : t_(rhs.t_) {}
        ~~~~
      };

  This was in an old codebase, which until recently had still been using
  GCC's `-Weffc++` to enforce C++98 idioms such as the explicitly non-defaulted
  copy constructor depicted above.

  > If you're still using `-Weffc++`, please,
  > [stop using it!](https://github.com/google/googletest/issues/898#issuecomment-332582070)
---

The other day I ran across some code like this:

    template<class T>
    struct Holder {
      T t_;
      explicit Holder() : t_() {}
      Holder(const Holder& rhs) : t_(rhs.t_) {}
      ~~~~
    };

This was in an old codebase, which until recently had still been using
GCC's `-Weffc++` to enforce C++98 idioms such as the explicitly non-defaulted
copy constructor depicted above.

> If you're still using `-Weffc++`, please,
> [stop using it!](https://github.com/google/googletest/issues/898#issuecomment-332582070)

We'd pulled this codebase forward to C++17 and GCC 12, and all was going swimmingly.
We even enabled [PC-Lint](https://pclintplus.com/demo/). And that's when PC-Lint complained
about a usage like this:

    using Node = Holder<int[2]>;
    Node n1;
    Node n2 = n1;

    h.cpp:5 error 21: array initializer must be an initializer list
    h.cpp:5 supplemental 891: in instantiation of member function
      'Holder<int [2]>::Holder' requested here
    h.cpp:11 supplemental 897: in instantiation of function template
      'Holder<int [2]>::Holder' triggered here

"Array initializer must be an initializer list"? Hm, I see: When `T` is `int[2]`,
`t_` is an array, and indeed it's not valid C++ to initialize an array with a
copy of another array.

    int a[2];
    int b[2] = a; // ERROR
    int c[2](a);  // ERROR
    struct S { int d[2]; S() : d(a) {} }; // ERROR, but...
      // accepted from GCC 4.7 until GCC 14

However, the GCC compiler itself didn't flag `d` as an error until GCC 14!
(This was [GCC bug 54965](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=59465).)
GCC from 4.7.x to 13.x would, as an "accidental extension" to standard C++,
allow you to initialize one array with another. Luckily, the codegen for this
is what you'd expect: it does indeed copy the contents of the array.
So that's how we were able to use this "feature" for over a decade
without noticing anything amiss.

Furthermore, even though GCC 4.6.x rejected the code above, it (and going back
at least as far as GCC 3.4, released in 2004) would still let you initialize one
array with another as long as the initialization wasn't trivial:

    struct S {
      std::string s_[2];
      S(const S& rhs) : s_(rhs.s_) {}
        // accepted from the dawn of time until GCC 14
    };

But Clang, MSVC, and EDG have never accepted this extension, as far as I can tell;
and neither has PC-Lint. So as we move to PC-Lint (and someday even to GCC 14), we
have to clean up this copy constructor.

---

The correct fix here, of course, is to stop manually defining your copy constructors.
Write `=default` instead.

    struct S {
      std::string s_[2];
      S(const S& rhs) = default;
    };

If you can't `=default` your copy constructor for some other reason, then you'll just have
to turn the member-initializer into a default-initialization followed by a loop over assignment:

    struct S {
      std::string s_[2];
      S(const S& rhs) {
        std::copy(rhs.s_, rhs.s_ + 2, s_);
      }
    };

Alternatively, you could switch from `T[N]` to `std::array<T, N>` — although see
["Prefer core-language features over library facilities"](/blog/2022/10/16/prefer-core-over-library/) (2022-10-16) —
because even though an _array_ can't be copy-initialized like this,
a _struct containing_ an array (which is all `std::array` is) _can_ be.

    struct S {
      std::array<std::string, 2> s_;
      S(const S& rhs) : s_(rhs.s_) {}
    };

---

Notice that `Holder`'s default constructor above is no problem (syntactically);
it's totally valid to value-initialize an array data member with empty parentheses.
In fact, in this generic context, it's probably the best option.

Of course in ordinary code you should _prefer_ to use a default member initializer,
so that you can default your zero-argument constructor:

    struct TwoInts {
      int data_[2] = {};
      explicit TwoInts() = default;
    };

But sadly C++ has no generic syntax that works to value-initialize _both_ arrays and
non-arrays. You'd think `= T()` would Just Work for arrays, like it works for all other
value-initializable types in the language:

    template<class T>
    struct Holder {
      T t_ = T();  // fantasy syntax
      explicit Holder() = default;
      ~~~~
    };

but no, `T()` is [deliberately and surgically broken](https://eel.is/c++draft/expr.type.conv#1.sentence-6)
when `T` is an array type, for some unknown historical reason.
(And `T{}` [fails](https://godbolt.org/z/n6xY8ns6n) when `T` is an array of `ExplicitlyDefaultConstructible`,
because `T{}` does [list-initialization](https://eel.is/c++draft/dcl.init#general-16)
instead of value-initialization.)
[CWG914](https://cplusplus.github.io/CWG/issues/914.html) asks for a paper to fix
this defect; no such paper has materialized yet.
