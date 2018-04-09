---
layout: post
title: 'A modest proposal for a GCC diagnostic'
date: 2018-04-09 00:02:00 +0000
tags:
  memes
  this-should-exist
---

    template<class T, class U>
    auto less(const T& t, const U& u)
        noexcept(noexcept(noexcept(t < u)))
                          ^~~~~~~~
    warning: 'noexcept(noexcept(noexcept' is too noexcept for GCC

Notice that `noexcept(noexcept(false))` is a synonym for `noexcept(true)`.

Someone the other day asked how many times in a row we have to write
[`[[unlikely]]`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0479r4.html)
before it turns into [`__builtin_unreachable()`](https://gcc.gnu.org/onlinedocs/gcc/Other-Builtins.html#index-_005f_005fbuiltin_005funreachable).

The number of times you need to write `noexcept` in a row, before it turns into
`noexcept(true)`, is exactly three.
