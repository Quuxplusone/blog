---
layout: post
title: "Don't `forward` things that aren't forwarding references"
date: 2023-05-27 00:01:00 +0000
tags:
  c++-style
excerpt: |
  Some readers of yesterday's post may wonder about the part where I wrote:

        explicit Foo(Ts... as, Ts... bs)
          : a_(static_cast<Ts&&>(as)...),
            b_(static_cast<Ts&&>(bs)...) {}

  Since we all know that `std::forward<T>(t)` is synonymous with `static_cast<T&&>(t)`,
  why did I choose `static_cast` above instead of writing `std::forward<Ts>(as)...`?
---

Some readers of yesterday's post
(["A deduction guide for `Foo(Ts..., Ts...)`"](/blog/2023/05/26/metaprogramming-halfmap/), 2023-05-26)
may wonder about the part where I wrote:

    template<class... Ts>
    class Foo {
    public:
      explicit Foo(Ts... as, Ts... bs)
        : a_(static_cast<Ts&&>(as)...),
          b_(static_cast<Ts&&>(bs)...) {}

    private:
      std::tuple<Ts...> a_;
      std::tuple<Ts...> b_;
    };

Since we all know that `std::forward<T>(t)` is synonymous with `static_cast<T&&>(t)`,
why did I choose `static_cast` above instead of writing `std::forward<Ts>(as)...`?

The answer is _not_ simply that I [prefer core-language features over library facilities](/blog/2022/10/16/prefer-core-over-library/),
and it's _not_ that I'm worried about `forward`'s [compile-time performance](/blog/2022/12/24/builtin-std-forward/).
Now, it's true that the Standard Library could get along fine without `std::forward`: Whenever
you use `std::forward<T>(t)` on a forwarding reference `T&& t`, it's always doing exactly
the same thing as `static_cast<T&&>(t)`. One could rewrite all one's `std::forward`s
as casts. But we generally don't do this, because the specific spelling `std::forward`
gives the reader useful information: "I'm forwarding this forwarding-reference parameter!"
It's greppable, and somewhat self-explanatory — much more so than a cast, anyway. We use
`std::forward` when we are, semantically, doing perfect forwarding.

If you use `std::forward` only for perfect-forwarding, then I can take your program
and mechanically search-and-replace every instance of `std::forward<T>(t)` into
`decltype(t)(t)`, and it'll still compile and do exactly the same thing. I've actually
done this for libc++, by search-and-replacing `std::forward<T>(x)` into `_LIBCPP_FWD(x)`
and then compiling with various macro-definitions of `_LIBCPP_FWD`. Of course this works
only if you never do anything "weird" via `std::forward`. If you shoehorn it in places
where it is physically correct but semantically incorrect, this mechanical transformation
won't work at all. For example, if you write:

    std::unique_ptr<int> oops() {
      auto p = std::make_unique<int>(42);
      return std::forward<std::unique_ptr<int>>(p);
    }

This `return` statement is _physically_ valid: It takes the lvalue `p`, abuses
`std::forward` to cast it to `unique_ptr&&`, and then constructs the return slot
using `unique_ptr`'s move-constructor. But it's not _semantically_ correct: We
aren't doing perfect forwarding here, so we shouldn't use `std::forward`. In fact
what we meant was simply

      return p;

In my experience, programmers new to move semantics will tend to sprinkle their
code with `std::move` and `std::forward` until it "works," ending up with a program
that doesn't look nearly as clean as it should, and works by accident, if at all.
(Programmers new to pointers will do the same with `*` and `&`.) It helps immensely
if we can give simple concrete rules for when to use `move` and `forward`, and when not to.

* Use `std::move(x)` on the last use of an object `x`, to signify
    your willingness to transfer ownership of that object's resources.
    Otherwise, don't use `move`.

* Use `std::forward<X>(x)` when perfect-forwarding a function parameter of forwarding-reference
    type `X&&`. (This, also, is a transfer of ownership, and must be the last use of `x`.)
    Otherwise, don't use `forward`.

Just as the guideline against ["pass-by-const-value"](/blog/2022/01/23/dont-const-all-the-things/#in-function-signatures-the-ugly-is-the-bad)
allows us to quickly detect that there's a bug in

    auto plus(const std::string s, const std::string& t) {
        return s + t;
    }

the guideline against "forwarding anything that's not a forwarding reference" allows us
to quickly detect that there's some kind of mistake in

    template<class T>
    void bar(std::vector<T>& c, int i) {
       foo(std::forward<T>(c[i]));
    }

(a snippet seen in a real std-proposals thread from 2018). We might not know
quite what the bug is, yet; but we know something's wrong, because the programmer
is using `forward` where no forwarding reference exists.

The flip side — the price I must pay to gain this amazing bug-finding benefit — is that
I mustn't write `forward` where no forwarding reference exists. Not even when, as in
yesterday's post, it
[contingently](https://plato.stanford.edu/entries/modality-varieties/) happens to work out
to the right physical behavior in all cases. In those situations, I avoid `forward`.

> Use `std::forward` only when perfect-forwarding.

----

See also:

* [“&thinsp;‘Universal reference’ or ‘forwarding reference’?”](/blog/2022/02/02/look-what-they-need/) (2022-02-02)
* ["Benchmarking Clang's `-fbuiltin-std-forward`"](/blog/2022/12/24/builtin-std-forward/) (2022-12-24)
