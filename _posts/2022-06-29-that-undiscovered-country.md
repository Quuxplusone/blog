---
layout: post
title: '`void` versus `[[noreturn]]`'
date: 2022-06-29 00:01:00 +0000
tags:
  c++-learner-track
  cpplang-slack
  litclub
excerpt: |
  Yesterday someone on [the cpplang Slack](https://cppalliance.org/slack/) evinced confusion
  over the following code:

      int divide1(int a, int b) {
          if (b != 0) {
              return a / b;
          } else {
              throw std::runtime_error("div by zero");
          }
      }

      void throw_error() {
          throw std::runtime_error("div by zero");
      }
      int divide2(int a, int b) {
          if (b != 0) {
              return a / b;
          } else {
              throw_error();
          }
      }

  All compiler vendors [agree](https://godbolt.org/z/qj88fGqYW) that `divide2`
  should produce a warning along the lines of "control reaches end of non-void function,"
  but `divide1` is totally fine. Our newbie was confused by this behavior. In
  the case of `divide1`, we return a value in the `if` branch but not in the
  `else` branch; the compiler is fine with this, because it knows `throw` means
  "this branch doesn't return anything; it throws." In `divide2` we do the
  same thing, but just hidden inside the `throw_error` helper function. Suddenly
  the compiler is not okay with this.

  An expert advised to mark `throw_error` with the `[[noreturn]]` attribute,
  to show the compiler that `throw_error` really never returns and therefore
  it can be treated the same as a `throw`. Our newbie replied:

  > I don't understand why that should change things.
  > `throw_error`'s return type being `void` already means it never returns anything.
---

Yesterday someone on [the cpplang Slack](https://cppalliance.org/slack/) evinced confusion
over the following code:

    int divide1(int a, int b) {
        if (b != 0) {
            return a / b;
        } else {
            throw std::runtime_error("div by zero");
        }
    }

    void throw_error() {
        throw std::runtime_error("div by zero");
    }
    int divide2(int a, int b) {
        if (b != 0) {
            return a / b;
        } else {
            throw_error();
        }
    }

All compiler vendors [agree](https://godbolt.org/z/qj88fGqYW) that `divide2`
should produce a warning along the lines of "control reaches end of non-void function,"
but `divide1` is totally fine. Our newbie was confused by this behavior. In
the case of `divide1`, we return a value in the `if` branch but not in the
`else` branch; the compiler is fine with this, because it knows `throw` means
"this branch doesn't return anything; it throws." In `divide2` we do the
same thing, but just hidden inside the `throw_error` helper function. Suddenly
the compiler is not okay with this.

An expert advised to mark `throw_error` with the `[[noreturn]]` attribute,
to show the compiler that `throw_error` really never returns and therefore
it can be treated the same as a `throw`. Our newbie replied:

> I don't understand why that should change things.
> `throw_error`'s return type being `void` already means it never returns anything.

This reminded me of the classic exchange from Act III of Tom Stoppard's
[_Rosencrantz and Guildenstern Are Dead_](https://archive.org/details/rosencrantzguild0000stop_p9t9/page/79/mode/1up)
(see also [the scene from the 1990 movie version](https://www.youtube.com/watch?v=SEZvh5rVnPo&t=70s)).

> ROS.&nbsp; Do you think death could possibly be a boat?
>
> GUIL.&nbsp; No. Death is... not. Death isn't. You take my meaning?
> Death is... not-being. You can't not-be on a boat.
>
> ROS.&nbsp; I've frequently not-been on boats.
>
> GUIL.&nbsp; No, no, no. What you've done is been not-on-boats.

A function that is declared `void` is saying (by default) that it _does_ return
not-a-value. A function that is declared `[[noreturn]]` is saying (with even higher priority)
that it does not return at all — that it throws, or terminates the program, or
loops forever, or some combination of these, but that it definitely never _returns_.

The C++ versions of Rosencrantz and Guildenstern (Strousencrantz and Guildenstrup?)
might have said something like

> GUIL.&nbsp; A function can't not-return an `int`.
>
> STR.&nbsp; I frequently write functions that don't-return an `int`.
>
> GUIL.&nbsp; No, no, no. What you've written are functions that return not-an-`int`.

Mind you, this analogy is so inexact that I'd argue Guildenstrup's first statement is flatly wrong.
In C++, syntactically, as a matter of the language grammar, every function must specify a return type,
even when that function is also marked `[[noreturn]]`. So in C++, we can indeed write a function
that doesn't-return an `int`, or doesn't-return a `string`, or doesn't-return `void`:

    [[noreturn]] int f() { throw "oops"; }
    [[noreturn]] std::string g() { while (true); }
    [[noreturn]] void h() { abort(); }

And this difference is meaningful within the language, because we can observe the type of
an expression `f()` without actually executing `f()` (whereas Rosencrantz
cannot observe the on-a-boatness of himself without actually _being_ himself).

    static_assert(std::same_as<decltype(f()), int>);
    static_assert(!std::same_as<decltype(g()), int>);

Vice versa, we cannot observe the `[[noreturn]]`-ness of a function from within C++,
because attributes like `[[noreturn]]` don't participate in the type system. This was
probably a mistake, but C++ is stuck with it for the foreseeable future.
We actually ran into that issue previously on this blog; see
["Classically polymorphic `visit` replaces some uses of `dynamic_cast`"](/blog/2020/09/29/oop-visit/) (2020-09-29).

In short:

- `void f()` means "`f` returns not-a-value."

- `[[noreturn]] void g()` means "`g` does not return at all; it does something else instead of returning."

- In C++, a function's return type and its noreturn-ness are independently controllable.
    Usually there's no reason to make a `[[noreturn]]` function with a non-`void` return type,
    but in unusual cases you might need to.
