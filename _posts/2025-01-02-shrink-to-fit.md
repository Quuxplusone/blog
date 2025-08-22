---
layout: post
title: "Does `v.clear()` free the allocation?"
date: 2025-01-02 00:01:00 +0000
tags:
  implementation-divergence
  standard-library-trivia
  stl-classic
---

A perennial question on the [cpplang Slack](https://cppalliance.org/slack/):
"When I call `v.clear()` on a vector, does that hang onto the heap allocation,
or free it?" The short answer is: "It hangs onto the allocation. Follow up with
`v.shrink_to_fit()` if you need the allocation eagerly returned."
Now here's the long answer!

In general, the STL tries not to do things you didn't ask for. When you're
actually done with the vector, of course its _destructor_ will both destroy
the vector's elements _and_ free the heap allocation. But when you
merely call `v.clear()`, you're saying you're _not_ done with the vector —
you might put some elements into it again. Therefore it would be silly
for the STL to free the old heap allocation, because it'll just need to make
a new heap allocation again for those new elements. By hanging onto the
vector's old heap allocation, we save one round-trip to the heap.

When your intent is to actually free the block — that is, to
_reduce memory usage_ as much as possible — you should use
the dedicated intent-expressing method `v.shrink_to_fit()`. That is, first you
`clear` the vector to reduce its `size` to zero, and then you `shrink_to_fit` it
to reduce its `capacity` also to zero. You can also `shrink_to_fit` a non-empty
vector:

    v = {1,2,3};
    v.shrink_to_fit();

This makes `v.capacity()` equal to 3 (with the usual caveat that _technically_
the behavior is implementation-defined; but all vendors get this part right).
Notice that this requires the library to make a new memory allocation of size&nbsp;3
before it can give back the original allocation! That's right: `shrink_to_fit`
doesn't just mean "please free unused blocks," it means "please relocate my data
into _new, smaller, blocks_ so that you can free the older larger blocks."
Thus `shrink_to_fit` invalidates pointers and iterators.

> [P0447 "`std::hive`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p0447r28.html)
> introduces a new verb, `v.trim_capacity()`, to mean "please free unused blocks
> without relocating elements or invalidating iterators." This new verb is almost
> meaningless to `vector`; but it is very meaningful to `deque`, and if P0447 is accepted,
> `deque` should certainly also gain a `trim_capacity` method.

## How do I clear and free in one line?

Usually whoever's asking about `v.clear()` won't take `shrink_to_fit` for an answer.
Everyone's looking for a one-liner here. So they ask: How about `v.resize(0)`? How about `v = {}`?
How about...?

Here ([Godbolt](https://godbolt.org/z/9cdYq6jK8), [backup](/blog/code/2025-01-02-shrink-to-fit.cpp))
is a test program running through each STL vendor's implementation
of each relevant container. Unfortunately this produces a table with three independent
dimensions, which I have to squeeze into two for presentation.

| Code          | `vector` | `string`   | `deque`  |
|:--------------|:--------:|:----------:|:--------:|
| `v.clear()`   | &#x274C; | &#x274C;   | MS       |
| `v.resize(0)` | &#x274C; | &#x274C;   | &#x274C; |
| `v = {}`      | &#x274C; | &#x274C;   | &#x274C; |
| `v = V()`     | &#x2705; | libc++, MS | &#x2705; |
| `V().swap(v)` | &#x2705; | &#x2705;   | &#x2705; |

The sole one-liner that reliably works for all three containers, on all three vendors' implementations,
is `V().swap(v)` — the same one-liner that's been known since C++98. It's covered in Item 7.3 of Herb Sutter's
[_More Exceptional C++_](https://amzn.to/4h2DvFL) (2001), for example.

Only Microsoft's `deque` currently treats `d.clear()` as a request to free the deque's allocation.
(So only Microsoft ever treats `clear` any differently from `resize(0)`.)
But notice that Microsoft is also the only vendor where `deque` can hang onto an arbitrarily large
number of unused blocks! libc++'s and libstdc++'s `deque` will never keep more than two unused blocks.
In fact, libstdc++'s `deque` is "never-empty": it always hangs onto one block's worth of storage,
even in its "emptiest possible" default-constructed state.

> That's why libstdc++ trunk manually warrants its `deque` as trivially relocatable:
> its move-constructor may throw. It would get the [vector pessimization](/blog/2022/08/26/vector-pessimization/),
> if not for that manual marking. Still, wrapping libstdc++'s `deque` in a Rule-of-Zero type
> reinstates the vector pessimization (unless you have [P1144](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1144r12.html)):
> [Godbolt.](https://godbolt.org/z/vM4dTM9cM)
>
> Microsoft's `list` has the same issue as libstdc++'s `deque`,
> but stoically accepts its vector pessimization:
> [Godbolt.](https://godbolt.org/z/sa8T6eMKo)

`deque`'s heap-management strategy is complicated, highly varied from vendor to vendor,
and definitely a story for another day.

Finally, we might ask: What is the type of the right-hand operand in `v = {}`?
This trivia question fooled me! You might already know the surprising factoid that
an initialization like `A a = {}` _never_ calls an initializer-list constructor — it
calls `A`'s default constructor instead. So you might think `a = {}` would work the
same way. (I did!) But we'd be wrong: In fact the special case for `{}` is
codified in <a href="https://eel.is/c++draft/dcl.init.list#3.5">[dcl.init.list]</a> and thus applies
only to initialization. The assignment-expression `a = {}`, like any other function call such as `f({})`,
prefers to bind `{}` to a parameter of type `initializer_list`, rather than to a parameter of type `A`.
([Godbolt.](https://godbolt.org/z/sPfxh4or8)) So `a = {}` means something different from — and
generally less memory-efficient than — `a = A()`.
