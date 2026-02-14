---
layout: post
title: 'Trivially swappable'
date: 2018-06-29 00:02:00 +0000
tags:
  move-semantics
  proposal
  relocatability
  triviality
  type-traits
  value-semantics
---

This is a recycling of a reply I just made to Mingxin Wang and Gašper Ažman over at
[std-proposals](https://groups.google.com/a/isocpp.org/d/msg/std-proposals/HGCHVSRwSMk/5w1zNETwBgAJ).
TLDR I think "trivially swappable" is a straightforward corollary to "trivially relocatable",
but its cost-of-specification might be a bit higher, and its benefit-in-performance is
essentially zero as far as any use-case *I* can think of.

[In my C++Now 2018 talk on "trivially relocatable"](https://www.youtube.com/watch?v=MWBfmmg8-Yo)
(which is fully captioned and I highly recommend you watch it!), I promised in the outline to
talk about the relationship between "trivially relocatable" and "trivially swappable," and then
did not actually do so. Sorry! Let's do that now.

Essentially, if a type is trivially relocatable then it intuitively ought to be considered
trivially swappable.  However, there are two minor caveats that I can think of off the top
of my head (and the reason I didn't talk about it at C++Now is that I haven't thought about
it much, and the reason for *that* is that I don't have a motivating use-case).

Caveat (A): Trivial relocation can be optimized into `memcpy` or `memmove`.
Trivial swap cannot be optimized into `mem`-anything, because there is no libc primitive for
swapping arrays of bytes. We could certainly propose to add a `__builtin_memswap()` that would
perform the swap "in-place" in cache-line-sized blocks, but I'm not aware of any proposals
nor prior art in that area.

Caveat (B): Notice that whereas "relocate" means "move-construct, then destroy", we might say
that "swap" means "move-construct, then move-assign, then move-assign, then destroy." (This
being the operation done by the unconstrained `std::swap` template.)  This involves a
relationship among 3 operations, which might be a little scarier than relocate's relationship
among 2 operations, which is scarier than the current Standard Library's "trivially X" traits
which all involve only a single operation.

Caveat (C): For small types like `unique_ptr`, `__builtin_memswap()` will not be any faster
than the unconstrained `std::swap` template. The point of optimizing into `mem`-anything is
to get speedups on _large_ arrays, such as during `std::vector` reallocation.
`std::vector` _swapping_ is already fast, and cannot be made faster by `__builtin_memswap()`.

Now, `std::array` swapping could be made faster! Consider:

    std::array<std::unique_ptr<int>, 10000> a;
    std::array<std::unique_ptr<int>, 10000> b;
    a.swap(b);  // could probably get a factor-of-2 speedup on this operation by using __builtin_memswap

But this is not an operation that happens often enough in real programs for anyone to get
really motivated about.

So, "trivially swappable" seems like a straightforward corollary to "trivially relocatable",
but its cost-of-specification might be a bit higher, and its benefit-in-performance is
essentially zero as far as any use-case *I* can think of.
