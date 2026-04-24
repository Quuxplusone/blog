---
layout: post
title: "Things C++26 `define_static_array` can't do"
date: 2026-04-24 00:01:00 +0000
tags:
  constexpr
  metaprogramming
  reflection
---

We've [seen previously](/blog/2023/10/13/constexpr-string-round-2/)
that it's not possible to create a `constexpr` global variable of container type,
when that container holds a pointer to a heap allocation. It's fine to create a
global constexpr `std::array`, or even a `std::string` that uses only its SSO buffer;
but you can't create a global constexpr `std::vector` or `std::list` (unless it's
empty) because it would have to hold a pointer to a heap allocation.

Think of constexpr evaluation as taking place "in the compiler's imagination."
Since C++20 it's fine to use `new` and `delete` at constexpr time; but there's a firewall between
constexpr evaluation and real, material runtime existence. You can't, at runtime, get
a pointer to a heap allocation that was made only "in the compiler's imagination," any more
than you can get a pointer to a local variable of a stack frame that was made only
"in the compiler's imagination." So none of these snippets will compile:

    constexpr int *f() { int i = 42; return &i; }
    constinit int *p = f(); // error

    constexpr int *f() { return new int(42); }
    constinit int *p = f(); // error

    constexpr std::vector<int> f() { return {1,2,3}; }
    constinit std::vector<int> p = f(); // error

But if you can compute a `std::vector<int>` at constexpr time, then you can persist its contents
into a global constexpr `std::array` of the appropriate size. The appropriate size is just
the `.size()` of the vector you computed, of course. So we have what's become known as the
"constexpr two-step" ([Godbolt](https://godbolt.org/z/fMdfPe14f)):

    constexpr std::vector<int> f() { return {1,2,3}; }

    constinit auto a = []() {
      std::array<int, f().size()> a;
      std::ranges::copy(f(), a.begin());
      return a;
    }();

Thanks to Barry Revzin's [P3491](https://isocpp.org/files/papers/P3491R3.html) (June 2025)
and Jason Turner's ["Understanding the Constexpr 2-Step"](https://www.youtube.com/watch?v=_AefJX66io8) (C++ On Sea 2024)
for the term "constexpr two-step." Jason's talk deals with a specific formula in which
instead of _repeating_ — and repeatedly evaluating — `f()` in the body of the lambda,
we factor it out into a template argument ([Godbolt](https://godbolt.org/z/dff31Y6E9)):

    constexpr std::vector<int> f() { return {1,2,3}; }

    template<auto B>
    consteval auto to_array() {
      // MAGIC NUMBER WARNING!
      constexpr auto v = B() | std::ranges::to<std::inplace_vector<int, 999>>();
      std::array<int, v.size()> a;
      std::ranges::copy(v, a.begin());
      return a;
    }

    constinit auto a = to_array<[]() { return f(); }>();

C++26 will introduce a new and improved tool for this kind of compile-time array generation.
It's spelled `std::define_static_array`. In C++26 you can just write this
([Godbolt](https://godbolt.org/z/5dc3EGraq)):

    constexpr std::vector<int> f() { return {1,2,3}; }
    constinit std::span<const int> sp = std::define_static_array(f());

This call to `define_static_array` returns a `span` over a static-storage constant array of three ints.
Basically this is asking the compiler to take the data it's come up with "in its imagination" and
write down a copy of it in the object file. This is much cleaner and more compile-time-efficient than
the "two-step"!

Unfortunately, if I understand it correctly, C++26 `define_static_array` does not (yet?) support
several things that you _can_ do using the "two-step." Here are a few such things.

### 1. Non-structural types

`std::define_static_array` is defined in terms of `std::meta::reflect_constant(e)`,
which [C++26 defines](https://eel.is/c++draft/meta.reflection.result) as
`std::meta::template_arguments_of(^^TCls<e>)[0]` for some invented template `TCls`. That is,
`reflect_constant` (and thus `define_static_array`) is defined only for
[structural types](https://eel.is/c++draft/temp.param#def:type,structural). `int` is a structural
type, and thus we can write the code above. But we cannot write

    using OInt = std::optional<int>;
    constexpr std::vector<OInt> f() { return {1,2,3}; }
    std::span<const OInt> sp = std::define_static_array(f());

because `optional<int>` is not a structural type. Nor are `string`, `string_view`, `span` itself...
There are many types that can't be materialized using `define_static_array`, even though
they work fine with the "constexpr two-step" ([Godbolt](https://godbolt.org/z/48fE48Mz6)).

### 2. Pointers to string literals

Because `reflect_constant` is defined in terms of `TCls<e>`, not only must the
_type_ of `e` be structural, but each particular _value_ `e` in the array must be suitable for
use as a template argument. `const char*` is a structural type, but if that pointer points to
a string literal, then it's not suitable for use as a template argument. So we can use
`define_static_array` to make an array of null pointers:

    constexpr std::vector<const char*> f() { return {nullptr, nullptr, nullptr}; }
    std::span<const char *const> sp = std::define_static_array(f());

but it cannot make an array of pointers to literals:

    constexpr std::vector<const char*> f() { return {"a", "b", "c"}; }
    std::span<const char *const> sp = std::define_static_array(f());

On the other hand, the "constexpr two-step" has no problem with string literals
([Godbolt](https://godbolt.org/z/7Tfo9Krb3)).

### 3. Move-only types

In order to create a template parameter object representing `e`, we must make
a copy of `e` ([[temp.arg.nontype]/4](https://eel.is/c++draft/temp.arg.nontype#4)).
Therefore NTTP types must be copyable. You can (with care) use the two-step to create
a static array of move-only type:

    constexpr auto a = []() {
      std::array<MoveOnly, f().size()> a;
      std::ranges::copy(f() | std::views::as_rvalue, a.begin());
      return a;
    }();

but you cannot do the same with `define_static_array`. ([Godbolt.](https://godbolt.org/z/nMv34E1qq))

The above snippet, like all my other examples of the "two-step," never actually uses move-construction;
it uses default construction followed by assignment. This is unsatisfying, and prevents the two-step
from creating e.g. an array of `reference_wrapper`. `define_static_array`, on the other hand, does
not use default-construction ([Godbolt](https://godbolt.org/z/Wbn73MaGM)).
Can we rework the two-step to eliminate the default-constructibility requirement?
I imagine we can, but at the moment I don't see how.

### 4. Make the array mutable

`define_static_array` allocates its array in rodata and gives you a `span<const T>`
over it. This allows the compiler to do cool things, like point multiple invocations of
`define_static_array` at the same backing array ([Godbolt](https://godbolt.org/z/zKTMs8bd4)).
In fact, the compiler is actually _required_ to do that, because
`reflect_constant` is defined in terms of a [template parameter object](https://eel.is/c++draft/temp.param#13)
which for all intents and purposes behaves like an inline variable: there is guaranteed
to be only one template parameter object with a given type and value in the whole program
([Godbolt](https://godbolt.org/z/KnzT1qhae)).

Treating template parameter objects as inline variables means the compiler _must_ combine
such objects when they have the same type and value (optimization! hooray!) but sadly also
_forbids_ an otherwise sufficiently smart compiler from combining such objects when their
types are merely similar. [Godbolt](https://godbolt.org/z/ea9f943rK):

    template<auto V> auto tpo() { return std::span(V); }
    template<auto V> auto tpo2() { return std::span(V); }

    const void *p1 = tpo<std::array<signed char,3>{1,2,3}>().data();
    const void *p2 = tpo2<std::array<signed char,3>{1,2,3}>().data();
    const void *p3 = tpo<std::array<unsigned char,3>{1,2,3}>().data();
    const void *p4 = tpo<std::array<char,3>{1,2,3}>().data();

All four of these pointers point to arrays of the three bytes `01 02 03`. `p1` and `p2`
are required to point to the same byte; `p3` and `p4`, since they point to `std::array`
objects of different types, are required to point to different arrays. The compiler
isn't allowed to coalesce `p3` and `p4`, the way it's allowed to coalesce
the backing arrays of differently typed `initializer_list`s ([Godbolt](https://godbolt.org/z/8EPae4cPo)).

But (hooray! and thanks to Tim Song for correcting me on this!) there is a special case
specifically for the "template parameter objects of array type" created by `reflect_constant_array`
and `define_static_array`. _These_ objects _are_ permitted
([[intro.object]/9.3](https://eel.is/c++draft/intro.object#def:object,potentially_non-unique))
to overlap or be coalesced, just like `initializer_list`s and string literals. Clang trunk
isn't smart enough to coalesce potentially non-unique objects; therefore the Clang reference
implementation of C++26 Reflection doesn't coalesce these array objects either; but it's
not the paper standard's fault. [Godbolt](https://godbolt.org/z/o6r73rP3W):

    const void *p1 = std::define_static_array(std::vector<signed char>{1,2,3}).data();
    const void *p2 = std::define_static_array(std::list<signed char>{1,2,3}).data();
    const void *p3 = std::define_static_array(std::vector<unsigned char>{1,2,3}).data();
    const void *p4 = std::define_static_array(std::vector<char>{1,2,3}).data();

All four of these pointers point to arrays of the three bytes `01 02 03`. `p1` and `p2`
are required to point to the same byte; `p3` and `p4` are permitted, but not required,
to point to different arrays. In practice Clang makes them different; GCC, once it implements
`define_static_array`, will presumably make them the same.

However, template parameter objects are invariably const! Therefore, you cannot use
`define_static_array` to produce a `constinit`-but-mutable array, the way you can
with the "constexpr two-step." It seems to me perfectly reasonable to want a magic consteval
function that says, "Please generate me a mutable array in static storage with these
contents" — specified as a constexpr-time `vector<int>` — "and give me a `span` over it":

    template<class R>
    consteval auto define_mutable_static_storage_array(R&& r)
        -> std::span<std::ranges::range_value_t<R>>;

Perfectly reasonable to _want_ such an API; but C++26 `define_static_array` fundamentally
isn't that API. It can't produce mutable data: it can't produce _anything_ except pointers
into (potentially non-unique) template parameter objects, which behave like const inline variables.

## Conclusion

In short, `define_static_array` is constitutionally unsuited for some conspicuous use-cases.
I'm not sure what this means for the future. I'm sure we don't want to require people to
use the "constexpr two-step" forever; but `define_static_array` doesn't seem suited to
replace _all_ of its uses — certainly not in C++26, and I don't see how it could be extended
in the future to solve any of the problems I outlined above.

I imagine the answer is not "`define_static_array` will solve all your problems today,"
nor "a new and improved `define_static_array` will solve all your problems in C++XY,"
but rather "C++XY will introduce a new and different facility for manipulating static storage" —
possibly related to the as-yet-unstandardized code-generation side of reflection —
and we'll use that new facility to solve some (but perhaps not all) of the above problems.

----

UPDATE: Actually, problems (1), (2), and (3) all stem from `define_static_array`’s
requirement that each element be usable as an NTTP. Barry Revzin's
[P3380R1 "Extending support for class types as NTTPs"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3380r1.html)
(December 2024) lays out a plan that would permit the programmer to mark their own types
as _explicitly structural_, thus (if accepted) addressing all three of those problems.
On the other hand, making a user-defined type
_explicitly structural_ per P3380R1 seems to involve pretty arcane programming.
The "constexpr two-step" stays general by staying above the fray: it simply never requires
anything to be encoded as a template argument.
