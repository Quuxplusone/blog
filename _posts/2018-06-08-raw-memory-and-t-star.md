---
layout: post
title: 'Pointer to raw memory? `T*`.'
date: 2018-06-08 00:01:00 +0000
tags:
  alignment
  allocators
  c++-style
  library-design
  rant
  standard-library-trivia
---

In my talk ["The Best Type Traits C++ Doesn't Have"](https://youtu.be/MWBfmmg8-Yo?t=41m),
I present a design for `tombstone_traits`, whose primary template looks like this:

    template<class T>
    struct tombstone_traits {
        static constexpr size_t spare_representations = 0;
        static constexpr size_t index(const T *p) { return size_t(-1); }
        static constexpr void set_spare_representation(T *p, size_t i) = delete;
    };

The idea is that you can hook this up to optimize the memory layout of `std::optional<T>`.
Many details omitted from this sample code; this is merely to help you get the idea if
you haven't seen the talk. (Thanks to Nicole Mazzuca for the suggestion that as long as
we're breaking ABI anyway, `tombstone_traits` should be a defaulted template type
parameter instead of hard-coded.)

    template<
        class T,
        class Traits = std::tombstone_traits<T>,
        class = std::enable_if_t<Traits::spare_representations >= 1>
    >
    class optional_base {
        union {
            char dummy_;
            T value_;
        };
        optional_base() {
            Traits::set_spare_representation(&value_, 0);
        }
        ~optional_base() {
            if (Traits::index(&value_) == 0) {
                value_.~T();
            }
        }
    };

Notice that when we call `index`, the parameter `T *p` points to an actual `T`
object if and only if the `optional` happens to be engaged. The implementation
of `index` must rely on type-punning to examine the bitwise contents of that memory,
to figure out whether there's a `T` object actually present or not.

Both times I've presented this (so far), somebody has asked, "Why do you use `T*`
as the parameter type of `index`? If `index` needs to look at the raw contents of memory,
shouldn't you be passing in something like `std::span<std::byte>` instead?"

I answer "No."


## `T*` before `byte*`

First of all, `std::span<std::byte>` would be crazy heavyweight (both in object size and
in header weight) compared to `T*`. And it would require C++2a, since `std::span` isn't in
C++17.

Okay, so what about `std::byte*`? That still drags in some header weight (because C++17 `std::byte`
for whatever reason is not a core-language type; it's
[an `enum` defined in `<cstddef>`](https://github.com/llvm-mirror/libcxx/blob/5272877/include/cstddef#L65)),
and requires C++17. Also, we're likely going to be type-punning the contents of memory anyway,
so passing `std::byte*` instead of `T*` doesn't stop us from needing to write `reinterpret_cast`s.
(Unless we're satisfied with reading memory as an array of `std::byte`s, of course. But notice
that you can't do arithmetic on `std::byte` values; they support `&` and `<` but not `+` or `-`.)

"Okay, so what about `void*`?" asks my interrogator. That doesn't have *any* header weight;
it's portable back to C++11 and even older; it doesn't pretend to be a type that it's not, because
it doesn't pretend to be *any* type in particular. So why not `void *p`?

Well, `void*` doesn't provide any benefits *over* `T*`; see the argument above about needing to
write `reinterpret_cast`s (or in `void*`'s case, `static_cast`s) either way. And it loses something
in the self-documentation department.

"But doesn't `T*` imply that it actually points to a `T` object, or else is null? In your case,
it might be that neither is true."

No. Consider:

- [`destroy_n`](https://en.cppreference.com/w/cpp/memory/destroy_n) takes an input iterator range
  `[first, last)` which points to a range of `T` objects which the algorithm will destroy. (After the
  call, the range contains raw memory.) In practice, the iterator type we pass in is usually `T*`.

- [`uninitialized_copy`](https://en.cppreference.com/w/cpp/memory/uninitialized_copy) takes an
  output iterator `d_first` which points to raw memory in which the algorithm will construct `T` objects.
  In practice this iterator type is usually `T*`.

- [`allocator::construct`](https://en.cppreference.com/w/cpp/memory/allocator_traits/construct)
  takes a `T*` pointing to raw memory, and constructs a `T` object there.

- [`allocator::allocate`](https://en.cppreference.com/w/cpp/memory/allocator/allocate) returns
  a `T*` pointing to raw memory. (Notice specifically that it does *not* return `void*`!)

- The deprecated [`get_temporary_buffer`](https://en.cppreference.com/w/cpp/memory/get_temporary_buffer)
  returns a `pair<T*, ptrdiff_t>` where the `T*` points to raw memory.

In all cases, the significant factor is that the `T*` parameter (or return value) points to a
place in memory where a `T` object *might plausibly* be stored, now or in the future; even if
there happens to be no `T` object there right now.

So, a parameter of type `T*` tells us that this parameter expects a pointer value that is, or could be,
the address of a `T` object. Most notably, we know that it must be *suitably aligned* for a `T` object.

> [When you hear hoofbeats, think horses, not zebras.](https://en.wikipedia.org/wiki/Zebra_(medicine))

Naturally, all this "*or could be*" business applies only to APIs that are known to deal specifically
in raw or uninitialized memory. In general, for the average API, you should confidently assume that
when a library API asks for a `T*`, it's asking for the address of a `T` object (or maybe null, depending
on the API); and when a library API asks for a `T&`, it's asking for the address of a `T` object, period.
Don't get all paranoid about `T*` now!

But, suppose you're *designing* an API for one of these rare "zebras" that deals in pointers to memory
that might not contain `T` objects right now, but is nevertheless known to be suitably sized and aligned for `T`.
What vocabulary type should you use in your API to pass and return those pointers to memory suitably sized and aligned for `T`?

_You should use `T*`._


## But sometimes `void*`

If at compile time you don't know anything about the memory's alignment, you should use `void*`.
Avoid `std::byte*` (header weight, portability).
Also avoid `char*`, `signed char*`, `unsigned char*`, or `uint8_t*`.
APIs actually concerned with character data should of course use `char*` (for example, `strcpy`).

Good examples, using `void*`:

- [`malloc`](https://en.cppreference.com/w/cpp/memory/c/malloc) and [`free`](https://en.cppreference.com/w/cpp/memory/c/free)
- [`memcpy`](https://en.cppreference.com/w/cpp/string/byte/memcpy)
- [`pmr::memory_resource::allocate`](https://en.cppreference.com/w/cpp/memory/memory_resource/allocate)
- [`align`](https://en.cppreference.com/w/cpp/memory/align)
- [`declare_reachable`](https://en.cppreference.com/w/cpp/memory/gc/declare_reachable)

Poor example, failing to use `void*`:

- [`declare_no_pointers`](https://en.cppreference.com/w/cpp/memory/gc/declare_no_pointers)
