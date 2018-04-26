---
layout: post
title: 'The Lakos Rule'
date: 2018-04-25 00:01:00 +0000
tags:
  contracts
  c++-style
  undefined-behavior
  wg21
---

The Lakos Rule is one of those STL design principles that is often brought up vaguely
during discussions, and then inevitably someone doesn't understand what it is, or knows
what it is but disagrees that the STL generally conforms to it, or whatever. Here's
a quick description of what it is and the precedent for it.

From [N3279 "Conservative use of `noexcept` in the Library"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2011/n3279.pdf) (March 2011):

> A *wide contract* for a function or operation does not specify any undefined behavior.
> Such a contract has no preconditions [...]
>
> A *narrow contract* is a contract which is not wide. Functions or operations having a narrow contract
> result in undefined behavior when called in a manner that violates the documented contract.
>
> [...]
>
> Each library function having a *wide* contract, that we agree
> cannot throw, should be marked as unconditionally `noexcept`.

The above guideline is _half_ of the Lakos Rule, as generally understood. The other, more subtle, half
of the Lakos Rule — and the half not explicitly stated in N3279 — is the converse:

- A library function having a *narrow* contract, which has undefined behavior when passed certain parameter values,
  should *not* be marked as `noexcept`.

Here's a concrete example. Suppose our library contains the following `array`-like class:

    class ArrayOfTen {
        int data[10];
    public:
        int unsafe(int i) const { return data[i]; }
        int safe(int i) const noexcept { return (0 <= i && i < 10) ? data[i] : -1; }
    };

Of course that's just an *implementation*; in WG21-world, we deal with *specifications*.

Looking at this
implementation, we can surmise that `ArrayOfTen::safe(int)` naturally ought to be specified with a *wide* contract.
No matter what `int i` you put in, our specification will tell you exactly what behavior to expect in return.
In terms of Jon Postel's [Robustness Principle](https://en.wikipedia.org/wiki/Robustness_principle), `ArrayOfTen::safe(int)`
is *liberal in what it accepts, and conservative in what it emits*.

And, looking at this
implementation, we can surmise that `ArrayOfTen::unsafe(int)` naturally ought to be specified with a *narrow* contract.
If you put in an `int i` satisfying `0 <= i && i < 10`, our specification will tell you
exactly what behavior to expect in return; whereas, if you violate that precondition, our specification
gives the library implementor the freedom to do anything they like — even to segfault the program.
In terms of the [Robustness Principle](https://en.wikipedia.org/wiki/Robustness_principle), `ArrayOfTen::unsafe(int)`
is *conservative in what it accepts*.

The Lakos Rule as stated in N3279 tells us categorically that `ArrayOfTen::safe(int)` should be marked `noexcept`.
It is a function with a wide contract, and we all agree that it cannot throw; therefore, it is `noexcept`. Easy.

The subtler half of the Lakos Rule tells us that `ArrayOfTen::unsafe(int)` should *not* be marked `noexcept`.
It is a function with a narrow contract. Thus, even though it does not seem to throw, we should *not* mark it
`noexcept`.

Why not?

Because in the precondition-violating case its behavior is undefined. The library implementor would be within
their rights to write something like this:

    class ArrayOfTen {
        int data[10];
    public:
        int unsafe(int i) const { return (0 <= i && i < 10) ? data[i] : -1; }
    };

Or even something like this:

    class ArrayOfTen {
        int data[10];
    public:
        int unsafe(int i) const {
    #ifdef PARANOID
            if (0 > i || i >= 10) throw std::out_of_range("precondition violation");
    #endif
            return data[i];
        }
    };

This is particularly nice because we can write unit tests against the `PARANOID` behavior: we can make sure
that our code that uses the `unsafe` function never accidentally violates the precondition. And then, to test
our tests, we can write some unit tests that *do* violate the precondition, and verify that when compiled with
`-DPARANOID`, they throw the expected exception.

If we mark `unsafe` as `noexcept`, then the `throw` in its body above will result in a call to `std::terminate`,
and our unit tests won't work.

So the chain of logic here is: Narrow contract — undefined behavior in some cases — maybe even `throw` in some
cases — might throw — therefore can't be `noexcept`.

We see this precedent being applied in the STL today. Here are some functions that you might
naively expect to be `noexcept` (because they do not throw), but which are actually non-`noexcept`
(because their contracts are narrow):

- `std::vector::operator[]`
- `std::array::operator[]`
- `std::optional::operator*` and `operator->`

But there is one huge exception to the Lakos Rule: *smart pointers*. The dereference operator on a
pointer type definitely has a narrow contract. The precondition is that the pointer must be non-null. But
WG21 decided that having `noexcept(*ptr) == false` would just be *too weird* to inflict on
future generations of programmers in the name of consistency.
And therefore the following handful of member functions *are* `noexcept`
even though the Lakos Rule implies that they should not be!

- `std::shared_ptr::operator*` and `operator->`
- `std::unique_ptr::operator*` and `operator->`


## Postscript: Not the Lakos Rule but still relevant

N3279 provided another guideline as well:

> If a library `swap` function, move-constructor, or move-assignment operator is conditionally-wide
> (i.e. can be proven not to throw by applying the `noexcept` operator) then it should be marked as
> conditionally `noexcept`. No other function should use a conditional `noexcept` specification.

Personally I am not a fan of this rule, but WG21 certainly does follow it.

Here are some templates that you might naively expect to be `noexcept` with certain template parameters
(because they have wide contracts and do not throw), but which are actually non-`noexcept`
(because the STL is allergic to conditional `noexcept`):

- `std::optional<int>::emplace(int)`
- `std::exchange(int&, int)`
- `std::copy` and `std::transform`
- `std::less<int>::operator()(const int&, const int&)`
- `std::less<void>::operator()(const int&, const int&)`
