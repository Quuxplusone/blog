---
layout: post
title: 'P0732R0 and "trivially comparable"'
date: 2018-03-19 00:01:00 +0000
tags:
  language-design
  templates
  wg21
---

Someone in SG14 asks, what do we all think about Jeff Snyder's
[P0732R0 "Class Types in Non-Type Template Parameters"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0732r0.pdf).
Executive summary: I think it's great.

First, the problem statement:

    template<class T, T value>
    int foo() {
        return 42;
    }

This function works great when called as `foo<int, 7>()` or `foo<char, 'A'>()`, but it
[fails to compile](https://wandbox.org/permlink/80fHbyBLCuYgwocJ)
when invoked as `foo<float, 3.14>()` or `foo<std::string, "hello world">()`. Why is that?

[Let's look](https://godbolt.org/g/UKtPrJ) at how the compiler treats `foo` in each case.

    _Z3fooIiLi7EEiv:
      movl $42, %eax
      ret
    _Z3fooIcLc65EEiv:
      movl $42, %eax
      ret

Notice that the name `foo` is getting [name-mangled](https://en.wikipedia.org/wiki/Name_mangling)
based on its template parameters. This is what ensures that `foo<int, 7>` and `foo<int, 8>` end up
with distinct names; and, contrariwise, ensures that `foo<int, 7>` has the _same_ name no matter which
translation unit we're in. Notice that the name-mangling scheme for these primitive types is very
simple: we just name the type, and then express its value as a decimal integer. (If the value is
negative, we prefix the character `n` instead of `-`, since the latter isn't a valid identifier
character.)

Notice also that no matter whether we write `foo<int, 7>` or `foo<int, 3+4>`, the compiler takes care
of making sure that the name is mangled into exactly the same, unique, unambiguous representation
of the value we were trying to express (namely, "seven").

Converting a value into an unambiguous representation in a certain alphabet is closely related to
_serialization_, _marshalling_, or what Python calls [_pickling_](https://docs.python.org/2/library/pickle.html).
Different applications of serialization require different guarantees from the process. In the case
of C++ template mangling, we require very stringent guarantees:

* Obviously, `foo<6>` must be a different function from `foo<7>`.
  We must have `mangle(v1) != mangle(v2)` whenever `v1 != v2`.

* Vice versa, `foo<7>` and `foo<3+4>` must be the same function.
  We must have `mangle(v1) == mangle(v2)` whenever `v1 == v2`.

Okay, fine, those weren't very stringent. But they implicitly contain some interesting assumptions...

What does it mean to say "whenever `v1 == v2`"? We're talking about the behavior of `operator==` there;
but we're also implicitly talking about all of _value semantics_. For example, we are implicitly assuming
that `v1 == v1`.

For which C++ types does `v1` sometimes not equal `v1`?

* `float`. For example, [`nan != nan`](https://wandbox.org/permlink/llWvi1WX34qXTu6H).

* String literals. For example, [sometimes `"hello" != "hello"`](https://wandbox.org/permlink/R4mKfXqYy4EYK0zJ).

So that's why C++ has gone through so many revisions and _still_ doesn't let you write
`f<float, 3.14>()`! Because what would we do with `f<float, some_nan_value>`?

In fact, it gets worse. Are `f<3.14f>` and `f<3.13f + 0.01f>` the same entity, or not? Maybe it
would depend on your platform. Maybe it would depend on your current
[rounding mode](http://en.cppreference.com/w/cpp/numeric/fenv)! And when we go to name-mangle the
thing, how sure are we that each compiler involved will output exactly, let's say, `_Z3fooIfLf3p14EEiv`,
and not `_Z3fooIfLf3p1400000001EEiv` or `_Z3fooIfLf3p139999984EEiv`? Floating-point is kind of a hot mess.
Floating-point template parameters would be too messy to live.


So what about P0732?
--------------------

P0732 proposes that we permit template parameters of a user-defined type *if and only if* that type
is what is known as _trivially comparable_. This is kind of analogous to C++11's notion of
_trivially destructible_ (and so on). The type must have a _defaulted_ comparison operator —
this is new in C++2a, thanks to Herb Sutter's
[P0515 "Consistent comparison"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0515r3.pdf) —
and that defaulted comparison operator must essentially be equivalent to `memcmp`.

> A type is _trivially comparable_ if it is:
>
> * a scalar type for which the `<=>` operator returns either `std::strong_ordering` or `std::strong_equality`, or
>
> * a non-union class type for which
>     - there is an `operator<=>` that is defined as defaulted within the definition of the class,
>     - `operator<=>` returns either `std::strong_ordering` or `std::strong_equality`,
>     - the type of each of its bases is trivially comparable, and
>     - the type of each of its non-static data members is either trivially comparable or an array of such a type (recursively).

Notice that according to P0515R3, all floating-point types have an `operator<=>` that returns
`std::partial_ordering`. So structs containing floating-point members will *not* be trivially comparable
(because `memcmp` would do the wrong thing for them).

Now, one thing that P0732 probably gets wrong is that it **forgets about padding bytes**. So for example
it would consider the following user-defined type to be _trivially comparable_:

    struct Oops {
        char ch;
        // 3 padding bytes of indeterminate value
        int i;
    };

This is not a problem for name-mangling, because name-mangling always happens in a context where the
compiler can see the struct definition, enumerate the members one by one, and quietly skip over the
padding bytes (whose values will be indeterminate and thus must not contribute to the mangling).
However, the padding bytes *do* prevent us from using `memcmp` at runtime to compare the values of
two `struct Oops`es. The topic of one of my C++Now 2018 talks will be on ways to speed up
`vector<T>::operator==`, `std::hash<T>`, and so on, by doing the fast thing when
`is_trivially_comparable<T>` can be detected at compile time. This approach would fall flat on its
face if we permitted `is_trivially_comparable<Oops>`.

Now, maybe all we need is two distinct names — `is_memberwise_comparable` for P0732's purposes, and
`is_memcmp_comparable` for `vector::operator==`'s purposes. But it would be really really nice to
get the two notions unified into a single concept. Because `is_trivially_copyable`
already means "as if by `memcpy`," I think it would be nice for `is_trivially_comparable` to mean
"as if by `memcmp`."

The problem with just saying that structs with padding aren't trivially comparable, and otherwise
letting P0732 go through as written, is that it would prevent us from using types such as `Oops`
(or more realistically, `std::pair<char, int>`) as template parameters!  Worse, it would end up
being *implementation-defined* whether a certain type was usable as a template parameter or not;
and adding or removing or reordering the members of a struct might unexpectedly make it unusable.
Admittedly we have this problem already today, in that innocuous-looking changes could make a
structure non-literal or non-trivially-copyable; but I don't like the idea of making it *worse*.


Final thoughts
--------------

I would prefer that P0732 do something about padding bytes, rather than pretend that they're
okay. I'm not sure _what_ to do about them, though.

Finally, I am mildly concerned that we're adding so many ways for the compiler to "guess" at the
triviality or "well-knownedness" of user-definable operations. For example, C++11 gave us
_trivially copyable_; but recently the Clang project has (wisely!) added a
[new attribute `[[trivial_abi]]`](https://reviews.llvm.org/D41039)
whose raison d'etre is for the programmer to force-override the compiler's "wrong" guess at
trivial copyability — to say "I know this type looks non-trivial — and maybe it is — but trust me,
it's okay to pass it in registers." With P0732 in C++2a, might we soon need an attribute
`[[trivially_comparable]]` in order to say, "I know I provided a non-defaulted `operator==`,
but trust me, it's okay to use this type as a template parameter"?
