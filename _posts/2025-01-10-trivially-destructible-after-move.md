---
layout: post
title: '"Trivially relocatable" versus "trivially destructible after move"'
date: 2025-01-10 00:01:00 +0000
tags:
  library-design
  relocatability
  triviality
  type-traits
excerpt: |
  Recently I discovered that besides the various libraries using P1144-style
  (or occasionally P2786-style) trivial relocatability, there are also at least
  two that use [P1029R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1029r2.pdf)-style
  "trivial destructibility after move."

  This blog post demonstrates the difference between `is_trivially_relocatable`
  and `is_trivially_destructible_after_move`.
---

Recently I discovered that besides the various libraries using P1144-style
(or occasionally P2786-style) trivial relocatability, there are also at least
two that use [P1029R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1029r2.pdf)-style
"trivial destructibility after move."

> Note: Not [P1029R3](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1029r3.pdf)-style
> "move = bitcopies"; that's much closer to the union of both traits together.

This blog post demonstrates the difference between `is_trivially_relocatable`
and `is_trivially_destructible_after_move`.

Here are the two implementations of `is_trivially_destructible_after_move` of which
I'm aware:

* Domagoj Šarić's [psiha/vm](https://github.com/psiha/vm/) defines the trait
`moved_out_value_is_trivially_destructible`.

* Artur Bać's [arturbac/small_vectors](https://github.com/arturbac/small_vectors/) defines the concept
`concepts::relocatable`.

Recall that the verb "relocate" means "take the object that used to be at `src`,
get rid of it, and make an object at `dst` with that same value." Recall that this
is different from move-construction, and also different from move-assignment. Here's
a Standard C++ definition of the P1144-alike library algorithm `uninitialized_relocate`.
(For simplicity, I'm assuming raw pointers instead of iterators, and omitting all
exception-safety and error-handling.)

    template<class T>
    T *raw_uninitialized_relocate(T *first, T *last, T *dfirst) {
      T *dlast = std::uninitialized_move(first, last, dfirst);
      std::destroy(first, last);
      return dlast;
    }

The two different traits let us add two different optimization codepaths,
like this:

    template<class T>
    T *raw_uninitialized_relocate(T *first, T *last, T *dfirst) {
      if constexpr (my::is_trivially_relocatable_v<T>) {
        size_t n = last - first;
        std::memmove((void*)dfirst, (void*)first, n * sizeof(T));
        return dfirst + n;
      } else if constexpr (my::is_trivially_destructible_after_move_v<T>) {
        return std::uninitialized_move(first, last, dfirst);
      } else {
        T *dlast = std::uninitialized_move(first, last, dfirst);
        std::destroy(first, last);
        return dlast;
      }
    }

Only `is_trivially_relocatable` allows us to replace a whole swath of relocations with
`memmove`, because only `is_trivially_relocatable` says anything about the triviality
of the relocation operation as a whole. The optimization codepath for
`is_trivially_destructible_after_move` doesn't involve `memmove`; it skips
the destructors of the moved-from elements, but it doesn't have any insight into
what the move constructor might have done.

## `std::list` illustrates the difference

Recall that there are two implementation strategies for the STL's `std::list`.

* The "sentinel-node" strategy, used by Microsoft STL. Every `list` ends with
    an additional allocated node, called the "sentinel." Even a default-constructed
    (empty) `list` manages a single heap-allocated sentinel node.

* The "dummy-node" strategy, used by libstdc++ and libc++. The `list` object
    itself contains a "dummy" node, consisting of only a prev/next pointer pair
    (no payload). The `back()` node's next pointer points at that dummy node,
    and the dummy node's prev pointer points at the heap-allocated `back()` node.

The relocation operation for each kind of `list` looks like this. (Lines with arrows
represent one-way pointers; lines without arrows represent two-way prev/next pairs.
The elements highlighted in red are created or updated by the move constructor.)

| Step | Sentinel node<br>(MSVC) | Dummy node<br>(everyone else) |
|------|---------------|------------|
| Initial state:<br> `src` holds `{A,B,C}` | ![](/blog/images/2025-01-10-a1.png) | ![](/blog/images/2025-01-10-a2.png) |
| Move-construct<br> `dst` from `src` | ![](/blog/images/2025-01-10-b1.png) | ![](/blog/images/2025-01-10-b2.png) |
| Destroy the<br> moved-from `src` | ![](/blog/images/2025-01-10-c1.png) | ![](/blog/images/2025-01-10-c2.png) |

Notice that MSVC's move-construction operation doesn't need to touch any of the old heap-allocated nodes;
but it does need to allocate a new sentinel node for the moved-from `src`. Contrariwise, the dummy-node
implementation doesn't allocate, but it does need to update node `C`'s next pointer to point to the
dummy node stored inside `dst` (where previously it pointed to the dummy node stored inside `src`).

Notice that in the dummy-node implementation we can safely skip the destructor of the moved-from `src`;
it no longer controls any resources. Contrariwise, in MSVC's sentinel-node implementation, skipping
the destructor of `src` would leak memory.

Therefore:

* In Microsoft STL, `is_trivially_relocatable<std::list<int>>` is `true`
    but `is_trivially_destructible_after_move<std::list<int>>` is `false`.

* In libc++ and libstdc++, `is_trivially_destructible_after_move<std::list<int>>` is `true`
    but `is_trivially_relocatable<std::list<int>>` is `false`.

## Another example, and caveat

libstdc++'s `deque` has a "never-empty" invariant similar to MSVC's `list`.

* In libstdc++, `is_trivially_relocatable<std::deque<int>>` is `true`
    but `is_trivially_destructible_after_move<std::deque<int>>` is `false`.

In fact, there's a big caveat to mention at this point: many types (such as
for example libstdc++'s `string`) are trivially destructible after move _construction_
but not trivially destructible after move _assignment!_

    std::string src = "this string heap-allocates";
    std::string dst = "this string heap-allocates too";
    dst = std::move(src);
    assert(src.capacity() > std::string().capacity());

So if you're thinking of using a trait like `is_trivially_destructible_after_move`,
you should consider your optimizations carefully: make sure you know what kind of
"move" operation you're "after," before assuming that it's okay to skip a destructor call.

    void inplace_vector<T>::operator=(inplace_vector&& rhs) {
      size_t n = size();
      if (n < rhs.size()) {
        std::move(rhs.begin(), rhs.begin() + n, begin());
        std::uninitialized_move(rhs.begin() + n, rhs.end(), begin() + n);
        if constexpr (my::is_trivially_destructible_after_move<T>) {
          // WRONG: we cannot skip the first n destructor calls!
        } else {
          std::destroy(rhs.begin(), rhs.end());
        }
      } ~~~~
    }

The `operator=` above leaks memory when `T` is libstdc++'s `string`.

Finally, it should go without saying that none of this holds for operations that "move"
from type `T` into a different type `U`. For example, this code is dangerously buggy:

    void transfer(T *src, U *dst) {
      std::construct_at(dst, std::move(*src));
      if constexpr (my::is_trivially_destructible_after_move<T>) {
        // WRONG: we cannot skip this destructor call!
      } else {
        std::destroy_at(src);
      }
    }

because it leaks memory when:

    using T = std::shared_ptr<int>;
    struct U { U(const T&) {} };
