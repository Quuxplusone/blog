---
layout: post
title: "Constexpr `factors_of`"
date: 2025-03-14 00:01:00 +0000
tags:
  compile-time-performance
  constexpr
  math
  cpplang-slack
excerpt: |
  I'm just now getting around to blogging this snippet from March 2024
  (hat tip to Chip Hogg). Apparently some units libraries — things like
  [Au](https://aurora-opensource.github.io/au/main/discussion/implementation/vector_space/#magnitude)
  and [mp-units](https://mpusz.github.io/mp-units/latest/blog/2024/09/27/mp-units-230-released/#common-units) —
  find it useful to represent linear combinations of units as the products of primes;
  for example, if you represent "meters" as 2 and "seconds" as 5,
  then "meter-seconds" is 10. Or at least that's the general idea, I think.
  Don't quote me on that part.

  This means there's a market for fast compile-time versions of the functions `next_prime_after(p)`
  and `prime_factors_of(c)`. Chip even went so far as to propose that vendors should provide
  "largest prime factor of `c`" as a builtin; see [P3133 "Fast first-factor finding function"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3133r0.html)
  (February 2024). I said, "You don't need the vendor to do your factoring; you can write that in
  general-purpose C++ already!"
---

I'm just now getting around to blogging this snippet from March 2024
(hat tip to Chip Hogg). Apparently some units libraries — things like
[Au](https://aurora-opensource.github.io/au/main/discussion/implementation/vector_space/#magnitude)
and [mp-units](https://mpusz.github.io/mp-units/latest/blog/2024/09/27/mp-units-230-released/#common-units) —
find it useful to represent linear combinations of units as the products of primes;
for example, if you represent "meters" as 2 and "seconds" as 5,
then "meter-seconds" is 10. Or at least that's the general idea, I think.
Don't quote me on that part.

This means there's a market for fast compile-time versions of the functions `next_prime_after(p)`
and `prime_factors_of(c)`. Chip even went so far as to propose that vendors should provide
"largest prime factor of `c`" as a builtin; see [P3133 "Fast first-factor finding function"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3133r0.html)
(February 2024). I said, "You don't need the vendor to do your factoring; you can write that in
general-purpose C++ already!"

As a proof of concept, I took the (GPL-licensed) [code of GNU `factor`](https://github.com/coreutils/coreutils/blob/master/src/factor.c),
stripped out a good portion of it, and added `constexpr` in all the appropriate places,
producing a C++20-friendly `constexpr std::vector<uintmax_t> factors_of(uintmax_t)`
([Godbolt](https://godbolt.org/z/8sWcP9ojG); [backup](/blog/code/2025-03-14-constexpr-factor.cpp)).

The code depends on two existing compiler intrinsics: `__builtin_ctzll` (counts trailing zeroes in a 64-bit input)
and `unsigned __int128`. These seem to be well supported across GCC and Clang
on all 64-bit targets I tested (x86-64, ARM64, Power64, RISC-V), and also supported by
[EDG in its GCC-compatible mode](https://godbolt.org/z/bcKG3oeY4). On MSVC you must replace them
with [the STL team's secret `std::_Uint128` type](https://stackoverflow.com/a/76440171/1424877)
and C++20's [`std::countr_zero`](https://en.cppreference.com/w/cpp/numeric/countr_zero) respectively.
Of course we could use `std::countr_zero` on GCC and Clang too; but that would cost a lot more
constexpr ops.

On GCC, `-fconstexpr-ops-limit=2` suffices to compile the following initialization.
On Clang, magically, you can get away even with `-fconstexpr-steps=0`.

    constexpr int x = __builtin_ctzll(1uLL);

But you need `-fconstexpr-ops-limit=32` in order to compile this one.
On Clang `-fconstexpr-steps=6`; on MSVC (with MS STL) `-constexpr:steps 36`.

    #include <bit>
    constexpr int x = std::countr_zero(1uLL);

There are still plenty of very large integer values that can't be factored even in a million
constexpr steps — at least, not with this code as it stands. I think it would be interesting
to keep improving this code's "constexpr performance" to lower that number — maybe even to the
point where it could factor _any_ `uintmax_t` value within any vendor's default constexpr step limit —
but I have no plans to mess with it any more myself.
