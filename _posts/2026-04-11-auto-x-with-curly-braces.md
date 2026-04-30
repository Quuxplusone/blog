---
layout: post
title: '`auto{x} != auto(x)`'
date: 2026-04-11 00:01:00 +0000
tags:
  c++-learner-track
  c++-style
  initialization
  initializer-list
---

Recently it was asked: What's the difference between the expressions `auto(x)`
and `auto{x}` in C++23?

The construct `auto(x)` arrived via
[P0849 "_decay-copy_ in the language"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0849r8.html).
We could already write direct-initialization to a _named_ type as either a declaration
or a cast-expression:

    T y(x); // declaration
    return T(x); // cast-expression

P0849 just extended this syntax to work for a _placeholder_ type as well:

    auto y(x); // declaration
    return auto(x); // cast-expression

Both of the latter lines mean "Deduce the type of `auto` from the type of `x` (decaying to
a non-array object type if necessary) — let's call that type `T` — and then explicitly cast `x` to
that type exactly as if the user had written `T` in place of `auto`."

This _usually_ means we're just making a copy of `x` using its copy constructor.
If `x` stands for an xvalue expression, we're calling the move constructor, and if
`x` is a prvalue, we're probably not doing anything at all. `auto(x)` is simple.

But `auto{x}` is more complicated, because curly braces produce an initializer list.
This means the same thing as `T{x}`: "given the list of elements `{x}`, make me a `T`
with those elements." As [[dcl.init.list]/3.7](https://eel.is/c++draft/dcl.init.list#3.7) shows,
that's not always the same thing as "make me a copy of `x`." [Godbolt](https://godbolt.org/z/75MEveMPW):

    auto paren() {
        std::vector<std::any> v;
        return auto(v);
    }

    auto curly() {
        std::vector<std::any> v;
        return auto{v};
    }

The former means "make an empty vector, then return a copy of that vector (with no elements)."
The latter means "make an empty vector, then return a vector _containing_ that vector (with one element)."

* As of this writing, MSVC gets this wrong: it calls the copy constructor in both cases.
    But [[dcl.init.list]/3.7](https://eel.is/c++draft/dcl.init.list#3.7) (last improved by
    [CWG2638](https://cplusplus.github.io/CWG/issues/2638.html)) makes it very clear that
    MSVC is in the wrong.

* `return (x);` implicitly moves from `x` (see [P2266 "Simpler implicit move"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2266r3.html)),
    but `return auto(x);` does not. This makes sense, because `return T(x);` doesn't move-from `x` either.
    Remember, all `auto` does here is hold the place of an explicitly specified `T`.

Consider also these variations:

    auto p = (v); // copy
    auto c = {v}; // initialize a new vector of 1 element

    auto p(v); // copy
    auto c{v}; // initialize a new vector of 1 element (MSVC gets this wrong)

Of course `vector<any>` is a pathological case. Its benefit is that it's also a _simple_ case
using only STL types, in case you ever need to demonstrate the difference between `(x)` and `{x}`
to anyone else.

The takeaway: As always, you should use curly braces when you have a sequence of elements
(such as when initializing an aggregate or a container); if you aren't in that situation (such as when
you're writing generic code) you should use ordinary parentheses.
See ["The Knightmare of Initialization in C++"](/blog/2019/02/18/knightmare-of-initialization/) (2019-02-18).

---

I suspect there is no situation where it ever makes sense to use `auto{x}` in real code.
But I'm glad it exists in the language, for symmetry and consistency with `T{x}`.

Note that all of these lines—

    auto a(1,2,3);
    auto a = auto(1,2,3);
    auto a{1,2,3};
    auto a = auto{1,2,3};

—are invalid C++. You can never direct-initialize `auto` with multiple arguments.
However, both of the following copy-initializations are legal and silly:

    auto i = (1,2,3); // comma operator; i is int
    auto i = {1,2,3}; // i is initializer_list<int>

The latter is a historical accident which is supported these days, as far as I know, _only_ so that we can specify the behavior
of `for (int i : {1,2,3})` without having to write a special case into [[stmt.ranged]](https://eel.is/c++draft/stmt.ranged).

[N3922 "New rules for auto deduction from braced-init-list"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3922.html)
is the paper that removed `auto a{1,2,3}` from the language. N3922 came in 2014, at the height of the "Almost Always Auto" and "Uniform Initialization"
fads; it was widely assumed that newbies would write `auto a{1,2}` and shoot themselves in the foot, but writing `auto a = {1,2}`
wasn't so attractive to newbies and thus wasn't treated so urgently as a footgun. At the same time, N3922 changed both
`auto a{1}` and `auto a = {1}` to deduce `int` rather than `initializer_list<int>`. Only `auto a = {1,2}` remains as a special case
inconsistent with the rest of the language. [N3912 §1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3912.html) says
this special case will be useful to "advanced users," which I think in hindsight was a bad justification for keeping it.
