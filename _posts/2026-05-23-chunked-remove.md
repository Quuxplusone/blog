---
layout: post
title: "Does bulk memmove speed up `std::remove_if`? (No.)"
date: 2026-05-23 00:01:00 +0000
tags:
  benchmarks
  stl-classic
  triviality
---

This morning I was reading the umpteenth std-proposals thread proposing some variety of
[`unstable_remove`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0041r0.html)
and it occurred to me that one odd thing about a swap-and-pop-based `unstable_remove`
is that it tends to replace large swaths of contiguous removals by _reversing_ the
elements that are kept. For example ([Godbolt](https://godbolt.org/z/f9doebhch)):

    template<class BidirIt, class Pred>
    BidirIt unstable_remove_if(BidirIt first, BidirIt last, Pred pred) {
      while (true) {
        first = std::find_if(first, last, pred);
        if (first == last) return first;
        while (true) {
          --last;
          if (first == last) return first;
          if (!pred(*last)) break;
        }
        *first++ = std::move(*last);
      }
    }

    int main() {
      auto in234 = [](int x) { return 2 <= x && x <= 4; };
      std::vector<int> v = {1,2,3,4,5,6,7,8,9};
      v.erase(unstable_remove_if(v.begin(), v.end(), in234), v.end());
      // v is {1,9,8,7,5,6}
    }

It would be more aesthetically pleasing to produce `{1,7,8,9,5,6}`: for trivially copyable
elements, this would be a single memmove. In a sense, "reversing the elements" is extra work
that we might save time by avoiding. But to avoid _that_ work, we might have to do _even more_
work in the form of bookkeeping. I haven't attempted to benchmark any such change to
`unstable_remove`.

But the same aesthetic consideration applies to the ordinary, stable `std::remove_if`.
Traditionally it's a loop over `operator=`, like this:

    template <class FwdIt, class Pred>
    FwdIt smooth_remove_if(FwdIt first, FwdIt last, Pred pred) {
      FwdIt dfirst = std::find_if(first, last, pred);
      if (dfirst != last) {
        for (first = std::next(dfirst); first != last; ++first) {
          if (!pred(*first)) {
            *dfirst++ = std::move(*first);
          }
        }
      }
      return dfirst;
    }

But what if we "chunked" the writes to `*dfirst`, like this?

    template <class FwdIt, class Pred>
    FwdIt chunky_remove_if(FwdIt first, FwdIt last, Pred pred) {
      FwdIt dfirst = std::find_if(first, last, pred);
      if (dfirst != last) {
        for (first = std::next(dfirst); first != last; ++first) {
          if (!pred(*first)) {
            FwdIt sfirst = first;
            first = std::find_if(std::next(first), last, pred);
            dfirst = std::move(sfirst, first, dfirst);
            if (first == last) break;
          }
        }
      }
      return dfirst;
    }

Delegating the work to the [`std::move`](https://en.cppreference.com/cpp/algorithm/move) algorithm
allows the library to use memmove for that work, if the element type happens to be trivially
copyable. The nested loop complicates the generated code ([Godbolt](https://godbolt.org/z/ja48jh4nv)),
but is it worth it? Do we actually save CPU cycles?

Well, if the average length of a "run" of survivors is long, then we
might expect the cost of a single large memmove to beat out the cost of `operator=`-in-a-loop.
But at the other extreme, if the average length of a run is only 1 element, then memmove won't
save anything; we'll be paying all the bookkeeping cost for none of the benefit.

> By "bookkeeping" I mean not only the extra register and icache pressure of the more complicated
> algorithm, but also the overhead of the call to memmove itself, and that memmove will necessarily
> start by checking whether it needs to handle forward overlap. (That branch is completely predictable
> — it doesn't — but is still overhead absent from the "smooth" version.)

I wrote a little benchmark ([backup](/blog/code/2026-05-23-chunked-remove-benchmark.cpp))
to time a call to `std::remove_if` on an array of a million ints,
with a bit-masking predicate that removed half of the elements; one in 8;
one in 128; or one in 1024. I contrived this benchmark deliberately to play
to the "chunky" algorithm's strengths: trivially copyable elements, with large swaths moved
contiguously.

The "smooth" column here is merely to prove that my `smooth_remove_if` implementation
is essentially the same as libc++'s default implementation:
my `chunky_remove_if` can't blame its defeat on "library vendor magic."
Yet it _is_ defeated, and decisively so:

| Elements removed |   `std` |  smooth |  chunky | Penalty | smooth<br>assignments | chunky<br>memmoves |
|:-----------------|--------:|--------:|--------:|--------:|----------------------:|-------------------:|
| 1 in 2           | 3122 us | 3157 us | 3797 us |    +20% |                499549 |             249971 |
| 1 in 8           | 1082 us | 1105 us | 1471 us |    +33% |                874707 |             109724 |
| 1 in 128         |  416 us |  420 us |  468 us |    +11% |                992148 |               7750 |
| 1 in 1024        |  327 us |  326 us |  396 us |    +21% |                998482 |                958 |
{:.smaller}

As expected, `chunky_remove_if` loses when the expected length of a memmove is short.
But it also loses when the expected length of a memmove is long! And, counterintuitively,
as we remove _fewer_ elements (and thus execute _more_ individual assignments in the "smooth"
case and _fewer_ individual memmoves in the "chunky" case), `smooth_remove_if` gets faster and
faster, and `chunky_remove_if`’s performance penalty doesn't budge.

I tentatively blame this on the branch predictor. The fewer elements we remove, the more
predictable the result of `pred(*first)`. Removing half the elements is the worst case for
`smooth_remove_if` simply because that turns it into a tight loop over a single completely
unpredictable branch. Making that branch more predictable dominates every other consideration,
including any speedup we might get from memmove (which, after all, is also a tight loop over
a mostly predictable branch: "have we counted all the way to `n` yet?")

Conclusion: "Chunking" the elements moved by `remove_if` into larger memmoves
isn't an optimization; it's a pessimization.

This doesn't directly say anything about `unstable_remove`, but it does suggest that
when it comes to benchmarking `remove`-related algorithms, we ought not to rabbit-hole on
_simply_ minimizing the number of move-assignments; that kind of thing may be outweighed
by cache and branch-prediction effects.
