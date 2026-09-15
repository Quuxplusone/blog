---
layout: post
title: 'Fun with "deducing `this`" lambdas'
date: 2024-01-23 00:01:00 +0000
tags:
  cpplang-slack
  explicit-object-parameters
  lambdas
  name-lookup
---

Someone on [the cpplang Slack](https://cppalliance.org/slack/) asks: Why can I give
a non-capturing lambda a C++23 "deducing `this`" explicit object parameter of an
arbitrary type, but I can't do the same for a capturing lambda?

    auto alpha = [](this std::any a) { return 42; };
      // OK

    auto beta = [&](this std::any a) { return 42; };
      // ill-formed

GCC complains: "a lambda with captures may not have an explicit object parameter of an unrelated type."
(GCC's diagnostic is — I think properly — [SFINAE-friendly](https://godbolt.org/z/fK4f13343).
Clang and MSVC — I think improperly — allow you to form the lambda type, and then error on any
attempt to call it.)

This restriction exists by the following logic: A capturing lambda has captures, presumably,
because it wants to use them. To use its captures, the lambda's `operator()` must have access
to the lambda object (because its captures are stored as data members of that object). Therefore,
the `operator()` must have an object parameter of the lambda's own type — or at least a type
unambiguously derived from that type!

    auto gamma = [x]() { return x + 1; };

is lowered by the compiler into basically

    struct Gamma {
      int x_;
      auto operator()() const { return x_ + 1; }
    };
    auto gamma = Gamma{x};

It's able to get at `x_` (i.e. `this->x_`) only because it has a `this` parameter
(an "implicit object parameter") of type `Gamma`. Change the lambda to use either
of the new C++23 syntaxes which fiddle with that object parameter, and you'll find
trouble. The `static` specifier removes the object parameter entirely:

    auto delta = [x]() static { return x + 1; };
      // is ill-formed...

    struct Delta {
      int x_;
      static auto operator()() { return x_ + 1; }
        // ...because this is ill-formed!
    };

The `this` specifier fiddles with the object parameter's type, either by pinning
it down to one concrete type, or by making it a template parameter:

    auto zeta = [x](this std::any self) { return x + 1; };
      // is ill-formed...

    struct Zeta {
      int x_;
      auto operator()(this std::any self) {
        return static_cast<Zeta&>(self).x_ + 1;
          // ...because this is ill-formed!
      }
    };

    auto eta = [x](this auto self) { return x + 1; };
      // can be ill-formed...

    struct Eta {
      int x_;
      auto operator()(this auto self) {
        return static_cast<Eta&>(self).x_ + 1;
          // ...(roughly) whenever this is ill-formed!
      }
    };

_But the lowering is not the reality!_

In the above examples, I was careful to use the name `x` for the lambda's capture (in the real C++23 code)
and the name `x_` for the struct's data member (in the lowered code). That reminds us that the lowering
operation isn't a _program transformation_; you can't blithely assume that everything inside
the lambda-expression's curly braces works exactly the same as it would in an ordinary member function.
For example, the keyword `this` acts "differently" inside a lambda — which is to say, it acts the same
as it does outside the lambda. That's usually what we want.

    struct Worker {
      void run();

      void sync_run_on_this_thread() {
        this->run();  // OK
      }
      void sync_run_on_another_thread() {
        std::thread([&]() {
          this->run();  // OK
        }).join();
      }
    };

It's convenient that the two lines marked `OK` both mean the same thing. Inside a lambda, historically,
there's been no sense in letting `this->run()` mean "invoke the `run` method _of this lambda object itself_,"
because that's never been something you can do with a lambda. Starting in C++23, we can create types derived
from lambda types whose `operator()` (thanks to "deducing `this`") can actually get at the derived type.
So we can now create puzzles like this...

    return [&](this auto self) {
      printf("%d %d %d\n", x, this->x, self.x);
    };

When embedded at the line marked `HERE` in this program, the lambda above prints "1 2 3"
([Godbolt](https://godbolt.org/z/Pe1rePn6P)) — that is, each name `x` refers to a different
object.

    int one = 1;
    struct Enclosing {
      int x = 2;
      auto factory(int& x = one) {
        // HERE
      }
    };
    using Base = decltype(Enclosing().factory());
    struct Derived : Base {
      int x = 3;
      Derived(Enclosing& e) : Base(e.factory()) {}
    };

    int main() {
      auto e = Enclosing();
      auto theta = Derived(e);
      theta();
    }

Of course, this assumes you find a C++23 compiler capable of compiling this program at all!
Readers may recall Knuth's ["man or boy" test](https://www.chilton-computing.org.uk/acl/applications/algol/p006.htm)
for ALGOL 60; this program seems to be a bit like that for C++23 at the moment.
