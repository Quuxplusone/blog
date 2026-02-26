---
layout: post
title: "Is `__int128` integral? A survey"
date: 2019-02-28 00:01:00 +0000
tags:
  implementation-divergence
  standard-library-trivia
---

How well is `__int128` supported, and what are its
properties? A Godbolt-based survey of the Big Four compilers.
Spoiler alert: libc++ thinks it's integral, old versions of libstdc++ think it's integral
only in `-std=gnu++XX` mode(!), and MSVC thinks it doesn't exist at all.

Many thanks to Rein Halbersma for alerting me to libstdc++'s different behavior
in `-std=gnu++XX` mode versus `-std=c++XX` mode. (UPDATE, 2021-07-16: And
to Jonathan Wakely for alerting me to libstdc++ 10.3's new behavior!)

UPDATE, 2026-02-26: libstdc++ 16.x (starting with
[this commit](https://github.com/gcc-mirror/gcc/commit/ea42e28cbc3d4d579666582e8893e7629bf6a390))
will treat `__int128` in the same way as libc++! This is good news,
and renders most of this blog post obsolete.
Thanks to Giuseppe D'Angelo for alerting me.


## Compiler support

    using I128 = __int128;

GCC, Clang, and Intel ICC all support a built-in `__int128` type.
Microsoft MSVC does not support any 128-bit integral type as far as I'm aware.

On GCC, Clang, and ICC, `__int128` is a token similar to `int`: you can modify
it with `unsigned` to produce `unsigned __int128`. However, all three front-ends
also support the built-in synonyms `__int128_t` and `__uint128_t`.

    static_assert(std::is_same_v<__int128_t, __int128>);
    static_assert(std::is_same_v<__uint128_t, unsigned __int128>);
        // PASSES on all three compilers

Both `__int128_t` and `__uint128_t` are safe to use in non-type template parameters
and constant expressions, on all compilers that support them.


## Sneaky macros

libc++ treats `__int128` as an integer type, full stop.

The story on libstdc++ is complicated. Prior to 16.x, libstdc++ doesn't do anything special for
`__int128`, but it does provide _macro hooks_
([link](https://github.com/gcc-mirror/gcc/blob/87798f3f2cf97b30e75110b5a8d2964aef62be02/libstdc%2B%2B-v3/include/std/type_traits#L285-L293))
so that for example
`__GLIBCXX_TYPE_INT_N_0` will be treated as an integral type, whatever that macro
expands to. (If it doesn't expand to anything, it'll be ignored.)

If you compile with GCC in `-std=c++XX` mode (for any `XX`),
`__GLIBCXX_TYPE_INT_N_0` won't be defined and so libstdc++ will consider
`__int128` a non-integral type.

But if you compile with GCC in `-std=gnu++XX` mode (for any `XX`), then the compiler
will predefine these two macros:

    #define __GLIBCXX_BITSIZE_INT_N_0 128
    #define __GLIBCXX_TYPE_INT_N_0 __int128

[As of July 2016-ish](https://bugs.llvm.org/show_bug.cgi?id=23156), Clang also
defines these two macros in `-std=gnu++XX` mode.

> You might wonder if these macros have any relationship with `_GNU_SOURCE`: They do not.
> Both Clang and GCC unconditionally define `_GNU_SOURCE` in all modes, anyway.
> If you're manually passing `-D_GNU_SOURCE` in your `CFLAGS`, like a lot of projects
> I've seen, well, you can stop doing that.

By the way, you can use the options `-dM -E` to view all your compiler's predefined macros:

    clang++ -std=c++17 -dM -E test.cpp

The upshot is, if you compile in `-std=gnu++XX` mode, libstdc++ (prior to 16.x)
will treat `__int128` and `unsigned __int128` as integral types with the proper
numeric limits; but if you compile in the usual `-std=c++XX` mode, libstdc++
will not do any of that.

libstdc++ 16 improves this story: it now invariably considers `__int128` and `unsigned __int128`
to be integer types. The mechanism is a little convoluted: in `-std=gnu++XX` mode the compiler
predefines `__GLIBCXX_BITSIZE_INT_N_0=128` and `__GLIBCXX_TYPE_INT_N_0=__int128` as always,
and libstdc++ 16 picks it up by that route as always.
In `-std=c++XX` mode the compiler predefines `__STRICT_ANSI__=1` as always; and when libstdc++ 16
sees that `__STRICT_ANSI__` is defined, it adds `__int128` to the list of integer types as a
special case, instead.

## Relationship to integer types and `intmax_t`

    static_assert(std::is_integral_v<__int128>);
        // TRUE on libc++, "it depends" on libstdc++

libstdc++ 15-and-earlier (in standard, non-`gnu++XX` mode) leaves `is_integral_v<__int128>` as `false`.
This makes a certain amount of sense from the library implementor's point of view,
because `__int128` is not one of the _standard_ integral types, and furthermore,
if you _call_ it integral, then you have to face the consequence that `intmax_t`
(which is 64 bits on every ABI that matters) is kind of lying about being the
"max."

libstdc++ 16 (and earlier in `-std=gnu++XX` mode) makes `is_integral_v<__int128>` come out to `true`,
as described in the previous section on sneaky macros.

Meanwhile, libc++ simply
[sets](https://github.com/llvm-mirror/libcxx/blob/aeeba70ab15e1469841c0f3f1f9c78211a541699/include/type_traits#L738-L741)
`is_integral_v<__int128>` to `true`, unconditionally.

Yes, this means that there often exist
integral values that do not fit in `intmax_t`! ([Godbolt.](https://godbolt.org/z/lMD4Iu))

> In case you're wondering: libstdc++ in standard C++17 mode considers `__int128` a "compound" type.
> The definition of "compound type" is simply "anything that's not a fundamental type,"
> where "fundamental" encompasses the arithmetic types (both integral and floating-point), `void`,
> and `nullptr_t`. So for example `int*` is also a compound type.


## Numeric limits

Prior to the GCC 10.3 release, libstdc++ specialized `numeric_limits<__int128>` only in non-`gnu++XX` mode.
This [had the unfortunate effect](https://godbolt.org/z/RpaD-7) of

    constexpr __int128 int128_max =
        std::numeric_limits<__int128>::max();
    static_assert(int128_max == 0);  // TRUE on libstdc++ in std mode before 10.3

However, since GCC 10.3
(specifically [this August 2020 commit](https://github.com/gcc-mirror/gcc/commit/386fd16c551188e20d5b1684b7139e4269f9a739#diff-c454605c184994804a0a061dd4862ddc94799af2bb6961c144ab44c71b39cf60R1655-R1660)),
libstdc++ has provided the appropriate specialization in all modes, just like libc++, so that
`numeric_limits<__int128>::min()` and `numeric_limits<__int128>::max()`
have the appropriate values instead of zero.

Naturally, `numeric_limits<__int128>::is_integer` is `true` whenever the specialization is
provided at all — i.e., unconditionally on libc++ and libstdc++ 10.3+, and on older libstdc++
in `-std=gnu++XX` mode. So:

    static_assert(std::numeric_limits<__int128>::is_integer);  // TRUE everywhere since August 2020
    static_assert(std::is_integral_v<__int128>);  // still FALSE on libstdc++ in std mode


## Signedness

libc++ reports that `is_signed_v<__int128>` and `is_unsigned_v<unsigned __int128>`.
libstdc++ 15-and-earlier in standard mode of course denies both, because only arithmetic types can be
signed or unsigned, and libstdc++ denies that `__int128` is arithmetic.

As for `make_signed`, libc++ (and libstdc++ in `gnu++XX` mode) reports what you'd expect —
`make_signed<__uint128_t>::type` is `__int128_t`, and `make_unsigned<__int128_t>::type` is `__uint128_t`.

Trying to instantiate `make_signed<__uint128_t>` on libstdc++ 15-and-earlier in standard mode
[results in](https://godbolt.org/z/bfaj3MGsc) a hard error which is not SFINAE-friendly.
(It's undefined behavior to instantiate `make_signed` of any type which is neither
integral nor an enumeration type.)

Incidentally, today I learned that `is_signed_v<float> == true` but [`make_signed_t<float>`
is a hard error](https://godbolt.org/z/t6v1CJ).


## Incrementability

UPDATE, 2021-07-22: Jonathan Wakely alerted me to some new wrinkles in C++20. C++20 gives us
at least two relevant library concepts: `std::integral` and `std::incrementable`.

`std::integral` is true of integral types, i.e., `std::integral<T>` is true iff
`std::is_integral_v<T>` is true. On libc++ (or libstdc++ in `-std=gnu++XX` mode)
they're both true; on libstdc++ in `-std=c++XX` mode they're both false.

`std::incrementable` refines `std::weakly_incrementable`, which is modeled by
[integer-like types](https://en.cppreference.com/w/cpp/iterator/weakly_incrementable)
and also by pointer-like, iterator-like types. It is significant because if a type does not
satisfy `std::weakly_incrementable`, then (by definition) it is not integer-like, and
that means that it can't be used as any iterator's `difference_type`. Since we certainly
want to be able to use `__int128` as the `difference_type` of, say, `iota_view<__int128>`,
it's important that `__int128` be considered "integer-like." Therefore,
[libstdc++ in `-std=c++XX` mode specializes](https://github.com/gcc-mirror/gcc/commit/5e9ad288eb6fb366142b166e7985d16727b398e1)
C++20's [`std::incrementable_traits`](https://en.cppreference.com/w/cpp/iterator/incrementable_traits)
for both `__int128_t` and `__uint128_t`.

On libc++, and libstdc++ in `-std=gnu++XX` mode, `__int128` is automatically `std::incrementable`
by virtue of being `std::integral`; it doesn't need those additional specializations.
(Note that `std::integral` does not _subsume_ `std::incrementable`;
[that's another story.](/blog/2018/11/26/remember-the-ifstream))

----

For more on `__uint128_t` arithmetic, see
["The abstraction penalty for wide integer math on x86-64"](/blog/2020/02/13/wide-integer-proof-of-concept/) (2020-02-13).
