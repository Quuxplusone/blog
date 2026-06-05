---
layout: post
title: "Happy birthday, Donald Knuth! and lamp-trolls"
date: 2023-01-10 00:01:00 +0000
tags:
  algorithms
  coroutines
  knuth
---

Today (January 10th) is [Donald Knuth](https://www-cs-faculty.stanford.edu/~knuth/)'s 85th birthday.
Happy birthday, Dr. Knuth!

A few months ago I was reading Knuth's [_Selected Papers on Computer Languages_](https://amzn.to/3X1Mc9v),
which contains an interesting paper titled ["Efficient Coroutine Generation of Constrained Gray Sequences"](https://arxiv.org/pdf/cs/0404058.pdf)
written on the occasion of [Ole-Johan Dahl](https://en.wikipedia.org/wiki/Ole-Johan_Dahl)'s seventieth birthday (2001).
The paper describes an algorithm for generating Gray sequences — that is, sequences of bit-strings such that each
bit-string differs from the previous bit-string in only one position. For example, we can run through all three-bit
bit-strings in a Gray sequence as follows:

`000`, `001`, `011`, `010`, `110`, `111`, `101`, `100`

(Notice that this is also a Gray _cycle_, because the head and tail of the sequence differ by only
one bit. So you can loop around to the beginning — or, prefix another bit, toggle _that_ bit, count
back down, and ta-da, you have a four-bit Gray cycle! If this recursive construction reminds you of
the Towers of Hanoi, that's [not a coincidence](https://en.wikipedia.org/wiki/Tower_of_Hanoi#Gray-code_solution).
The above Gray sequence also represents a [Hamiltonian path](https://en.wikipedia.org/wiki/Hypercube_graph#Hamiltonicity)
among the eight corners of a unit cube, starting at $$(0,0,0)$$ = `000`.)

To generate the above Gray sequence of bit-strings, Knuth uses a "family of coroutines," which he
personifies as "an array of friendly [Norwegian] trolls."

> Each troll carries a lamp that is either off or on; he also can be either
> awake or asleep. Initially all the trolls are awake, and all their lamps are off.

A troll may be "poked." (Pokes always come from one's right neighbor.)
When an awake troll is poked, he toggles his lamp and then goes to sleep.
When a sleeping troll is poked, he wakes up and then pokes his left neighbor.
In pseudo-Algol — including Algol's 1-based indexing and parens-less function-call syntax,
but writing `yield` where Knuth wrote `return` —

    Boolean coroutine poke[k];
      while true do begin
        // the troll is now awake
        a[k] := 1 − a[k];  // toggle the lamp
        yield true;       // and we're done being poked
        // the troll is now asleep
        if k > 1 then
          yield poke[k−1] // poke neighbor, and we're done
        else
          yield false;    // we're done
      end.

We'll have a "driver" program that calls `poke[n]` in a loop, until it returns `false`:
that signifies that the leftmost troll _would_ have poked its left neighbor if it had one,
but it doesn't; which means we've finished iterating this particular Gray sequence.

I thought this would be a neat way to experiment with C++20 coroutines. Notice that Knuth's trolls
don't appear quite as well-structured as "generators" (Python `yield`, C++23 `std::generator<T>`),
but also aren't as symmetric as the coroutines used in [Knuth's elevator simulator](https://github.com/Quuxplusone/KnuthElevator)
([_TAOCP_ volume 1](https://amzn.to/3IH21hx), section 2.2.5). A troll can't go poke _any_ arbitrary
troll in line; each troll only pokes its neighbor, and the pokee's `poke` routine will eventually
"return" control to the poker (not to anybody else). In other words, we still have a stack discipline.
Each troll in the stack does maintain its own state, i.e., its lamp; but that's not unusual either —
that's just saying that the trolls are _objects_ with _member data_. The unusual thing about these
trolls, compared to your average C++ object, is that the troll's "wakefulness" state resembles _control flow_
more than _data_.


## Translated into C++14 objects

Suppose our driver function looks like this:

    class Troll;

    void driver(int n) {
        auto lamps = std::deque<bool>(n, false);
        auto trolls = std::vector<Troll>(n);
        for (int i=0; i < n; ++i) {
            trolls[i] = Troll::make(i > 0 ? &trolls[i-1] : nullptr, &lamps[i]);
        }
        while (trolls[n-1].poke()) {
            printf("Lamps are: ");
            for (bool b : lamps) {
                printf("%c", (b ? '1' : '0'));
            }
            printf("\n");
        }
    }

Then we could implement `class Troll` in C++14 as follows:

    struct Troll {
        Troll *left_neighbor = nullptr;
        bool *lamp = nullptr;
        bool is_asleep = false;
        bool poke() {
            if (!is_asleep) {
                is_asleep = !is_asleep;
                *lamp = !*lamp;
                return true;
            } else {
                is_asleep = !is_asleep;
                return (left_neighbor ? left_neighbor->poke() : false);
            }
        }
        static Troll make(Troll *t, bool *lamp) {
            return Troll{t, lamp, false};
        }
    };

But notice that we have to manage the `is_asleep` flag manually. Knuth's coroutine
specification manages that state automatically, simply by "remembering" where it left off
after each `yield`. The troll's "wakefulness" state seems to correspond more to a _position in code_
than to a piece of bits-and-bytes _data_. We might refer to it as a "suspend point," rather
than as a piece of "state" per se.

Also, please note that while this particular species of troll has only two suspend points
(so we can get away with a `bool is_asleep`), Knuth's actual paper immediately proceeds to describe
two more species of troll with six and eight suspend points, respectively; and then starts combining
and crossbreeding them in clever ways. Representing _suspend points_ as _data_ doesn't
scale very well.


## Translated into (idiosyncratic) C++20 coroutines

So let's simplify our `Troll` using C++20 coroutine syntax.

    struct Troll : TrollBase<Troll> {
        static Troll make(Troll *left_neighbor, bool *lamp) {
            while (true) {
                *lamp = !*lamp;
                co_yield true;  // and be asleep
                co_yield (next_troll ? next_troll->poke() : false);  // and be awake
            }
        }
    };

This C++20 code matches Knuth's pseudo-Algol, line for line. Of course, we cheated a little bit:
we hid some really icky stuff in that [CRTP](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#crtp)
base class `TrollBase`! Here's the icky stuff:

    template<class Derived>
    struct TrollBase {
        struct promise_type;
        using handle_t = std::coroutine_handle<promise_type>;
        struct promise_type {
            Derived get_return_object() { Derived d; d.coro_ = handle_t::from_promise(*this); return d; }
            auto initial_suspend() { return std::suspend_always(); }
            auto final_suspend() noexcept { return std::suspend_never(); }
            auto yield_value(bool b) noexcept {
                value_ = b;
                return std::suspend_always();
            }
            void unhandled_exception() {}
            bool value_ = false;
        };

        explicit TrollBase() = default;
        TrollBase(TrollBase&& rhs) noexcept : coro_(std::exchange(rhs.coro_, nullptr)) {}
        void operator=(TrollBase rhs) noexcept { std::swap(coro_, rhs.coro_); }
        ~TrollBase() { if (coro_) coro_.destroy(); }

        bool poke() {
            coro_.resume(); // should modify value_
            return coro_.promise().value_;
        }
    private:
        handle_t coro_ = nullptr;
    };

Notice that the only "data member" in this whole contraption is `handle_t coro_`.
The C++14 version's data member `is_asleep` became the coroutine's suspend point.
The data members `left_neighbor` and `lamp` became local variables
of the coroutine `Troll::make`, which means they live (not on the stack, not inside the
`Troll` object, but) inside the `Troll`'s _coroutine frame_, which is heap-allocated and
managed by the `coro_` handle. We don't think of this `Troll` as "having" that "state"
anymore; instead, the `Troll` is just a _piece of code_, a _coroutine_, which manages its
local variables in a natural way.

By the way, I don't claim that this implementation of `TrollBase` is great code. I hacked it
into shape by cannibalizing one of the `generator` implementations from [Quuxplusone/coro](https://github.com/Quuxplusone/coro).

To see the entire C++20 program in action (and some of the more complicated species of troll),
check out [Quuxplusone/KnuthElevator](https://github.com/Quuxplusone/KnuthElevator/blob/main/spiders.cpp) on GitHub.


## Translated into Python

Peter Brady did [a very clean Python version of this code](https://github.com/pbrady/KnuthElevator/blob/3ffd52729a02e0d96acbf277ff6e86d99be5de7f/trolls.py).
His version of the simple troll and its driver boils down to just this:

    def poke(k, lights):
        coro = poke(k-1, lights) if k > 1 else None
        while True:
            lights[k-1] = 1 - lights[k-1]
            yield True
            yield next(coro) if coro else False

    def driver(n):
        lights = [0] * n
        coro = poke(n, lights)
        while next(coro):
            print(*lights, sep='')

This version benefits from the structure of the troll array: each troll in this version
ends up privately _owning_ its left neighbor, as opposed to the C++ version where we have an array of
`Troll` objects each of whom holds a _non-owning pointer_ to its left neighbor. Either way is fine,
since we only ever poke the rightmost troll.


## Future directions

The trolls above produce Gray sequences that visit all $$2^n$$ $$n$$-bit bit-strings. But Knuth's goal
is to produce _constrained_ Gray sequences: sequences that visit only the bit-strings in a certain _set_,
where the set of bit-strings to be toured is defined in terms of the relations between the bits. For example, we
might say "You should visit any string $$b_0 b_1 b_2 b_3$$ where $$b_0\leq b_2$$," that is, you must visit `0101`
and `0010`, but you must never visit `1000` or `1101`. That's one example of a constraint. Knuth proves that any
_satisfiable_ set of constraints can always be represented as a totally acyclic directed graph (or "acyclic poset")
on $$n$$ nodes. Knuth gives several special-case examples (besides the special "no constraint" case we explored here)
before describing a divide-and-conquer solution to the entire problem.

Both Peter and I only went as far as Knuth's "fence digraph" special case (the species of troll he names `nudge`),
discussed on page 10 of [Knuth's paper](https://arxiv.org/pdf/cs/0404058.pdf). But the paper really takes off
around page 14, where Knuth starts describing how to compose multiple coroutines together. It would be very
interesting to see what that would look like in C++.

It would also be interesting to see a C++ solution modeled on Peter's Python solution, using C++23's `std::generator`.
Would that be more, or less, comprehensible to the average programmer than the array-of-`Troll` version?
Is there some way to preserve the array of `Troll` objects, but still implement `Troll` in terms of `std::generator`?
