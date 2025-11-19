---
layout: post
title: "Semantically ordered arguments, round 2"
date: 2025-01-05 00:01:00 +0000
tags:
  library-design
  proposal
  standard-library-trivia
  wg21
---

Back in 2021, I wrote that ["Semantically ordered arguments should be lexically ordered too."](/blog/2021/05/05/its-clamping-time/)
Two minor updates in that area, which are large enough to deserve a post of their own.

## `clamp` in x86-64 SSE2 assembly

A blogger who goes by "1F604" points out that
["gcc and clang sometimes emit an extra mov instruction for std::clamp on x86"](https://1f6042.blogspot.com/2024/01/stdclamp-still-generates-less-efficient.html)
(January 2024). This is because the SSE2 instructions `minsd` and `maxsd` are two-operand instructions
which return the value of their _non-destination_ operand in case of equality (e.g., when comparing `+0.0` against `-0.0`).

My previous post points out (with a hat tip to Walter E. Brown) that the STL makes a similar
mistake with `std::min` and `std::max`. The STL invariably prefers to return the _first_
(that is, leftmost) argument value, so that `std::min(-0, +0)` is `-0` and `std::max(-0, +0)` is also `-0`.
Unfortunately, on x86-64, these conventions fight with each other:

<table>
<tr><td><pre>double std::max(double a, double b) {
  return (b > a) ? b : a;
}</pre></td><td><pre>maxsd %xmm0, %xmm1
movapd %xmm1, %xmm0
retq</pre></td></tr>
</table>

`std::max`'s semantics require that we invoke `maxsd` with `a` in the _non-destination_ position, so that
it'll be preserved in case of equality. But that means that the result of `maxsd` ends up in `%xmm1`,
so we have to move it back into the return register `%xmm0`. From a codegen point of view, we'd rather
`max` had Walter Brown's semantics:

<table>
<tr><td><pre>double au_max(double a, double b) {
  return (a > b) ? a : b;
}</pre></td><td><pre>maxsd %xmm1, %xmm0
retq</pre></td></tr>
</table>

We have the same problem with `std::clamp`, which wants to return the value of its _semantically middle_
operand in case of equality. E.g., when clamping `+0.0` between `-0.0` and `-0.0`, we want to return
`+0.0`. The STL's `std::clamp` is (IMO wrongly) specified to take its semantically middle operand
in _first position_, so we have this:

<table>
<tr><td><pre>double std::clamp(double b, double a, double c) {
  return std::min(std::max(b, c), a);
}</pre></td><td><pre>maxsd %xmm0, %xmm2
minsd %xmm2, %xmm1
movapd %xmm1, %xmm0
retq</pre></td></tr>
</table>

Whereas if it had been specified to take its semantically middle operand in the _middle position_,
we'd have had this:

<table>
<tr><td><pre>double au_clamp(double a, double b, double c) {
  return std::min(std::max(b, c), a);
}</pre></td><td><pre>maxsd %xmm1, %xmm2
minsd %xmm2, %xmm0
retq</pre></td></tr>
</table>

However, all of this is contingent on SSE2's objectively strange choice to
prefer the _non-destination_ operand in `minsd` and `maxsd`, plus x86-64's (non-strange)
choice to make the first parameter register the same as the return register in this case.
None of this applies to RISC architectures with no `min` or `max` opcodes. None of this
applies to SSE2's successor AVX, either: AVX's `vminsd` and `vmaxsd` are three-operand instructions.
This is all just a quirk of SSE2.

## `std::pointer_in_range(mid, first, last)`

[P3234 "`pointer_in_range`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3234r1.html)
(Fernandes, 2024) continues the precedent set by `std::clamp`, of putting the semantically "middle"
argument lexically first rather than lexically in the middle. Admittedly there's implementation
precedent, too: Qt has [`q_points_into_range(mid, first, last)`](https://github.com/qt/qtbase/blob/f9376703c92e957c7334a2ff42a801237f53c8bf/src/corelib/tools/qcontainertools_impl.h#L29-L40)
and Boost has [`pointer_in_range(mid, first, last)`](https://github.com/boostorg/core/blob/749b6340b57ae4d64bc99a9f4e5e2b24cd01033a/include/boost/core/pointer_in_range.hpp#L30-L45).

For `pointer_in_range`, switching the parameter order makes no difference to the codegen.
First because parameters of type `T*` are passed in general-purpose registers and ISAs tend not to
privilege one general-purpose register above another. For example, x86-64 doesn't care
whether we ask "Is `%rdi` between `%rsi` and `%rdx`?" or "Is `%rsi` between `%rdi` and `%rdx`?"
And second because when we're dealing with general-purpose registers, the return register
`%rax` isn't used to pass _any_ of the parameters. So even a no-op function like
`[](int x) { return x; }` ends up performing a move.

Would I still prefer the parameters to be passed as `(first, mid, last)` instead of `(mid, first, last)`?
Yes! But for purely aesthetic and philosophical reasons, not codegen reasons.

Of course, I wrote ["Semantically ordered arguments should be lexically ordered too"](/blog/2021/05/05/its-clamping-time/) (2021-05-05)
without realizing that the parameter order affected `clamp`'s codegen.
Maybe in another three years I'll hear how parameter order affects `pointer_in_range`'s codegen, too...
