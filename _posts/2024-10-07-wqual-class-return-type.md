---
layout: post
title: "Why don't compilers warn for `const T f()`?"
date: 2024-10-07 00:01:00 +0000
tags:
  antipatterns
  c++-style
  compiler-diagnostics
  llvm
excerpt: |
  The other day I got an intriguing question from [Sándor Dargó](https://www.sandordargo.com/).
  In ["`const` all the things?"](/blog/2022/01/23/dont-const-all-the-things/#return-types-never-const) (2022-01-23)
  I had written:

  > Returning “by const value” is always wrong. Full stop.

  Sándor writes: "I was wondering — given that it's really the case,
  [even the Core Guidelines says so](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f49-dont-return-const-t),
  and it seems to be easy to identify — do you know why
  we don't have compiler warnings for such return types?"
---

The other day I got an intriguing question from [Sándor Dargó](https://www.sandordargo.com/).
In ["`const` all the things?"](/blog/2022/01/23/dont-const-all-the-things/#return-types-never-const) (2022-01-23)
I had written:

> Returning “by const value” is always wrong. Full stop.

Sándor writes: "I was wondering — given that it's really the case,
[even the Core Guidelines says so](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines#f49-dont-return-const-t),
and it seems to be easy to identify — do you know why
we don't have compiler warnings for such return types?"

Now, GCC and Clang [do diagnose](https://godbolt.org/z/cnW7sPKf1)
const-qualified _primitive_ return types,
under the `-⁠Wignored-qualifiers` flag. It's only const-qualified _class_ return types
that compilers seem to struggle with. Why?

It's not because const-qualified class types are difficult to diagnose. Compilers
actually have to go out of their way to avoid diagnosing `const string f()`, given
that they have decided that they want to diagnose `const int f()`.

In fact, looking at Clang's codepaths for this diagnostic, I discovered that
their out-of-its-way code caused a bug:
C++20 equally deprecated `volatile string f()` and `volatile int f()`, but
[Clang trunk fails to diagnose the former](https://github.com/Quuxplusone/llvm-project/issues/39),
because they added the deprecation check [in the wrong place](https://github.com/Quuxplusone/llvm-project/commit/03ecd7e8d27afede512fb3031953008728c4e75d)!

----

Why don't compilers diagnose "return (a class type) by const value" by default, under `-⁠Wall` or `-⁠Wextra`?
I suspect that it's because there would be too much noise. That is, it would find mainly true positives,
but so many of them, in code that basically works anyway, that users would get annoyed.
And why would there be so many positives? Because Scott Meyers recommended the pattern, decades ago!
So it's probably gotten into many third-party libraries that are a pain to change.

## Field-testing `-Wqual-class-return-type`

I [modified my Clang fork](https://github.com/Quuxplusone/llvm-project/commit/37913b77d4a9ecece55de148e24a2726efc1eb8e)
to diagnose `cv string f()` (and incidentally also `cv T f()` where `T` is a dependent type)
under the warning flag `-Wqual-class-return-type`, and compiled my employer's codebase with it.
([Godbolt.](https://godbolt.org/z/1oKnMEd7W))

> Interested in upstreaming this patch? Send me an email!

As expected, this new warning found a lot of positives. Out of the first 77,000 LOC compiled, 14 lines needed to change.
In every case, the only cv-qualifier present was `const`. I believe all of them were true positives,
in the sense that the appropriate fix was always to do something with that bogus cv-qualifier:

- replace `const T f()` with `T f()`
- replace `const T f()` with `const T& f()`
- replace `const ptr<T> f()` with `ptr<const T> f()`
- replace `T const f()` with `T f() const` (yes, we found one of these!)

The closest I found to a reason someone might write `const T f()` is... well,
it's exactly what Scott Meyers was thinking of, back in 2005, when he recommended
"return by const value." We had something like this immutable `Map` type:

    struct Map {
        const string operator[](int i) const;
    };
    Map m;

    m[123] = "abc";

With the `const`, that assignment expression is ill-formed (as desired).
If we remove the `const`, then the assignment becomes well-formed — but it doesn't
actually modify `m[123]`! It assigns a new value to the temporary `string`
returned from `operator[]`.

I still removed that bogus `const`. But an even better fix in this specific case
might be to rename `operator[]` to `at`, since I don't think people
would _expect_ `m.at(123) = "abc"` to work. That is, this is Murphy's Law in action:
we had been fiddling with the return type in order to deal with a misunderstanding
that could happen (and thus "will happen"), but we would have done better to
make the misunderstanding impossible!

> Clang/LLVM's own codebase also generates a lot of warnings,
> [all of which look like true positives to me](https://github.com/Quuxplusone/llvm-project/commit/b5a4e527cf82ed85631fa25ff2699a7e91184c16).


## Two quirks exposed by this diagnostic

Here's a fun one:

    const S f();
    S g();

    auto& r = f();
      // OK: `auto` is `const S`; `const S&` binds to the rvalue

    auto& s = g();
      // Ill-formed: `auto` is `S`; `S&` cannot bind to the rvalue

This did come up in one place in our codebase.
I can't imagine any code actually depends on this quirk in any non-trivial way.
The fix is simple: write `const auto& s` instead.

----

Will Wray informs me of another quirk tangentially related to "return by const value"
([Godbolt](https://godbolt.org/z/jYa4GjEx3)):

    struct Tie {
      int& i;

      template<class Other>
      void operator=(const Other& rhs) const { i = rhs.i; }
    };

    const Tie f(int& i) { return {i}; }
    Tie g(int& i) { return {i}; }

Here `f` triggers the new diagnostic; we should write `g` instead.
"But," says the programmer, "I need to write `f`!" Why?
"Because my code uses assignments of this form:

    int i, j;
    f(i) = f(j);

and `g(i) = g(j)` refuses to compile!"

That's because your `Tie` has more assignment operators than we can see. The core language believes that
`Tie::operator=<Tie>`, being a template instantiation, is never a copy-assignment operator.
(This is the crux of the `Plum` example in
["Types that falsely advertise trivial copyability"](/blog/2024/05/15/false-advertising/) (2024-05-15), too.)
Therefore, the language will generate a defaulted copy-assignment operator:

      Tie& operator=(const Tie&) = default;

This candidate is non-viable when the left-hand side is const-qualified, but when the
left-hand side is non-const, as in `g(i) = g(i)`, this candidate becomes viable —
and the best match. We choose it... only to discover that
it's actually defaulted-as-deleted because `Tie` has a data member of reference type
([[class.copy.assign]/7.2](https://eel.is/c++draft/class.copy.assign#7))!

The fix here, of course, is to give `Tie` a proper non-defaulted copy-assignment operator.

    struct Tie {
      int& i;

      void operator=(const Tie& rhs) const { i = rhs.i; }

      template<class Other>
      void operator=(const Other& rhs) const { i = rhs.i; }
    };

Although Will had his reasons, I think `Tie`'s original sin was to have had a data member of reference type in the first place.
See Lesley Lai's ["The implication of const or reference member variables"](https://lesleylai.info/en/const-and-reference-member-variables/) (September 2020).

Neither of these quirks are reasons _not_ to enable `-Wqual-class-return-type` in your
own codebase; they're just interesting corner cases that you might encounter.

---

See also:

* ["`const` is a contract"](/blog/2019/01/03/const-is-a-contract/) (2019-01-03)
