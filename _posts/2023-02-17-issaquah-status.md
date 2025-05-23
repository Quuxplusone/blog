---
layout: post
title: "How my papers did at Issaquah"
date: 2023-02-17 00:01:00 +0000
tags:
  implementation-divergence
  initializer-list
  llvm
  relocatability
  sg14
  wg21
excerpt: |
  The ISO C++ Committee met in Issaquah, Washington, last week.
  I currently have three papers in the pipeline:

  - [P2752 "Static storage for braced initializers"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2752r3.html)
  - [P2596 "Improve `std::hive::reshape`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2596r0.html)
  - [P1144 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)

  How did they fare?
---

The ISO C++ Committee met in Issaquah, Washington, last week.
I currently have three papers in the pipeline:

- [P2752 "Static storage for braced initializers"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2752r3.html)
- [P2596 "Improve `std::hive::reshape`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2596r0.html)
- [P1144 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)

How did they fare?

## Static storage for braced initializers

[P2752 "Static storage for braced initializers"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2752r3.html)
was seen by the EWG Incubator on Friday night, and passed on to EWG uneventfully, as expected.
Since then, its proposed wording has been slightly wordsmithed (thanks, Jens Maurer!) so that hopefully
it'll be ready for C++26 at the next meeting. If it is accepted, you'll be able to write ([Godbolt](https://godbolt.org/z/19cs5ob9j))

    int f(std::initializer_list<int>);
    int test() {
        return f({1,2,3,4,5});
    }

and have the compiler (1) use no stack space for that function, and (2) make the call to `f` a tail-call.
Right now, due to an oversight in the C++11 rules for `initializer_list`, this doesn't work.

As you can see [on Godbolt](https://godbolt.org/z/19cs5ob9j), I have a prototype implementation of P2752
under the Clang command-line flag `-fstatic-init-lists`.

You might further ask, "I thought the vocabulary type for this kind of thing, since C++20, was `span`.
Why does the following snippet not even compile yet?" ([Godbolt.](https://godbolt.org/z/vajbh8Tzq))

    int g(std::span<const int>);
    int test() {
        return g({1,2,3,4,5});
    }

That problem would be fixed by the adoption of Federico Kircheis'
[P2447R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2447r2.html),
which you may remember from
["`std::span` should have a converting constructor from `initializer_list`"](/blog/2021/10/03/p2447-span-from-initializer-list/) (2021-10-03).
I don't believe there was any movement on P2447 at Issaquah; in fact I've heard that it is dead (perhaps because
it presented as a _change to C++26_ instead of as a _defect against C++20_); but I would love to see it revived.

### GCC `-fmerge-all-constants`

UPDATE, 2023-02-25: Kevin Zheng informs me that GCC trunk already has an even more powerful flag,
`-fmerge-all-constants`. That flag does things the Standard will never in a million years condone, such as
[merging the constant arrays](https://godbolt.org/z/9zc18zs4M) in

    void h(const int*, const int*);
    void test() {
        const int a1[] = {1,2,3,4,5};
        const int a2[] = {1,2,3,4,5};
        h(a1, a2);
    }

GCC's `-fmerge-all-constants` produces optimal codegen for `initializer_list` — even more optimal than
my quick-and-dirty prototype of `-fstatic-init-lists`! — but at the cost of non-conforming codegen for
the above.

Clang also has `-fmerge-all-constants`, but for historical reasons it is much less powerful. Historically,
`-fmerge-all-constants` was Clang's _default_ behavior. So it was watered down to avoid many of
the [obvious dangers](https://stackoverflow.com/questions/70219427/what-may-be-the-problem-with-clangs-fmerge-all-constants);
for example, it would never merge `a1` and `a2` above. Still,
people had been complaining about the non-conforming default [since at least 2014](https://bugs.llvm.org/show_bug.cgi?id=18538);
and they finally turned it off entirely in Clang 6.0.1 (July 2018). Passing `-fmerge-all-constants` will
still re-enable the non-conforming behavior — but only in the watered-down, almost-conforming state that
it was in when it was turned off. Clang has nothing like GCC's full-strength behavior.

## Improving `std::hive`

[P2596 "Improve `std::hive::reshape`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2596r0.html)
proposes simplifications to [P0447R21 `std::hive`](https://isocpp.org/files/papers/P0447R21.html) that
would give vendors more freedom to optimize, shrink its memory footprint, and make the `hive::splice` member
function `noexcept`. P2596 was seen by LEWG on Tuesday, with the following straw-poll results:

|---------------------------------------------------|---|---|---|---|---|
| Do we _ever_ want to see P0447 `std::hive` again? | 8 | 4 | 5 | 2 | 3 |
| Do we want to see P0447 `std::hive` again with P2596's changes applied? | 2 | 3 | 8 | 4 | 3 |

Committee chairs are fond of describing straw-poll results as "tea leaves,"
and (in order to self-fulfill that prophecy) seem to delight in crafting polls that
convey as little concrete information as possible. One way to read this pair
of polls is that the combined P0447+P2596 had far less support than the status quo:
12–5–5 in favor of P0447, 5–8–7 in favor of P0447+P2596 specifically. Another way to read
the polls is that _when you subtract out the five people in the room who never want
to see P0447 again in any form whatsoever,_ the room is 5–8–2 in favor of applying
P2596's changes.

I've made a "not-really-production-quality" implementation of P0447 `std::hive`
available [on Godbolt](https://godbolt.org/z/E3zdG5GPd). The P2596 API is available
under the macro `-D_LIBCPP_P2596`.

## Trivial relocatability

[P1144 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)
has a new name! I've decided that its former name, "Object relocation in terms of move plus destroy,"
was a little too much of a mouthful; and everyone knows the paper as "Trivially Relocatable" anyway.

There was big news on the trivial-relocation front at Issaquah: Bloomberg has entered the fray with
a paper coauthored by Mungo Gill and Alisdair Meredith, [P2786R0 "Trivial relocatability options."](https://isocpp.org/files/papers/P2786R0.pdf)
This paper wasn't finished in time for the pre-Issaquah mailing and was continually revised all week leading up
to Friday night's discussion; it [obeys Pascal's dictum](https://quoteinvestigator.com/2012/04/28/shorter-letter/),
weighing in at 30 pages. By comparison, P1144R6 is 17 pages; my [draft of P1144R7](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)
is down to 15 pages so far, and I'm trying hard to shorten it.

The major places where P2786R0 disagreed with P1144 are:

- Whether `std::swap` can be implemented as "relocate A into a temporary; relocate B into A;
    relocate the temporary into B," or whether it _must_ be implemented in terms of assignment —
    and what "assignment" means for trivial types anyway.
    Vendors already [optimize `copy_n`](https://godbolt.org/z/8nMoo6Tes) in a way detectable by the user;
    the question is if we should permit vendors to [optimize `swap`](https://godbolt.org/z/Y5YMndM56) in the same way.
    P1144 says "optimize," P2786 says "don't."

- Whether `vector::insert` can use relocation to "make a window" that
    is then filled with the new data ([as in Folly's `fbvector`](https://github.com/facebook/folly/blob/1d4690d0a3/folly/FBVector.h#L1273-L1292)),
    or whether it _must_ be implemented in terms of assignment.
    P1144 says "optimize," P2786 says "don't."
    See ["Should assignment affect `is_trivially_relocatable`?"](/blog/2024/01/02/bsl-vector-erase/) (2024-01-02).

- Whether a type containing a data member of type `boost::interprocess::offset_ptr<T>` may be
    explicitly warranted as `[[trivially_relocatable]]` ([Godbolt](https://godbolt.org/z/hz56dqq4Y)).
    P1144 says "optimize," P2786 says "don't."

EWGI straw-polled only this third bullet point, and as a three-way poll rather than the usual five-way;
the results were a dismal 7–5–6 in favor of optimizing.

But there is one major point where P2786R0's disagreement with P1144R6 was wise and good!

- Whether the `std::uninitialized_relocate` algorithm should do the Slower But Right Thing when the source and destination
    ranges overlap, as `std::memmove` does; or whether it should do the Fast But Wrong Thing,
    as `std::memcpy` and `std::copy` do. ([Godbolt.](https://godbolt.org/z/h46qYMWM4))
    P1144 says "fast but wrong," P2786 says "slower but right."

The trouble is, you can really only do the Right Thing on overlap if you can _detect_ overlap,
and that's possible only if you are given contiguous iterators. Qt defines two different algorithms
here: [`q_uninitialized_relocate_n`](https://github.com/qt/qtbase/blob/26fec96/src/corelib/tools/qcontainertools_impl.h#L71-L85)
whose trivial path can use `memcpy` (EDIT: [now it does](https://github.com/qt/qtbase/commit/4dbd97c8f990e8ed5714cc2a599446920670e78d)),
and [`q_relocate_overlap_n`](https://github.com/qt/qtbase/blob/26fec96/src/corelib/tools/qcontainertools_impl.h#L203-L233)
whose trivial path must use `memmove`. Should the standard library do the same?

In the next mailing, you can expect to see some changes in P1144R7 — beyond just shortening the darn thing! — and
probably a new joint paper coauthored by Alisdair Meredith and myself that expands further on the specific
design-decision bullet points above (and maybe some others, too).

Expect further blog posts in the next few weeks about trivial relocatability. If you're
champing at the bit, see my existing posts tagged [#relocatability](/blog/tags/#relocatability),
and Dana Jansens' blog post ["Trivially Relocatable Types in C++/Subspace"](https://danakj.github.io/2023/01/15/trivially-relocatable.html) (January 2023).
(EDIT: Here are the promised posts:
[I](/blog/2023/02/24/trivial-swap-x-prize/),
[II](/blog/2023/03/03/relocate-algorithm-design/),
[III](/blog/2023/03/10/sharp-knife-dull-knife/).)
