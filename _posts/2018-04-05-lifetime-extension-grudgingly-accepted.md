---
layout: post
title: 'On lifetime extension and `decltype(auto)`'
date: 2018-04-05 00:02:00 +0000
tags:
  c++-style
  move-semantics
---

"Lifetime extension" is a weird little quirk of C++98 that is still alive and
kicking in C++17. The basic idea is that you can write C++ code like this:

    int main() {
        const int& three = 1 + 2;
        return three;
    }

and it will just magically work. Sure, it *looks* like you're taking a reference
to a temporary, there; but actually behind the scenes the compiler will _extend
the lifetime_ of that temporary to match the scope of the reference variable
to which it's bound (namely, `three`).

This magic happens only when the compiler can see that you're dealing with a
temporary, a _prvalue_ expression, of type exactly `T`. The magic will *not* happen if
you are dealing with an expression of type `T&&`, or `const T&`, or anything else
that might conceivably be an _xvalue_. Thus:

    int foo(const int& x) { return x; }
    int a        = foo(42);  // OK
    const int& b = foo(42);  // OK

    const int& bar(const int& y) { return y; }
    int c        = bar(42);  // OK
    const int& d = bar(42);  // ERROR!

Because of the `ERROR!` on this final line, I've always considered lifetime extension
to be a pretty dangerous and arcane feature of C++, basically a bug in C++98 that
now we can't ever get rid of for historical reasons — and if you find yourself
using it, you are probably doing something wrong.

Similarly, I've always felt that the `decltype(auto)` return type is basically
dangerous and you should avoid using it in "normal" code. (Maybe it has a use
in some arcane metaprogramming. I don't know.)

However, the other day, while working on my [Hanabi bots](/blog/2018/03/29/hat-guessing-in-hanabi/),
I did come across a thought-provoking situation.

    class Server {
        std::vector<Card> discards_;
    public:
        const auto& discards() const {
            return discards_;
        }
    };

    class GameView {
        const Server *server_;
    public:
        const auto& discards() const {
            return server_->discards();
        }
    };

    void do_stuff(GameView view) {
        const auto& d = view.discards();
        // ...
    }

Now let's suppose that we replace this slow and heap-happy `std::vector<Card>`
with something small and fast and trivially copyable, like my bot's `CardCounts`
type.

    class Server {
        CardCounts discards_;
    public:
        const auto& discards() const {
            return discards_;
        }
    };

We don't need to change any of the rest of the code.
But then, why are we passing around references to trivially copyable (and small)
types? Let's just do value semantics right!

    class Server {
        CardCounts discards_;
    public:
        auto discards() const {
            return discards_;
        }
    };

...And now we have a dangling-reference bug! `GameView::discards()` returns
a *const reference* to the result of `server_->discards()`, which is a temporary,
which means that that reference is dangling as soon as the function returns.
This is horrible enough that both Clang and GCC will warn us about it.

We can adjust `GameView::discards()` to return by-value... but then it'll
do something inefficient (make a copy) if we ever change `Server::discards()`
*back* to returning by const reference.

So this might be the fabled use-case for `decltype(auto)`:

    class GameView {
        const Server *server_;
    public:
        decltype(auto) discards() const {
            return server_->discards();
        }
    };

This returns by-value (and does copy elision) if `server_->discards()` returns by-value;
and it returns by-const-reference if `server_->discards()` returns by-const-reference.
Nifty!

And the kicker: Notice that when `view.discards()` returns by-value,
our client code in `do_stuff` *magically continues to work* — because of
lifetime extension!

    void do_stuff(GameView view) {
        const auto& d = view.discards();
        // ...
    }

However, it's true that we could replace `const auto&` with `decltype(auto)` here
as well, and then we wouldn't be relying on lifetime extension anymore.

    int a = 42;
    const int& foo() { return a; }

    int main() {
        decltype(auto) b = foo();
        assert( &a == &b );
    }

Does this mean you should use `decltype(auto)` in your own code? Probably not.

Does this mean you should love lifetime extension? Probably not.

But it made me think that maybe there's an argument to be made in favor of
both of them, sometimes. They're no longer *100%* terrible in my eyes.
Just, like, 99% terrible.
