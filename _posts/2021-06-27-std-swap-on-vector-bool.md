---
layout: post
title: 'Implementation divergence on swapping bools within vectors'
date: 2021-06-27 00:01:00 +0000
tags:
  argument-dependent-lookup
  hidden-friend-idiom
  implementation-divergence
  proxy-references
  stl-classic
  templates
excerpt: |
  Today, while inventing questions for my next
  [C++ Pub Quiz](/blog/2020/09/18/cppcon-2020-pub-quiz-2/),
  I ran into a fairly obscure and meaningless divergence among
  vendors' implementations of `vector<bool>::reference`. Given:

      std::vector<bool> v = {true, true};

  The correct way to swap `v[0]` with `v[1]` is of course `swap(v[0], v[1])`,
  optionally preceded by `using std::swap`;
  see ["What is the `std::swap` two-step?"](/blog/2020/07/11/the-std-swap-two-step/) (2020-07-11).
  But look at all these _wrong_ things you can try!
---

Today, while inventing questions for my next
[C++ Pub Quiz](/blog/2020/09/18/cppcon-2020-pub-quiz-2/),
I ran into a fairly obscure and meaningless divergence among
vendors' implementations of `vector<bool>::reference`. Given:

    std::vector<bool> v = {true, true};

The correct way to swap `v[0]` with `v[1]` is of course `swap(v[0], v[1])`,
optionally preceded by `using std::swap`;
see ["What is the `std::swap` two-step?"](/blog/2020/07/11/the-std-swap-two-step/) (2020-07-11).
But look at all these _wrong_ things you can try!

|:---------------------------|:---------:|:------:|:------------:|
|                            | libstdc++ | libc++ | MSVC         |
|:---------------------------|:---------:|:------:|:------------:|
| `swap(v[0], v[1]);`        | OK        | OK     | OK           |
| `swap<>(v[0], v[1]);`      | error     | OK     | error        |
| `std::swap(v[0], v[1]);`   | OK        | OK     | OK pre-C++20 |
| `std::swap<>(v[0], v[1]);` | error     | OK     | OK pre-C++20 |
| `swap(v[0], {});`          | OK        | error  | error        |
| `swap<>(v[0], {});`        | error     | error  | error        |
| `std::swap(v[0], {});`     | OK        | error  | error        |
| `std::swap<>(v[0], {});`   | error     | error  | error        |
|:---------------------------|:---------:|:------:|:------------:|
{:.smaller}

I found it interesting that `std::swap(v[0], v[1])` doesn't
compile on Microsoft's STL. This is totally fine, according
to my reading of the paper standard; I even think it's a good thing;
but it still surprises me that they can get away with it. Also,
it's an error only in MSVC's `-std:c++latest` mode; it compiles
fine in `-std:c++17` mode. I haven't tried to track down why.

UPDATE, 2021-06-29: [Casey Carter explains](https://old.reddit.com/r/cpp/comments/o9e8fh/implementation_divergence_on_swapping_bools/h3eq6jx/)
that Microsoft's `std::_Vb_reference<T>` has a hidden-friend `swap` in
both C++17 and C++20. The trick is that MSVC's `-permissive` mode permits
many non-conforming extensions, and one of those extensions is
that hidden friends aren't actually hidden against qualified lookup.
So in MSVC's `-permissive` mode, a qualified call to `std::swap(v[0], v[1])`
successfully finds the hidden friend. Finally, `-permissive` is
MSVC's default prior to C++20; but when you turn on `-std:c++latest`,
it implicitly turns off `-permissive`.
Passing `-std:c++17 -permissive-` rejects the qualified call ([Godbolt](https://godbolt.org/z/cq3ffTPnM)),
and `-std:c++latest -permissive` accepts it.

----

I also found it amusing that `std::swap(v[0], {})`, on GNU libstdc++,
right now has the same codegen as `std::exchange(v[0], {})`: its physical
effect is to write `false` into `v[0]`. However, at the source level what's
actually happening is

    _Bit_reference br = {};
    // and then swap the two _Bit_reference instances:
    bool temp = v[0];
    v[0] = static_cast<bool>(br);
    br = temp;

and the way `_Bit_reference::operator bool` is implemented is as a
dereference-and-mask ([see](https://gcc.gnu.org/onlinedocs/libstdc++/libstdc++-html-USERS-4.2/stl__bvector_8h-source.html#l00080)):

    return !!(*_M_p & _M_mask);

A default-constructed `_Bit_reference` has `_M_p == nullptr` and `_M_mask == 0`,
so we dereference null and then bitwise-AND the result with zero. Both GCC and Clang
cleverly observe that anything ANDed with zero is zero, which prevents them from
making the even cleverer observation that the null dereference has undefined behavior.

----

The expressions of the form `swap<>(...)` rely on a syntax change in C++20,
which is already correctly implemented by all of GCC, Clang, and MSVC as far as I can tell.

Prior to C++20, `swap<` would have been parsed as a function template only if
unqualified lookup found a function template named `swap` in the current scope.
Which in this case we have not got. So this expression is simply a parse error
in C++17, regardless of what you put in the `...` part.

But C++20 (specifically,
[P0846 "ADL and Function Templates that are not Visible"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0846r0.html),
subsequently refactored by
[P1787 "Declarations and where to find them"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1787r6.html))
changed the rules. Now

> A `<` is interpreted as the delimiter of a _template-argument-list_
> if it follows a name that is not a _conversion-function-id_ and
>
> - that follows the keyword `template` or a `~` after a _nested-name-specifier_ or in a class member access expression, or
> - for which name lookup finds the _injected-class-name_ of a class template or finds any declaration of a template, or
> - that is an unqualified name for which name lookup either finds one or more functions or finds nothing, or
> - that is a terminal name in a _using-declarator_, a _declarator-id_, or a [type-only context](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0634r3.html) other than a _nested-name-specifier_.

Our `swap<` falls into the third category above: unqualified name lookup on `swap` finds nothing,
so we treat the `<` as an angle bracket and continue parsing. Of course if you later introduce
a variable called `swap` into the current scope, your call to `swap<>` will no longer parse; but
that's not too strange in practice.

The C++20 rules do introduce yet another context where it matters _what kind_ of entity is found
by name lookup; so we can construct super contrived examples like, say, [this](https://godbolt.org/z/jW8377rrj):

    namespace N {
        enum E { A };
        constexpr struct NTTP {} nttp;
        static bool operator<(int(int), NTTP) { return false; }
        template<NTTP> bool f(E) { return true; }
    }

    static int f(int x) { return x+1; };

    int main() {
        return f<N::nttp>(N::A);  // Is `f` a function?
    }

This program returns `1`, because the ADL call to `N::f<N::nttp>` returns `true`.
But if you change the definition of `::f` to

    static auto f = [](int x) { return x+1; };

then the program returns `0` instead. Because, when `f` is not a function,
`f<N::nttp>(N::A)` parses as a comparison, equivalent to

    return (f < N::nttp) > N::A;

and `false > N::A` is false.
