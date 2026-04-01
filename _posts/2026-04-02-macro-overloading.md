---
layout: post
title: 'The "macro overloading" idiom'
date: 2026-04-02 00:01:00 +0000
tags:
  msvc
  preprocessor
excerpt: |
  Here's a neat trick to create an "overloaded macro" in C, such that
  `M(x)` does one thing and `M(x, y)` does something else. For example,
  we could make a macro `ARCTAN` such that `ARCTAN(v)` calls `atan(v)`
  and `ARCTAN(y,x)` calls `atan2(y,x)`.

      #define GET_ARCTAN_MACRO(_1, _2, x, ...) x
      #define ARCTAN(...) GET_ARCTAN_MACRO(__VA_ARGS__, atan2, atan)(__VA_ARGS__)
---

Here's a neat trick to create an "overloaded macro" in C, such that
`M(x)` does one thing and `M(x, y)` does something else. For example,
we could make a macro `ARCTAN` such that `ARCTAN(v)` calls `atan(v)`
and `ARCTAN(y,x)` calls `atan2(y,x)`.

    #define GET_ARCTAN_MACRO(_1, _2, x, ...) x
    #define ARCTAN(...) GET_ARCTAN_MACRO(__VA_ARGS__, atan2, atan)(__VA_ARGS__)

So `ARCTAN(1)` expands to `GET_ARCTAN_MACRO(1, atan2, atan)(1)` expands to `atan(1)`,
while `ARCTAN(2,3)` expands to `GET_ARCTAN_MACRO(2,3, atan2, atan)(2,3)` expands to `atan2(2,3)`.

Or again, to make an "overloaded" `HYPOT` macro:

    #define GET_HYPOT_MACRO(_1, _2, _3, x, ...) x
    #define HYPOT(...) GET_HYPOT_MACRO(__VA_ARGS__, hypot3, hypot, )(__VA_ARGS__)

So `HYPOT(x)` expands to `(x)`, `HYPOT(x,y)` expands to `hypot(x,y)`, and `HYPOT(x,y,z)`
expands to `hypot3(x,y,z)`.

- `HYPOT(1,2,3,4)` expands to `GET_HYPOT_MACRO(1,2,3,4, hypot3, hypot,)(1,2,3,4)`
    expands to `4(1,2,3,4)`, which is garbage. It's likely to be ill-formed garbage, though,
    so that's not too user-unfriendly.

- `HYPOT()` expands to `GET_HYPOT_MACRO(,hypot3,hypot,)()` expands to `()`. `ARCTAN()`
    expands to `GET_ARCTAN_MACRO(, atan2, atan)()` expands to `atan()`. These are less
    user-friendly.

If you don't mind relying on a C23/C++20 preprocessor feature, you can improve the latter
experience:

    #define GET_ARCTAN_MACRO(_1, _2, x, ...) x
    #define ARCTAN(...) GET_ARCTAN_MACRO(__VA_ARGS__ __VA_OPT__(,) atan2, atan)(__VA_ARGS__)

Now `ARCTAN()` expands to `GET_ARCTAN_MACRO(atan2, atan)()` which is more cleanly ill-formed.
(It has too few macro arguments.)

> You might think you could use [a well-known GCC extension](https://gcc.gnu.org/onlinedocs/cpp/Variadic-Macros.html#:~:text=has%20a%20special%20meaning)
> to write `__VA_ARGS__##,` — but no, the token-paste operator `##` has its special meaning
> only within `,##__VA_ARGS__`, not within `__VA_ARGS__##,`.

Boost.Preprocessor implements [`BOOST_PP_VARIADIC_SIZE`](https://www.boost.org/doc/libs/latest/libs/preprocessor/doc/ref/variadic_size.html)
via a minor variation on this idiom:

    #define GET_SIZE_MACRO(_1, _2, _3, _4, _5, x, ...) x
    #define SIZE(...) GET_SIZE_MACRO(__VA_ARGS__ __VA_OPT__(,) 5,4,3,2,1,0)

Hat tip to [this blog post by "Quarterstar"](https://www.quarterstar.tech/2026/03/30/rust-to-cpp-implementing-the-question-mark-operator#the-implementation)
(March 2026); the technique is also shown by [CodeLucky](https://codelucky.com/c-macros/#6_Macro_Overloading) (September 2024),
on [StackOverflow](https://stackoverflow.com/questions/11761703/overloading-macro-on-number-of-arguments) (2012),
and presumably much older places.
[GitHub search](https://github.com/search?q=%2FGET_.*_MACRO%5B%28%5D__VA_ARGS__%2F&type=code) turns up many cases of the pattern,
even without considering variations in the `GET_*_MACRO` naming convention.

Caveat: As of 2026, MSVC's preprocessor can't handle this trick by default.
You have to tell it to behave conformingly, by adding [`-Zc:preprocessor`](https://learn.microsoft.com/en-us/cpp/build/reference/zc-preprocessor)
to your command line. (This is also how you get it to recognize `__VA_OPT__`!)
Alternatively, MSVC's old non-conforming preprocessor will accept the code as long as it's wrapped in
an additional layer of indirection. See
["The Fundamental Theorem of Software Engineering"](/blog/2018/06/18/fundamental-theorem-of-software-engineering/) (2018-06-18).

    // Work around MSVC's non-conforming preprocessor
    #define EXPAND(x) x
    #define GET_ARCTAN_MACRO(_1, _2, x, ...) x
    #define ARCTAN(...) EXPAND(GET_ARCTAN_MACRO(__VA_ARGS__, atan2, atan)(__VA_ARGS__))
