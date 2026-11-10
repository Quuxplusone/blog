---
layout: post
title: "Fun with designated initializers"
date: 2026-11-10 00:01:00 +0000
tags:
  contrived-examples
  designated-initializers
  implementation-divergence
excerpt: |
  C++20 (via [P0329](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0329r4.pdf))
  introduced a relatively nerfed form of designated-initializer (`.field = 42`) syntax.
  Unlike C, C++20 doesn't support out-of-order initializers, any mixture of designated
  and non-designated initializers, or array-element designators.

  Array-element designators like `[0] = 42` aren't supported by standard C++. Why not?
  Read the spoiler in <a href="https://eel.is/c++draft/diff.dcl#10.sentence-4">[diff.dcl]</a>,
  or, consider Clang's behavior on the following program
  ([Godbolt](https://godbolt.org/z/f4Psq38j7)).
---

C++20 (via [P0329](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0329r4.pdf))
introduced a relatively nerfed form of designated-initializer (`.field = 42`) syntax.
Unlike C, C++20 doesn't support out-of-order initializers, any mixture of designated
and non-designated initializers, or array-element designators.

Array-element designators like `[0] = 42` aren't supported by standard C++. Why not?
Read the spoiler in <a href="https://eel.is/c++draft/diff.dcl#10.sentence-4">[diff.dcl]</a>,
or, consider Clang's behavior on the following program
([Godbolt](https://godbolt.org/z/f4Psq38j7)).

    bool oldstyle() {
      #define ZERO 0
      std::any a[3] = {
        [ZERO] {},
      };
      return a[0].has_value(); // false
    }

    bool modernized() {
      constexpr int zero = 0;
      std::any b[3] = {
        [zero] {},
      };
      return b[0].has_value(); // true
    }

Spoiler: Clang treats `[0]{}` as a C99-style designated initializer for an array element,
using a kind of uniform-initialization style: "Initialize element 0 from this bag of
constructor arguments (which happens to be empty."
But Clang rightly treats `[zero]{}` as a lambda-expression capturing the local variable
`zero`, omitting the parameter list (which is legal, though I wish it weren't!),
and having an empty body. (That's legal C++: all compilers _must_ parse it that way!)

Ironically, GCC — which originally introduced the designated-initializer extension —
has never supported Clang's `=`-less syntax in C++ mode; although GCC _in C mode only_
does support a `=`-less syntax described in the manual as "obsolete since GCC 2.5."

    // in GNU C only
    int i[3] = { [1] 42 };
    struct A {};
    struct A a[3] = { [1] {} };

Clang supports that syntax in both C and C++ modes. In C++ mode, the initialization
of `i` gives `-Wgnu-designator`, but (since Clang 10) the initialization of `a` doesn't.
([Godbolt.](https://godbolt.org/z/93hWszd56))

    warning: use of GNU 'missing =' extension in designator [-Wgnu-designator]
        3 | int i[3] = { [1] 42 };

    warning: array designators are a C99 extension [-Wc99-designator]
        4 | struct A a[3] = { [1] {} };

Footnote: As far as I know, even in C, where out-of-order initializers are legal, no compiler
permits array-element initializers whose indices are determined at runtime. You might wish

    int a[5] = { [argc] = 1 };

were equivalent to

    int a[5] = {};
    a[argc] = 1;

but in fact both GCC and Clang reject it.
