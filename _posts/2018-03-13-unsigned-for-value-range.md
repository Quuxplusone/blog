---
layout: post
title: 'The "`unsigned` for value range" antipattern'
date: 2018-03-13 00:02:00 +0000
tags:
  c++-style
  rant
  undefined-behavior
excerpt: |
  At the WG21 committee meeting which is currently underway in Jacksonville, JF Bastien will be presenting [a proposal](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0907r0.html) to make C++'s `int` data type wrap around on overflow.
---

## Background: Signed Integers Are (Not Yet) Two's Complement

At the WG21 committee meeting which is currently underway in Jacksonville,
JF Bastien will be presenting [a proposal](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0907r0.html)
to make C++'s `int` data type wrap around on overflow. That is, where today the expression `INT_MAX + 1`
has undefined behavior, JF would like to see that expression formally defined
to come out equal to `INT_MIN`.

I have written, but not submitted, [a "conservative" fork](https://quuxplusone.github.io/draft/twosc-conservative.html)
of JF's proposal, in which I eliminate the `INT_MAX + 1 == INT_MIN` part but leave
some of the good stuff (such as `-1 << 1 == -2`). I'm not going to talk about the
good stuff right now. (If you're not a C++ compiler writer or committee member,
you probably *assume* you're getting all of the good stuff already, and would be
surprised to learn how much of it is still undefined.)

Anyway. On a mailing list for WG21 Study Group 12, Lawrence Crowl writes
in defense of `INT_MAX + 1 ==` undefined, and I agree with him:

> If integer overflow is undefined behavior, then it is wrong.
> Tools can detect wrong programs and report them.
>
> If integer overflow is wrapping, then one never knows whether or
> not the programmer is relying on wrapper or would be surprised at
> wrapping.  No diagnostic is possible.

Another commenter in the same thread, Myriachan, gave the example of

    uint16_t x = 0xFFFF;  // 65535
    x = (x * x);

In today's C++, on "typical modern platforms" where `int` is 32 bits,
the expression `(x * x)` has undefined behavior.
This is because after [integral promotion](http://en.cppreference.com/w/cpp/language/implicit_conversion#Numeric_promotions)
promotes `uint16_t` to `int`, the result is equivalent to `(int(65535) * int(65535))`, 
and the product of 65535 with itself — that is, 4294836225 — is not representable in a signed `int`.
So we have signed integer overflow and undefined behavior.

I can think of three ways to fix this:

* Eliminate the integral promotions entirely. `x * x` becomes simply `uint16_t(4294836225)`, i.e., `uint16_t(1)`.

* Tweak the integral promotions so that they preserve signedness. `x * x` becomes `unsigned(x) * unsigned(x)`, i.e., `4294836225u`.

* Adopt something like JF Bastien's proposal to make integer overflow well-defined. `x * x` becomes well-defined and equal to `int(-131071)`.


## The "`unsigned` for value range" antipattern

Lawrence wrote back:

> So the application intended modular arithmetic?  I was concerned about
> the normal case where `unsigned` is used to constrain the value range,
> not to get modular arithmetic.

Now, in my not-so-humble opinion, if anyone is using unsigned types
"to constrain the value range," they are doing computers wrong.
That is *not* what signed versus unsigned types are for.

As Lawrence himself wrote:

> **If integer overflow is undefined behavior, then it is wrong.**
> Tools can **detect wrong programs** and report them.

The contrapositive is: "If the programmer is using a type where integer overflow
is well-defined to wrap, then we can assume that the program relies on that
wrapping behavior" — because there would otherwise be a strong incentive for the
programmer to use a type that detects and reports unintended overflow.

The original design for the STL contained the "unsigned for value range" antipattern.
Consequently, they ran into trouble immediately: for example, `std::string::find`
returns an index into the string, naturally of type `std::string::size_type`.
But `size_type` is unsigned!  So instead of returning "negative 1" to indicate
the "not found" case, they had to make it return `size_type(-1)`, a.k.a.
`std::string::npos` — which is a positive value!  This means that callers have
to write cumbersome things such as

    if (s.find('k') != std::string::npos)

where it would be more natural to write

    if (s.find('k') >= 0)

This is sort of parallel to my quotation of Lawrence above:
If every possible value in the domain of a given type is a valid output (e.g. from `find`),
then there is no value left over with which the function can signal failure at runtime.
And if every possible value in the domain is a valid input (e.g. to `malloc`),
then there is no way for the function to detect incorrect input at runtime.

If it weren't for the STL's `size_type` snafu continually muddying the waters for
new learners, I doubt people would be falling into the "unsigned for value range"
antipattern anymore.

For more information on the undesirability of "unsigned for value range"
and the general desirability of "signed `size_type`" going forward in C++,
see:

* [Scott Meyers, "Signed and Unsigned Types in Interfaces", September 1995](http://www.aristeia.com/Papers/C++ReportColumns/sep95.pdf)

* [Alf Steinbach's answer to "Why is `size_t` unsigned?" on StackOverflow](https://stackoverflow.com/questions/10168079/why-is-size-t-unsigned/)

* [GSL issue 171, "size_type should be unsigned"](https://github.com/Microsoft/GSL/issues/171)
