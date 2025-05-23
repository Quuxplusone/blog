---
layout: post
title: "STL algorithms for trivial relocation"
date: 2023-03-03 00:01:00 +0000
tags:
  library-design
  relocatability
  stl-classic
  wg21
excerpt: |
  This is the second in a series of at least three weekly blog posts. Each post will explain
  one of the problems facing [P1144 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)
  and [P2786R0 "Trivial relocatability options"](https://isocpp.org/files/papers/P2786R0.pdf) as
  we try to (1) resolve their technical differences and (2) convince the rest of the C++ Committee
  that these resolutions are actually okay to ship.

  Today's topic is new library algorithms for relocation.
---

This is the second in a series of at least three weekly blog posts. Each post
([I](/blog/2023/02/24/trivial-swap-x-prize/),
[II](/blog/2023/03/03/relocate-algorithm-design/),
[III](/blog/2023/03/10/sharp-knife-dull-knife/),
[IV](/blog/2023/06/03/p1144-pmr-koans/))
will explain
one of the problems facing [P1144 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)
and [P2786R0 "Trivial relocatability options"](https://isocpp.org/files/papers/P2786R0.pdf) as
we try to (1) resolve their technical differences and (2) convince the rest of the C++ Committee
that these resolutions are actually okay to ship.

Today's topic is new library algorithms for relocation.


## What is relocation used for, in practice?

Relocation (trivial or otherwise) tends to be used in the following ways:

- On single objects that are (at least conceptually) member subobjects;
    e.g. the move-constructors of
    [SBO](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#sbo-soo-sso)-optimized
    `any` and `function`

- On non-overlapping arrays of objects; e.g. `Vector` reallocation and the move-constructor of `static_vector`

- On an array that overlaps itself; e.g. `vector::insert` (which needs to shift elements rightward
    before inserting the new element) and `vector::erase` (which needs to shift elements leftward
    after destroying the old element)

> You might say, "Wait, I don't think that's how `vector::erase` works!"
> For ordinary (non-trivial) types, you're right: erasure works by move-assigning
> elements to the left, and then destroying the last (moved-from) element.
> But for trivially copyable types, this is tantamount to `memmove`; it
> is also tantamount to `memmove` for trivially relocatable types.
> Facebook's `folly::fbvector` [already works that way](https://github.com/facebook/folly/blob/1d4690d0a3/folly/FBVector.h#L1273-L1292)
> for types that are `folly::IsRelocatable`,
> and Bloomberg's `bsl::vector`
> [does too](https://github.com/bloomberg/bde/blob/e15f05be6/groups/bsl/bslalg/bslalg_arrayprimitives.h#L3769-L3800)
> for types that are `bslmf::IsBitwiseMoveable`.

Finally, a use-case that remains hypothetical as far as I know:

- On a single object, producing a return value. This is the idea I explored two days ago in
    ["Making `priority_queue` and `flat_set` work with move-only types"](/blog/2023/03/01/priority-queue-displace-top/) (2023-03-01);
    it's used inside [`vector::displace_back`](https://github.com/Quuxplusone/llvm-project/commit/03d304c0977c5bed476d60132c7cf6c8ffe6f9c4#diff-f1f43ad1a94f42808135ffd81fbd791af1fcc8568e5801ce28b38d2d18c4d904R683-R688),
    although it doesn't _need_ to be.

Now, the "implementor" (the STL vendor) doesn't need any help to implement these use-cases.
The implementor can use their intimacy with the compiler to do things that are, on paper,
undefined behavior; or they can rely on non-portable compiler intrinsics.
But when _you_, the ordinary programmer of something like BSL or Folly or Qt, want to implement
your own `Vector`, `Function`, or `StaticVector` type, it'll help if the STL provides
portable APIs that you can use in each of these cases.

### Can't I just use `memmove`?

In practice, yes, that's the state of the art in BSL, Folly, and Qt.
But on paper, no, it's undefined behavior for at least two reasons.

- If I have two objects, it's defined behavior to `memmove` between them (and then inspect
    the value of the destination object) only if the object's type is _trivially copyable_
    ([[basic.types.general]/3](https://eel.is/c++draft/basic.types#general-3)).

- If I have only one object, and `memmove` from it into some storage where there is
    no object yet, and then _treat_ that storage as if it now held an object, that's
    defined behavior only if the object's type is _implicit-lifetime_
    ([[intro.object]/10](https://eel.is/c++draft/intro.object#10.sentence-2)).
    (The storage must also have been properly acquired, but that part is hard to screw up
    in practice. See [[intro.object]/13](https://eel.is/c++draft/intro.object#13).)

Both [P1144](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)
and [P2786](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2786r0.pdf)
agree that it's important for the STL to provide a public API that relocates
objects (trivially or otherwise) in a well-defined way. Neither paper is trying to
"bless" the current use of raw `memmove` as seen in BSL, Folly, and Qt; we're trying
to provide a blessed alternative.


## Overview of existing algorithms

Three major libraries provide "prior art" for P1144: BSL, Folly, and Qt.
We can imagine what the STL API should look like, by looking at what these libraries
define for their own internal purposes.

### Folly

Folly provides no generic algorithms, as far as I can tell. `fbvector` defines
a member function [`M_relocate`](https://github.com/facebook/folly/blob/1d4690d0a3/folly/FBVector.h#L624-L628)
which calls `relocate_move(T* d_first, T* first, T* last)`
followed by `relocate_done(T*, T* first, T* last)`. `relocate_move` calls either
[`D_uninitialized_move_a`](https://github.com/facebook/folly/blob/1d4690d0a3/folly/FBVector.h#L486-L504) or `memcpy`;
`relocate_done` calls either [`D_destroy_range_a`](https://github.com/facebook/folly/blob/1d4690d0a3/folly/FBVector.h#L353-L368)
or nothing.

That same division of labor is seen [in libc++'s `vector`](https://github.com/llvm/llvm-project/commit/9e6440cd3d9abc57104b5fcca8d04c20ba7d5067#diff-f1f43ad1a94f42808135ffd81fbd791af1fcc8568e5801ce28b38d2d18c4d904R954-R960),
by the way.

### Qt

Qt's approach to relocation semantics is the cleanest.
It provides these two algorithms:

    template<class T, class N>
      void q_uninitialized_relocate_n(T* first, N n, T* d_first);

    template<class T, class N>
      void q_relocate_overlap_n(T* first, N n, T* d_first)

[`q_uninitialized_relocate_n`](https://github.com/qt/qtbase/blob/147dd6e/src/corelib/tools/qcontainertools_impl.h#L71-L85)
relocates a non-overlapping array. This is the building block for `Vector::reserve` and `StaticVector`.
Qt's version of `static_vector` is called `QVarLengthArray`, and
[its move constructor](https://github.com/qt/qtbase/blob/fbfee2d/src/corelib/tools/qvarlengtharray.h#L312-L328)
relocates using `q_uninitialized_relocate_n`.

[`q_relocate_overlap_n`](https://github.com/qt/qtbase/blob/147dd6e/src/corelib/tools/qcontainertools_impl.h#L223-L253)
relocates an overlapping array left or right, handling overlap "by magic" just like `memmove` does.
This could be the building block for `Vector::insert` _and_ `Vector::erase`.
However, as of this post, Qt's `QList::erase` uses [`QMovableArrayOps::erase`](https://github.com/qt/qtbase/blob/fbfee2d/src/corelib/tools/qarraydataops.h#L842-L863)
which doesn't use `q_relocate_overlap_n`; it uses raw `memmove` instead.

> The naming roughly reflects the analogies that Qt's authors see with the STL:
> `q_uninitialized_relocate_n`, like `std::uninitialized_copy`, writes into uninitialized memory,
> and like `std::copy_n` does not document whether the data is copied front-to-back or back-to-front.
> `q_relocate_overlap_n` writes into a destination buffer that might initially hold objects,
> so its name doesn't involve the word `uninitialized`.

Qt doesn't provide a single-argument relocation function. Its equivalent of `std::any`,
named `QVariant`, is SBO-optimized but simply refuses to store anything non-trivially relocatable
in its SBO buffer. So [`QVariant`'s move constructor](https://github.com/qt/qtbase/blob/fbfee2d/src/corelib/kernel/qvariant.h#L276-L277)
can simply copy the bits.

### BSL

BSL has the largest zoo of helper algorithms by far:
["bslalg_arrayprimitives.h"](https://github.com/bloomberg/bde/blob/e15f05be6/groups/bsl/bslalg/bslalg_arrayprimitives.h)
is 4800 lines long. But BSL has only one primitive relocation algorithm.

    template<class T, class Alloc>
      void ArrayPrimitives_Imp::destructiveMove(
               T* d_first, T* first, T* last, Alloc a);

[`destructiveMove`](https://github.com/bloomberg/bde/blob/e15f05be6/groups/bsl/bslalg/bslalg_arrayprimitives.h#L1519-L1537)
relocates a non-overlapping array, just like `q_uninitialized_relocate_n`;
this is the building block for `Vector::reserve` and `StaticVector`.
In fact, BSL's documentation [shows an example](https://github.com/bloomberg/bde/blob/e15f05be6/groups/bsl/bslalg/bslalg_arrayprimitives.h#L231-L258)
of using this algorithm in `vector::reserve`.

BSL has some other helpers, but (as in Folly) they divide the labor in ways unclearly related to relocation.
For example, `bsl::vector::erase` calls
[`ArrayPrimitives::erase`](https://github.com/bloomberg/bde/blob/e15f05be6/groups/bsl/bslalg/bslalg_arrayprimitives.h#L77-L81),
which calls
[`ArrayPrimitives_Imp::erase`](https://github.com/bloomberg/bde/blob/e15f05be6/groups/bsl/bslalg/bslalg_arrayprimitives.h#L3769-L3800),
which uses `memmove` to do the trivial relocation. (It can't use BSL's own `destructiveMove`,
because `destructiveMove` works only on non-overlapping arrays.)

### P2786R0

Bloomberg's [P2786R0 "Trivial relocatability options"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2786r0.pdf)
(Gill & Meredith, February 2023) proposes the following library algorithms:

    template<class T>
      requires (is_trivially_relocatable_v<T> && !is_const_v<T>)
    void trivially_relocate(T* first, T* last, T* d_first) noexcept;

    template<class T>
      requires ((is_trivially_relocatable_v<T> && !is_const_v<T>) ||
                is_nothrow_move_constructible_v<T>)
    T* relocate(T* first, T* last, T* d_first) noexcept;

    template<class InputIterator, class NoThrowForwardIterator>
      requires is_nothrow_move_constructible_v<iter_value_t<NoThrowForwardIterator>>
    NoThrowForwardIterator
      move_and_destroy(InputIterator first, InputIterator last,
                       NoThrowForwardIterator d_first);

`trivially_relocate` is specified to _always_ trivially relocate.
It handles overlapping arrays "by magic," just like `memmove` and `q_relocate_overlap_n`.
So it could be the building block for _both_ `Vector::insert` and `Vector::erase`... except
that it cannot handle arbitrary types! It SFINAEs away when trivial relocation isn't possible.

`relocate` also handles overlapping arrays "by magic," like `memmove`.
It can _almost_ handle arbitrary types; it relocates trivially if possible, falling back
to move+destroy. But it cannot handle types with throwing move constructors, such as
MSVC's `std::list`, unless they happen to be trivially relocatable.
(Instead of cleaning up after an exception, the way
[`std::uninitialized_move`](https://en.cppreference.com/w/cpp/memory/uninitialized_move)
does, `relocate` is specified to SFINAE away when relocation might throw an exception.
Thus it never throws. P2786R0 missed marking it `noexcept`, but I've fixed that above.)

Finally, `move_and_destroy` relocates by relatively underspecified means. P2786R0 says
"We do not support overlapping ranges in this function," which I think means it's UB to
pass overlapping ranges to this function at all — as in BSL's `destructiveMove`,
Qt's `q_uninitialized_relocate_n`, and `std::copy_n`. Alternatively, it might mean that
the order of traversal is fixed, making `move_and_destroy` a suitable building block
for `Vector::erase` but not `Vector::insert`. `std::copy` and
[P1144R6's `uninitialized_relocate`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1144r6.html#wording-uninitialized.relocate)
follow that approach.

Since all of these algorithms are constrained, none of them can accurately be described
as a "building block" for any of our use-cases. For example, `StaticVector`'s move-constructor
can't be implemented in terms of `move_and_destroy` because `move_and_destroy` fails to compile
at all for C++03-era types such as

    struct Widget {
        std::string s_;
        Widget(const Widget& w) : s_(w.s_) {}
        void operator=(const Widget& w) { s_ = w.s_; }
    };

and of course `StaticVector` needs to be able to work with such types.

### P1144R6

My [P1144R6 "Object relocation..."](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1144r6.html)
(O'Dwyer, June 2022) proposed the following library algorithms:

    template<class T>
    T* relocate_at(T* source, T* dest);

    template<class T>
    T relocate(T* source);

    template<class InputIterator, class NoThrowForwardIterator>
      NoThrowForwardIterator
        uninitialized_relocate(InputIterator first, InputIterator last,
                               NoThrowForwardIterator d_first);

    template<class InputIterator, class Size, class NoThrowForwardIterator>
      pair<InputIterator, NoThrowForwardIterator>
        uninitialized_relocate_n(InputIterator first, Size n, NoThrowForwardIterator result);

`relocate_at` relocates an object (by any means possible) from `source` to `dest`.
This is the building block for `Function` and `Any`.

`relocate` relocates an object from `source` into the return slot.
See ["`std::relocate`'s implementation is cute"](/blog/2022/05/18/std-relocate/) (2022-05-18).
This is the building block for our hypothetical
[`Vector::displace_back`](https://github.com/Quuxplusone/llvm-project/commit/03d304c0977c5bed476d60132c7cf6c8ffe6f9c4#diff-f1f43ad1a94f42808135ffd81fbd791af1fcc8568e5801ce28b38d2d18c4d904R683-R688).

`uninitialized_relocate` relocates by any means possible, and invariably from left to right,
like `std::copy`. This makes it a suitable building block for `Vector::reallocate` and
`Vector::erase`, but not `Vector::insert`.

`uninitialized_relocate_n` is exactly the same: relocate by any means possible, from left
to right, like [`std::copy`](https://en.cppreference.com/w/cpp/algorithm/copy)
(and _not_ like [`std::copy_n`](https://en.cppreference.com/w/cpp/algorithm/copy_n)).

If you want to relocate from right to left, as in `Vector::insert`, then (under P1144R6)
you must write something like this:

    template<class BidirIterator, class NoThrowBidirIterator>
    NoThrowBidirIterator
    uninitialized_relocate_backward(BidirIterator first, BidirIterator last,
                                    NoThrowBidirIterator d_last)
    {
      std::uninitialized_relocate(
        std::make_reverse_iterator(last),
        std::make_reverse_iterator(first),
        std::make_reverse_iterator(d_last));
    }

> As usual, Alexander Stepanov was here 30 years ago. Instead of calling the C++98 STL's
> [`std::copy_backward`](https://en.cppreference.com/w/cpp/algorithm/copy_backward),
> you could have written:
>
>     std::copy(
>       std::make_reverse_iterator(last),
>       std::make_reverse_iterator(first),
>       std::make_reverse_iterator(d_last));
>
> But that requires your STL vendor's `std::copy` to have special-case `memmove`’ing
> codepaths not only for `TriviallyCopyable*` but also for `std::reverse_iterator<TriviallyCopyable*>`.
> No vendor does that. Instead, they provide `std::copy` and `std::copy_backward`, each
> with a single special-case codepath for `TriviallyCopyable*`. That's much easier to
> implement.

Therefore, P1144R7 will propose to add `uninitialized_relocate_backward`, too.


## Comparison table

In the table below, ✓ means "does what you'd expect"; UB means "undefined or unpredictable/wrong behavior";
SFINAE means "the call fails to compile, in a SFINAE-friendly way"; and "N/A" means "not applicable/don't care."
`q_uninitialized_relocate_n` supports types with non-`noexcept` move constructors, but _at least_ fails to say
(and possibly fails to consider) what happens at runtime were a move constructor actually to throw.

<table class="smaller">
<tr><td></td> <td></td> <td>T.r. types</td> <td>Non-t.r. types</td> <td>Throwing-move types</td> <td>Rightward motion (`insert`)</td> <td>Leftward motion (`erase`)</td> <td>Non-pointer iterators</td> </tr>
<tr><td rowspan="3">STL Classic (non-relocating)</td>
                                  <td><code>std::copy</code></td>                       <td>N/A</td> <td>N/A</td>  <td>✓</td>      <td>UB</td> <td>✓</td>  <td>✓</td>      </tr>
<tr>                              <td><code>std::copy_n</code></td>                     <td>N/A</td> <td>N/A</td>  <td>✓</td>      <td>UB</td> <td>UB</td> <td>✓</td>      </tr>
<tr>                              <td><code>std::copy_backward</code></td>              <td>N/A</td> <td>N/A</td>  <td>✓</td>      <td>✓</td>  <td>UB</td> <td>✓</td>      </tr>
<tr><td rowspan="2">cstring</td>  <td><code>memcpy</code></td>                          <td>✓</td> <td>UB</td>     <td>✓</td>      <td>UB</td> <td>UB</td> <td>SFINAE</td> </tr>
<tr>                              <td><code>memmove</code></td>                         <td>✓</td> <td>UB</td>     <td>✓</td>      <td>✓</td>  <td>✓</td>  <td>SFINAE</td> </tr>
<tr><td rowspan="2">Qt</td>       <td><code>q_uninitialized_relocate_n</code></td>      <td>✓</td> <td>✓</td>      <td>✓?</td>     <td>UB</td> <td>UB</td> <td>SFINAE</td> </tr>
<tr>                              <td><code>q_relocate_overlap_n</code></td>            <td>✓</td> <td>✓</td>      <td>✓</td>      <td>✓</td>  <td>✓</td>  <td>SFINAE</td> </tr>
<tr><td rowspan="1">BSL</td>      <td><code>destructiveMove</code></td>                 <td>✓</td> <td>✓</td>      <td>✓</td>      <td>UB</td> <td>UB</td> <td>SFINAE</td> </tr>
<tr><td rowspan="3">P2786R0</td>  <td><code>trivially_relocate</code></td>              <td>✓</td> <td>SFINAE</td> <td>SFINAE</td> <td>✓</td>  <td>✓</td>  <td>SFINAE</td> </tr>
<tr>                              <td><code>relocate</code></td>                        <td>✓</td> <td>✓</td>      <td>SFINAE</td> <td>✓</td>  <td>✓</td>  <td>SFINAE</td> </tr>
<tr>                              <td><code>move_and_destroy</code></td>                <td>✓</td> <td>✓</td>      <td>SFINAE</td> <td>UB</td> <td>?</td>  <td>✓</td>      </tr>
<tr><td rowspan="2">P1144R6</td>  <td><code>uninitialized_relocate</code></td>          <td>✓</td> <td>✓</td>      <td>✓</td>      <td>UB</td> <td>✓</td>  <td>✓</td>      </tr>
<tr>                              <td><code>uninitialized_relocate_n</code></td>        <td>✓</td> <td>✓</td>      <td>✓</td>      <td>UB</td> <td>✓</td>  <td>✓</td>      </tr>
<tr><td rowspan="1">P1144R7</td>  <td><code>uninitialized_relocate_backward</code></td> <td>✓</td> <td>✓</td>      <td>✓</td>      <td>✓</td>  <td>UB</td> <td>✓</td>      </tr>
</table>
