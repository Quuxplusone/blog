---
layout: post
title: "Data members that want to use `size()`"
date: 2026-08-25 00:01:00 +0000
tags:
  antipatterns
  constexpr
  cpplang-slack
  library-design
  reflection
excerpt: |
  The following snippet doesn't compile:

      struct A {
        static constexpr size_t size() { return 42; }
        int data_[size()];
      };

  The problem is that the size (and therefore the type) of data member `data_`
  can't be computed until we know the value of `size()`; but evaluating `size()`
  requires `A` to be complete (<a href="https://eel.is/c++draft/class.mem#def:complete-class_context">[class.mem.general]</a>),
  which won't happen until the closing brace on the next line.

  One workaround, obviously, is to repeat the magic number `42` in two places. That's probably the
  simplest option; but here are a few other workarounds.

---

The following snippet doesn't compile:

    struct A {
      static constexpr size_t size() { return 42; }
      int data_[size()];
    };

The problem is that the size (and therefore the type) of data member `data_`
can't be computed until we know the value of `size()`; but evaluating `size()`
requires `A` to be complete (<a href="https://eel.is/c++draft/class.mem#def:complete-class_context">[class.mem.general]</a>),
which won't happen until the closing brace on the next line.

One workaround, obviously, is to repeat the magic number `42` in two places. That's probably the
simplest option; but here are a few other workarounds.

## Do the work in `data_` instead of `size()`

    struct B {
      constexpr size_t size() const { return std::size(data_); }
      int data_[42];
    };

This one matches my advice in ["The ‘array size constant’ antipattern"](/blog/2020/08/06/array-size/) (2020-08-06).
Sadly, now that `size()` depends on `data_`, we must make `size()` a non-static member function.
We can actually fix that by exploiting a quirk of the `sizeof` operator:

    struct C {
      static constexpr size_t size() { return sizeof(data_) / sizeof(data_[0]); }
      int data_[42];
    };

`std::size(data_)` treats `data_` as meaning `this->data_`, and requires a `this` pointer.
`sizeof(data_)` treats `data_` as meaning `C::data_`, and is able to tell us the size of
that field without associating it with any particular `C` object.

This approach is great when the array bound is just `42`, but it's not so great when the
array bound is computed and lengthy to spell out. In that case, we might want a different workaround.

## Introduce a static constexpr helper variable

    struct D {
      static constexpr size_t N = 42;
      static constexpr size_t size() { return N; }
      int data_[N];
    };

Notice that we can make `size()` return `N`, but we cannot flip it around and make `N`
initialized to `size()`...

    struct D_does_not_work {
      static constexpr size_t size() { return 42; }
      static constexpr size_t N = size(); // error
    };

...for the same reason as the original failure. The value of `N` must be known
_at that point_, while the class type is still incomplete and `size()` is still
not evaluable.

## Put the size computation outside the class

Obviously this is fine:

    static constexpr size_t E_size() { return 42; }
    struct E {
      static constexpr size_t size() { return E_size(); }
      int data_[E_size()];
    };

## Put `size()` in a base class

This is also fine:

    struct Base {
      static constexpr size_t size() { return 42; }
    };
    struct F : Base {
      int data_[size()];
    };

Here `size()` refers to `Base::size()`, and `Base` is a complete type. Perhaps surprisingly,
there's an order dependency here: If you declare an `F::size()` before the declaration of `data_`
then you'll be right back in case `A`, and get an error. But you can declare a new meaning for
`F::size()` _after_ the declaration of `data_`, and the compiler won't complain. (Should you do so?
No. Is it [IFNDR](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#ifndr) to do so?
Possibly; I'm not sure.)

## Turn `size` into a data member

See ["The new static constexpr `integral_constant` idiom"](https://www.foonathan.net/2023/08/static-constexpr-integral_constant/)
(Jonathan Müller, August 2023). You can actually do this:

    struct G {
      static constexpr std::integral_constant<size_t, 42> size = {};
      int data_[size()];
    };

Here `size()` is not a function call, requiring a complete `G`; it's an evaluation of the `operator()`
of a static data member, and data members (unlike member functions) are perfectly usable in
incomplete-class contexts.

This is maybe the cleanest-looking workaround out of all of these. But it seems like a bad idea
for social reasons; your coworkers will not _expect_ `size` to be a static data member instead of a
member function!

In fact, C++26 Reflection might eventually make this a bad idea for technical reasons too:
surely many Reflection-based libraries will do things like "loop over all the member functions of `G`"
without considering that some of `G`’s member functions might be hiding among its static data members.
Notably, Ryan Keane's type-erasure library [`rjk::duck`](https://github.com/RyanJK5/rjk-duck)
doesn't understand this idiom. ([Godbolt.](https://godbolt.org/z/P66co4qzr))

Another surprising detail of this workaround is the redundant “`={}`.”
Try leaving it off; the code _means_ the same thing ("default-initialize this object"),
but the compiler will no longer accept it! For historical reasons, C++ rejects any attempt
to default-initialize a `const` variable unless you put the initializer really explicitly
in the code. For some types, like `int`, this makes perfect sense:

    int x;        // OK
    const int x;  // rejected, reasonably enough

but for other types it's frankly annoying:

    std::true_type x;       // OK
    const std::true_type x; // rejected for no good reason

See ["Implementation divergence with `const int i;` and `mutable`"](/blog/2019/12/04/mutable-divergence/) (2019-12-04).

A core-language variation on this idea is to use a lambda, which incidentally removes the
redundancy:

    struct H {
      static constexpr auto size = [](){ return 42; };
      int data_[size()];
    };

The `auto` keyword cannot be used to declare non-static data members, but static data members
(as here) are fair game: this version works in C++17 and later. (Before C++17, the lambda's
`operator()` isn't constexpr, so that `size()` isn't a constant expression.)

## Conclusion

I wish there were a clear winner out of all these sub-par options.
I've placed them basically in the order I'd recommend them, starting with the
most straightforward and finishing with the one I found to have the greatest number
of identifiable disadvantages. (Which is too bad, because it's also very clever!)
