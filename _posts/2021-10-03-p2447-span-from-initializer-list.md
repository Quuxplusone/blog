---
layout: post
title: "`std::span` should have a converting constructor from `initializer_list`"
date: 2021-10-03 00:01:00 +0000
tags:
  initializer-list
  llvm
  parameter-only-types
  proposal
  wg21
excerpt: |
  C++17 introduced `std::string_view` as a "parameter-only" drop-in replacement
  for `const std::string&`. This allows us to make clean refactorings such as:
---

{% raw %}
> UPDATE, 2023-12-28: This blog post describes a feature that was missing from C++20
> and C++23; but [P2447](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2447r6.html)
> has been adopted for C++26, so if you try these examples in C++26 you'll find that
> they all work fine, and that's a good thing.

C++17 introduced `std::string_view` as a "parameter-only" drop-in replacement
for `const std::string&`. This allows us to make clean refactorings such as:

    // C++14
    void f(const std::string&);
    void test() {
        std::string s = "hello";
        f(s);
        f("world");
    }

    // C++17
    void f(std::string_view);
    void test() {
        std::string s = "hello";
        f(s);        // OK
        f("world");  // OK
    }

C++20 introduces `std::span<[const] T>` as a "parameter-only" drop-in replacement
for `[const] std::vector<T>&`. This allows us to make clean refactorings such as:

    // C++17
    void f(const std::vector<int>&);
    void test() {
        std::vector<int> v = {1, 2, 3};
        f(v);
        f({1, 2, 3});
    }

    // C++20
    void f(std::span<const int>);
    void test() {
        std::vector<int> v = {1, 2, 3};
        f(v);          // OK
        f({1, 2, 3});  // ...error??
    }

You read that right: `f({1, 2, 3})` compiles when `f` takes `const vector<int>&`, but
it fails to compile when `f` takes `span<const int>`. The problem isn't that `span`
can't be constructed from `initializer_list`: [it can](https://godbolt.org/z/jGYh3a1Kc).

    static_assert(std::is_convertible_v<std::initializer_list<int>, std::span<const int>>);

The problem is that `span`'s templated constructor can't _deduce_ `initializer_list`.
The rules of C++ are such that a function taking `initializer_list` will happily match
a braced-initializer-list like `{1, 2, 3}`, but a function taking simply `T&&` will
never deduce `[with T=initializer_list<int>]`. If you want to be constructible from
a braced-initializer-list, you must provide a converting constructor specifically from
`initializer_list<T>`. And `span` (as of C++20) fails to do so.

Notice that we _can_ write any of

    void f(std::span<const int>);

    f(std::vector{1, 2, 3});                 // conforming
    f(std::array{1, 2, 3});                  // conforming
    f((int[]){1, 2, 3});                     // invalid: GCC and Clang only
    f(std::initializer_list{1, 2, 3});       // invalid: GCC and MSVC only
    f(std::initializer_list<int>{1, 2, 3});  // conforming

But if we're looking for something we can drop in as a replacement for `const vector<int>&`
function parameters — the way `string_view` drops in for `const string&` — well,
`span<const int>` doesn't quite fit the bill... yet.

Federico Kircheis has drafted a proposal for C++23 to fix this problem. It should appear
in an upcoming mailing as paper number P2447. I've done a quick reference implementation
for libc++, which you can find [here](https://github.com/Quuxplusone/llvm-project/commit/span-ctor),
and you can play with it on Godbolt Compiler Explorer [here](https://p1144.godbolt.org/z/qosqxMeP4).


## But... dangling?

When `string_view` was adopted as the parameter-only replacement for `const string&`,
we suffered through a few years of people worrying about the proverbial newbie
writing dangling-reference bugs like

    std::string getString() { return "hello"; }
    std::string_view sv = getString();
    std::cout << sv;  // UB, dangling pointer, possible segfault

But this was not a problem in practice, because we simply teach that `string_view`
is a parameter-only type. You use it _in function parameter lists_ for the express purpose
of avoiding unnecessary `string` constructions, in a function that would ordinarily
take `const string&`.

`span` is the same way. During its standardization, we suffered a little bit from people
worrying about things like ([Godbolt](https://godbolt.org/z/694PYMd79))

    std::vector<int> getVector() { return {1, 2, 3}; }
    std::span<const int> sp = getVector();
    for (int i : sp) std::cout << i;  // UB, dangling pointer, possible segfault

But this was not a problem in practice (and was less of a problem in the Committee
this time around, as far as I know), because people were already familiar with the
notion of "parameter-only types" and how they are meant to be used. Obviously
any reference-semantic type can dangle if it's misused; but `string_view` and `span`
have clearly delineated use-cases, and "put an rvalue into a local variable"
is not one of them.

"Put an rvalue into a _function parameter_," on the other hand, is explicitly
the use-case for both `string_view` and `span`. So it's important that we preserve
their ability to bind to rvalues. See
["Value category is not lifetime"](/blog/2019/03/11/value-category-is-not-lifetime/) (2019-03-11).

> Incidentally, notice that function-parameter-passing is not one
> of the use-cases for `std::reference_wrapper`; therefore it [does not provide](https://eel.is/c++draft/refwrap.const#2)
> construction from rvalues:
>
>     void f(std::reference_wrapper<const int>);
>
>     int i = 42;
>     f(i);   // OK
>     f(42);  // error
>
> There's never any reason to take a function parameter of type `reference_wrapper<const T>`,
> because you could just take a native `const T&` instead. So `reference_wrapper` has no
> vested interest in binding to rvalues.

So, if you're worried about code like this...

    std::string_view sv = "hello"s;
    std::cout << sv;  // UB, possible segfault

    std::span<const int> sp = {1, 2, 3};  // C++20: invalid; proposed: OK
    for (int i : sp) std::cout << i;  // UB, possible segfault

...don't be worried. People know that local variables of parameter-only types are
in danger of dangling; and if they don't know yet, well, we'll teach them!

Also, perhaps I should point out that with _double braces_, this already compiles!
The following code simply calls `span`'s converting constructor from `int (&)[N]`,
deducing `N` as 3 in this case.

    std::span<const int> sp = {{1, 2, 3}};  // OK
    for (int i : sp) std::cout << i;  // UB, possible segfault


## This constructor does change some code's behavior (for the better)

As with any change to the fragile monolith that is C++, adding this converting constructor
would break some (arguably pathological) code. Consider the following C++17 program:

    struct Sink {
        Sink() = default;
        Sink(Sink*) {}
    };

    void countElements(const std::vector<Sink>& v) {
        return v.size();
    }

    Sink a[10];
    int main() {
        std::cout << countElements({a, a+10}) << '\n';
    }

You might think that the implicit constructor `Sink(Sink*)` is extremely contrived;
but in fact two real-world types resembling `Sink` are `void*` (implicitly convertible from `void**`)
and `std::any` (implicitly convertible from `std::any*`).

Obviously, the program above prints `2`, because the vector passed to `countElements` is of size two.
Now we upgrade it to C++20 in the "obvious" way:

    void countElements(std::span<const Sink> v) {
        return v.size();
    }

    Sink a[10];
    int main() {
        std::cout << countElements({a, a+10}) << '\n';
    }

And suddenly it prints `10`! Although `{a, a+10}` _prefers_ to be interpreted
as an `initializer_list`, it _can_ (when no `initializer_list` constructor is present)
be interpreted as an implicit conversion from two arbitrary arguments:
in this case, an iterator pair.

In C++20 today, `span<const Sink>{a, a+10}` is a span of length 10.
If we add this proposed constructor, though, then `{a, a+10}` will get its
preferred interpretation as an `initializer_list`, and so both programs above
will have the same behavior: `{a, a+10}` will uniformly be treated as a range of length 2,
regardless of whether it's taken by `const vector&` or by `span`. In a sense, C++20
"broke" this code, and the proposed new constructor "restores" the expected behavior.


### Dangling array case is slightly altered

    std::span<const int> sp1 = {{1, 2, 3}};  // OK, dangles
    std::span<const int, 3> sp2 = {{1, 2, 3}};  // C++20: OK, dangles. Proposed: error

In C++20, these initializations pick up the constructor from `int(&)[3]`, which is
non-explicit in both cases. After the proposal, these initializations pick up the
constructor from `initializer_list<int>`, which is non-explicit in the first case
(so nothing changes) but explicit in the second case (so it will no longer compile).


## Implementation notes

The proposed constructor is of the form

    template<class E>
    struct Span {
        using V = remove_cv_t<E>;
        Span(initializer_list<V>) requires is_const_v<E>;
    };

Prior to C++20, we would have had to implement this
constructor with awkward metaprogramming.
Because it is not a constructor template,
[`enable_if_t` cannot be used to disable this declaration](https://stackoverflow.com/questions/52077051/sfinae-enable-if-cannot-be-used-to-disable-this-declaration).
So probably we'd put it into a base class, like this:

    template<class E, bool IsConst = is_const_v<E>>
    struct SpanBase {};

    template<class E>
    struct SpanBase<E, true> {
        SpanBase(initializer_list<remove_cv_t<E>>);
    };

    template<class E>
    struct Span : SpanBase<E> {
        using V = remove_cv_t<E>;
        using SpanBase<E>::SpanBase;
    };

In C++20, though, we can just use a simple `requires`-clause
to express the constraint. This is nice.

Here's another thing I noticed while doing [my implementation](https://github.com/Quuxplusone/llvm-project/commit/span-ctor).
You might think that the member-initializers would be simply

    Span(initializer_list<V> il) requires is_const_v<E>
        : data_(il.data()), size_(il.size()) {}

(mirroring the converting constructor from `std::array`).
However, you'd be in for a nasty surprise:

> `initializer_list` has no member function `data()`!

Instead, you must use one of `il.begin()`, `std::begin(il)`,
or `std::data(il)` to get the pointer to its contiguous data.

This discrepancy was noticed and proposed-to-be-fixed in
Daniil Goncharov and Antony Polukhin's
[P1990R1 "Add `operator[]` and `data()` to `std::initializer_list`"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1990r1.pdf)
(May 2020). However, since the authors were more interested
in the "`operator[]`" part of the paper than the "`data()`" part
(in fact `data()` wasn't even part of [P1990R0](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1990r0.pdf)),
and LEWG (rightly) wasn't interested in giving `initializer_list` an `operator[]`,
the paper was abandoned without ever getting `il.data()` working.

----

See also:

* ["`std::span`, the missing constructor"](https://fekir.info/post/span-the-missing-constructor/) (Federico Kircheis, June 2021)
* ["Chromium's `span`-over-initializer-list success story"](/blog/2026/03/19/p2447-success-story/) (2026-03-19)
{% endraw %}
