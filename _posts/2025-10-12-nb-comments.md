---
layout: post
title: "The C++26 NB comments have arrived"
date: 2025-10-12 00:01:00 +0000
tags:
  contracts
  relocatability
  wg21
excerpt: |
  The pre-Kona WG21 mailing includes [N5028 "SoV and Collated Comments,"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/n5028.pdf)
  which is the collation of all 26 National Bodies' comments on the C++26 [CD](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#wg21-cd-dis-is-nb-wd).
  Nineteen NBs submitted comments. It is important to note that different NBs have different mechanisms for soliciting their comments.
  The US NB simply submits every comment it receives — no filter. My impression is that some other NBs (UK? Sweden?)
  have a technical discussion about every comment and submit only comments that win consensus within the NB.

  C++26 is notable for the large number of relatively unimplemented and novel features it specifies;
  and the NB comments on C++26 are notable for the many comments of the form "We should completely
  remove novel feature X." By my count:
---

The pre-Kona WG21 mailing includes [N5028 "SoV and Collated Comments,"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/n5028.pdf)
which is the collation of all 26 National Bodies' comments on the C++26 [CD](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#wg21-cd-dis-is-nb-wd).
Nineteen NBs submitted comments. It is important to note that different NBs have different mechanisms for soliciting their comments.
The US NB simply submits every comment it receives — no filter. My impression is that some other NBs (UK? Sweden?)
have a technical discussion about every comment and submit only comments that win consensus within the NB.

C++26 is notable for the large number of relatively unimplemented and novel features it specifies;
and the NB comments on C++26 are notable for the many comments of the form "We should completely
remove novel feature X." By my count:

### On P2900 Contracts

- ES49 and ES50 ask to remove Contracts.
- US51 and US52 ask to remove Contracts.
- FR53 and FR54 ask to remove Contracts.
- FI71 asks to remove Contracts.
- RO56 asks to remove Contracts' `ignore` semantic, or else remove Contracts.
- ES55 asks to redesign Contracts' "constification" feature.
- CZ58 asks to redesign Contracts' "constification" feature.
- FR14=US15 and FR113=US112 ask to keep library hardening but remove Contracts.
    RU16 preemptively asks to keep both.

### On P2786 "trivial" relocation

- BG34 asks to make P2786 trivial relocation tantamount to memcpy.
- CN's sole comment (pages 4–5) asks to make P2786 trivial relocation tantamount to memcpy.
- US81 asks to make P2786 trivial relocation tantamount to memcpy, or remove all of P2786.
- US85 asks to make P2786 trivial relocation tantamount to memcpy, or adopt P1144.
- US23 asks to remove P2786's new contextual keywords, or adopt P1144.
    FR25 asks to make them real keywords.
    US82 asks to make them attributes or annotations, or remove all of P2786.
- US24 asks to remove P2786 replaceability, or adopt P1144.
- RO134=US135 objects to P2786 `trivially_relocate` as "insufficient."
- CA133 asks to remove P2786 `relocate`.
- US131 asks to remove P2786 `relocate`.
- US130 and US266 ask to add P3516 `uninitialized_relocate`.

### Miscellaneous new features

- FR214 asks to remove `optional`'s `begin`/`end` ([P3168](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3168r1.html)).
- IT230 asks to remove `<hive>` ([P0447](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p0447r28.html)).
- FR273 asks to remove `<linalg>` ([P1673](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1673r13.html)).

For what it's worth, I support all of these removals:

Readers of this blog know that I consider P2786 worse than nothing.
P2900 Contracts is irrelevant to my day-to-day work,
but seems to add complexity to the dark corners of the language and (like C++20 Modules)
needs a lot of tooling support that doesn't exist yet; I'd rather see it in a TS first.
`optional::begin` has immediate applications in code golf (see
["PSA: `views::single` doesn't really view"](/blog/2025/09/16/single-view/)
(2025-09-16)), but I think blurring the line between ranges and single objects will
cause more grief than it prevents.
The author of `<hive>` is already redesigning that container with different performance
characteristics and complexity guarantees; `<hive>` reminds me a lot of `<regex>`,
in that its entire target audience will use a better third-party implementation
(PCRE, RE2) while their STL vendor's hasty implementation stagnates.
Likewise `<linalg>`: it's expected that all three STL vendors will ship snapshots
of [a specific reference implementation](https://github.com/kokkos/stdBLAS/);
but since that implementation is portable, it doesn't _need_ to be shipped in the STL
(anyone who wants can use it as a third-party library, and filing bug reports
directly upstream will get them fixed faster); and since it is unoptimized,
nobody will _want_ to use it. The target audience will use fast third-party
implementations of
[BLAS](https://en.wikipedia.org/wiki/Basic_Linear_Algebra_Subprograms)
while their STL vendor's unoptimized `<linalg>` stagnates.

---

Notable large features of C++26 which *no* NB proposed removing include the following.
Personally I think a majority of these deserve removal too, but nobody asked for it;
which just goes to show — each of the features above was considered (by at least
one person or NB) worse for C++'s future than _any_ of the following!

- Concept template parameters and variable-template template parameters ([P2841](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2841r7.pdf))
- Pack indexing with `ts...[2]` ([P2662](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2662r3.pdf))
- Special treatment of variables named `_` ([P2169](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2169r4.pdf))
- `#embed` ([P1967](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p1967r14.html))
- Annotations ([P3394](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3394r4.html))
- Reflection and `<meta>` ([P2996](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2996r13.html))
- `<execution>`, a.k.a. senders/receivers ([P2300](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2300r10.html))
- `<simd>` ([P1928](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1928r15.pdf))
- `<hazard_pointer>` and `<rcu>` ([P2530](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2530r3.pdf) and [P2545](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2545r4.pdf))
- `<debugging>` ([P2546](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2546r5.html))
- `<text_encoding>` ([P1885](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1885r12.pdf))

---

For comparison, the NB comments for C++20 are in [N4844](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/n4844.pdf).
I count 17 "removal" comments, among them three against `concept boolean` (US195, US196, GB197) ✅;
two each against `strong_equality` and/or `weak_equality` (US170, CA173) ✅,
[`std::as_writable_bytes`](https://eel.is/c++draft/span.objectrep) (FR244, CA252),
[`std::chrono::gps_clock`](https://eel.is/c++draft/time.clock.gps) (GB336, GB337),
Coroutines (BG50, US370),
and move-only iterators (US253, GB270);
and one each against Modules (US73), `using enum` (US43), parens-agg-init (US56), and `concept semiregular` (FR204).
Items marked with ✅ were in fact ultimately removed from the DIS.


## Postscript

Lénárd Szolnoki wins some kind of award for identifying a small defect that accumulated
NB comments from six different NBs! PL12, US118, GB119, DE120, FI121, and CZ123 all
basically ask to adopt [P3842](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3842r0.html)
as the fix for an unintended consequence of [P3068](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3068r6.html)'s
adding `constexpr` to `std::current_exception()`:

    #include <exception>
    int main() {
      try {
        throw 42;
      } catch (...) {
        const bool b = (std::current_exception() != nullptr);
          // C++23: evaluated at runtime, b is true
          // C++26: surprisingly evaluated at compile-time, b is false
        assert(b); // C++26: fails
      }
    }

While reverting the constexprness of `current_exception()` is obviously a good
fix for C++26, the underlying problem here is that C++ is still schizophrenic
about the meaning of "`const`." To working programmers it means "I promise not to modify
this variable after initialization"; but to the language spec it still retains some traces of
"I'd like to hoist the initialization of this variable from runtime to compile-time,"
which used to be a major use-case for `const` in C++98 (before `constexpr` took over that
use-case) and still has not been eradicated, nor even deprecated, as of 2025.
The key phrases here are [_potentially-constant_](https://eel.is/c++draft/expr.const#def:potentially-constant)
and [_usable in constant expressions_](https://eel.is/c++draft/expr.const#def:usable_in_constant_expressions);
specifically that a non-`constexpr` variable can be _potentially-constant_.
I'd like to see that situation deprecated in C++29; I might bring a paper to that effect.
