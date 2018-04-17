---
layout: post
title: 'Downsides of omitting trivial destructor calls'
date: 2018-04-17 00:01:00 +0000
tags:
  standard-library-trivia
  sufficiently-smart-compiler
  today-i-learned
---

[Via the std-proposals mailing list.](https://groups.google.com/a/isocpp.org/d/msg/std-proposals/c-joqqwGkNE/9tX4FKlWAAAJ)
Which of these two functions — `foo` or `bar` — do you expect to give better codegen?

    struct Integer {
        int value;
        ~Integer() {} // deliberately non-trivial
    };

    void foo(std::vector<int>& v) {
        v.back() *= 0xDEADBEEF;
        v.pop_back();
    }

    void bar(std::vector<Integer>& v) {
        v.back().value *= 0xDEADBEEF;
        v.pop_back();
    }

[Compile both with GCC and libstdc++.](https://godbolt.org/g/nn2Xfc)
Did you guess correctly?

    foo:
      movq   8(%rdi), %rax
      imull  $-559038737, -4(%rax), %edx
      subq   $4, %rax
      movl   %edx, (%rax)
      movq   %rax, 8(%rdi)
      ret
    bar:
      subq   $4, 8(%rdi)
      ret

What's going on here is that GCC is smart enough to understand that when you
run a destructor on a piece of memory, you end its lifetime, which renders all
preceding writes to that piece of memory "dead". But GCC is *also* smart enough
to understand that a *trivial* destructor (such as the pseudo-destructor `~int()`)
is a no-op with no effects whatsoever.

So, `bar` calls `pop_back`, which runs `~Integer()`, which marks `vec.back()`
as "dead", and GCC eliminates the multiplication by `0xDEADBEEF` entirely.

On the other hand, `foo` calls `pop_back`, which runs the pseudo-destructor `~int()`
(it might choose to omit the call altogether, but it doesn't). GCC observes that
this is a no-op and forgets about it. Therefore GCC does *not* observe that `vec.back()`
is dead, and *cannot* eliminate the multiplication by `0xDEADBEEF`.

This happens for all trivial destructors, not just for pseudo-destructors like `~int()`.
Replace our `~Integer() {}` with `~Integer() = default;` and watch the `imull` instruction
reappear!
