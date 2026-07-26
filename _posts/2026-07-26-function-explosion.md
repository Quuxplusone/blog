---
layout: post
title: "Interconverting `std::function` with `copyable_function`"
date: 2026-07-26 00:01:00 +0000
tags:
  benchmarks
  implementation-divergence
  pitfalls
  standard-library-trivia
  type-erasure
---

C++11 introduced `std::function` as a [type-erased](/blog/2019/03/18/what-is-type-erasure/)
callable-object holder. `function` was badly designed in a few ways: Its rarely used
["go fish" API](/blog/2019/03/27/design-space-for-std-function/#type-un-erasure) bloats
every user; it advertises `operator() const` even when the controlled object's `operator()`
is mutating; it handles what should be a precondition violation by throwing an exception,
which again bloats every user and means its `operator()` can never be noexcept.
So C++26 fixed these problems by adding
[`std::copyable_function`](https://en.cppreference.com/cpp/utility/functional/copyable_function).
(Which preserves _some_ of `function`’s
suboptimal design decisions: `copyable_function<bool()>` remains implicitly convertible
to `copyable_function<void()>`, contextually convertible to `bool`, and assignable
from `nullptr`.)

[P2548 "`copyable_function`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2548r6.pdf) (Hava 2023)
contains this guidance for implementors:

> It is recommended that implementors do not perform additional allocations when
> converting from a `copyable_function` instantiation to a compatible
> `move_only_function` instantiation, but this is left as quality-of-implementation.

Note that converting in the other direction — from `move_only_function` to
`copyable_function` — is disallowed: `copyable_function` can hold only
copyable callables, and `move_only_function` is not copyable.

But `copyable_function` is interconvertible — both directions — with `std::function`!
Both of these type-erased wrappers are constructible from anything that is copyable
and callable, and themselves _are_ copyable and callable. So we can do this:

    std::function<int()> f = []{ return 5; };
    std::copyable_function<int() const> cf = std::move(f);
    std::function<int()> g = std::move(cf);

You'd hope the move from `f` into `cf` would be implemented something like this:

![](/blog/images/2026-07-26-hopedfor.png)

and vice versa for the move back into `g`:

![](/blog/images/2026-07-26-hopedfor2.png)

But if `copyable_function` doesn't know about `function`, it might just treat `f`
the same as any old rvalue of copyable, callable type: it might move a copy of `f`
onto the heap to own.

![](/blog/images/2026-07-26-expected.png)

And vice versa for the move back into `g`: `function` might just treat `cf` the
same as any old rvalue of copyable, callable type, and move a copy of `cf` onto
the heap to own.

![](/blog/images/2026-07-26-expected2.png)

This move operation is still relatively _performant_ — it does only O(1) work
for each construction — but after a sequence of $$n$$ such constructions, we've
produced a sort of "linked list" of callables of length $$n$$. Each time we invoke
`g`, `g`’s `operator()` will call the `operator()` of its controlled `copyable_function`,
which calls the `operator()` of its controlled `function`, which calls the `operator()`
of its controlled lambda (the green object in these diagrams).

---

As of this writing (July 2026), libstdc++ is the only one of the Big Three STL vendors
to have implemented C++26 `copyable_function`; and as of this writing, they've
implemented the "bad" behavior diagrammed above.

Consider the following contrived scenario:
We have some kind of "handler callback" stored in a `std::function`. Periodically we
consider whether to modify that handler based on user input, e.g.

    using Handler = std::function<bool(char)>;

    Handler update(Handler h, bool negate) {
      if (negate) {
        h = [f = std::move(h)](char c) { return !f(c); };
      }
      return h;
    }

    ~~~~
    Handler h = original_handler;
    while (~~~~) {
      ~~~~
      h = update(std::move(h), (input == '!'));
    }

Observe that `h` is moved into `update`, and moved out again (thanks to
[implicit move](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2266r3.html#wording)).
This uses the move constructor of `std::function`, which, like most move constructors,
is quick and non-allocating.

But suppose one of our coworkers updates part of this code to C++26,
where we might reasonably prefer to use `std::copyable_function` in place of
`std::function`.<sup><a href="#footnote-should-you-abandon-func">[1]</a></sup>
If our coworker updates the whole codebase to use `copyable_function` consistently,
there's no problem. But what if they update only their part, so that their new
`copyable_function`-based code must interoperate with everyone else's old
`function`-based code? Then we get something like this:

    using OldHandler = std::function<bool(char)>;
    using NewHandler = std::copyable_function<bool(char)>;

    OldHandler update(OldHandler h, bool negate);
      // unchanged, for the benefit of pre-C++26 callers

    ~~~~
    NewHandler h = original_handler;
    while (~~~~) {
      ~~~~
      h = update(std::move(h), (input == '!'));
    }

This triggers that "linked list" scenario, and `h`’s call operator gets slower
with every round-trip through `update` — even when `negate` is `false`!

I wrote up this little benchmark ([Godbolt](https://godbolt.org/z/zG9WW6bdK)):

    #include <chrono>
    #include <cstdio>
    #include <functional>

    size_t now() {
      static const auto epoch = std::chrono::high_resolution_clock::now();
      auto tp = std::chrono::high_resolution_clock::now();
      return std::chrono::duration_cast<std::chrono::microseconds>(tp - epoch).count();
    }

    void print_elapsed_time(int i, size_t call_started) {
      printf("Invocation on the %dth iteration took %zu us\n", i, now() - call_started);
    }

    using OldHandler = std::function<void(int, size_t)>;
    using NewHandler = std::copyable_function<void(int, size_t) const>;

    OldHandler update(OldHandler h, bool negate) {
      if (negate) { /* do something here, but for this example we don't care what */ }
      return h;
    }

    int main() {
      NewHandler handler = print_elapsed_time;
      for (int i = 0; i < 160'000; ++i) {
        handler = update(std::move(handler), false);
        if (i % 10'000 == 0) {
          handler(i, now());
        }
      }
    }

Run this program on libstdc++ (as of GCC 16) and you'll see output something like
the following:

    Invocation on the 0th iteration took 0 us
    Invocation on the 10000th iteration took 180 us
    Invocation on the 20000th iteration took 296 us
    Invocation on the 30000th iteration took 365 us
    [...]
    Invocation on the 130000th iteration took 1057 us
    Invocation on the 140000th iteration took 1108 us
    Invocation on the 150000th iteration took 1202 us

Each time we invoke `handler(i, now())` the "linked list of callables" hanging off
`handler` is 10,000 elements longer than before, and takes about 100 microseconds
longer to chase those pointers. Of course it also holds onto many kilobytes more
memory than needed, too. This is a "logical memory leak": that memory
is technically still accessible and will be freed by `h`’s destructor, but as long
as `h` lives we'll see our RAM usage creep higher and higher over time, without
any change in the "logical" state of our program.

---

As of July 2026, Microsoft STL makes `move_only_function` and `function` interoperate
without such "logical memory leaks," and I bet their `copyable_function` will also
avoid them, once they implement that type. libstdc++'s implementation may improve
in the future. Meanwhile, libc++ hasn't implemented either `move_only_function` or
`copyable_function` yet, so they still have plenty of time to think about this issue.

---

Beware of a simpler version of this pitfall, by the way, when writing your own type-erased
types. Make sure that when you copy a `Printable`-holding-an-`int`, you get another
`Printable`-holding-an-`int` and not a `Printable`-holding-a-`Printable`-holding-an-`int`.
Write unit tests to make sure that still holds when constructing from a `Printable&`;
from a `const Printable&`; from a `Printable&&`; and from a `const Printable&&`.
Solving this problem gets more painful if, like the STL, you have more than one
type-erased type that need to play well together. Try to avoid getting into that
situation if you can; but be aware of this pitfall if you must.

---

Footnote: _Should_ you abandon `function` for `copyable_function`
in C++26? Some would say yes. Personally I would say that you ought to abandon both,
and write your own type-erased callable instead. It takes less than 100 lines!

In completely green-field C++26 code, sure, I'd recommend `copyable_function` over
`function` (once your vendor implements it). I would _not_ recommend mixing both
types in the same brown-field codebase; I'd pick one and stick with it, while working
toward a point where you could update the entire codebase at once.
