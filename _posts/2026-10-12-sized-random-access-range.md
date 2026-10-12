---
layout: post
title: "Is a sized random-access range always common?"
date: 2026-10-12 00:01:00 +0000
tags:
  library-design
  proposal
  ranges
  type-erasure
---

[P3411R6 `any_view`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2026/p3411r6.html#proposed-design)
proposes for C++29 a design for a type-erased Ranges view. The idea (as usual with type erasure) is
to permit replacing templated code like this:

    template<std::ranges::range R>
    void f(R&& rg) {
      bool b = std::ranges::is_heap(rg, std::greater<>());
      assert(b);
    }

    int a[] = {1,2,3,4,5};
    std::vector<int> v = {1,2,3,4,5};
    f(a);
    f(v);
    f(std::views::iota(1, 6));
    f(std::vector<int>{1,2,3,4,5});

with non-templated code like this:

    // This is not a function template!
    void g(std::ranges::any_view<int, std::ranges::any_view_options::random_access, int> rg) {
      bool b = std::ranges::is_heap(rg, std::greater<>());
      assert(b);
    }

    g(a);
    g(v);
    g(std::views::iota(1, 6));
    g(std::vector<int>{1,2,3,4,5});

Because there are so many different flavors of range, `any_view` takes a few template parameters
to specify what kind of range you mean — just like `std::function` takes a function signature as
a template parameter to specify what kind of function you mean. In the example above, we specify
that `g` takes a type-erased copy of a _random-access_ view _of a range of `int`_. The third template
parameter forces the copy's `reference` type to `int` (as opposed to `int&` or `const int&`), for the
benefit of `views::iota`.

`any_view` inherits from `std::ranges::view_interface,` so it gets many convenient
member functions for free. For example,

    using enum std::ranges::any_view_options;
    void g(std::ranges::any_view<int, random_access, int> rg) {
      assert(rg.front() == 1);
    }

because `view_interface` provides [`front`](https://en.cppreference.com/cpp/ranges/view_interface/front)
whenever its iterator category is forward-or-better.
Recently on [std-proposals](https://lists.isocpp.org/std-proposals/2026/10/19191.php) it was
noted that `back` is different:

    void h(std::ranges::any_view<int, random_access | sized, int> rg) {
      assert(rg.back() == 5);  // error
    }

You might think `rg.back()` would be defined as equivalent to `rg[rg.size()-1]` — perfectly
well-defined for a _sized random-access_ range as we have here — but in fact it's defined as
`*prev(rg.end())`, which is well-defined only for _common bidirectional_ ranges. (Recall that a _common_ range
is one where `end()` returns the same iterator type as `begin()`, so that `prev(end())` is legal. For a general,
non-common, range, `end()` is allowed to return a sentinel that doesn't support arithmetic.)

---

The problem is already addressed inside the `views::common` adaptor itself. Ordinarily, to make a non-common
range into a `common_view`, you basically have to turn every iterator into a variant — a
[`common_iterator<I, S>`](https://eel.is/c++draft/iterators.common) — that can hold either an iterator
_or_ a sentinel, and you represent `views::common(rg).end()` as `common_iterator<I, S>(rg.end())`.
Since a `common_iterator` value might hold a sentinel, `common_iterator` itself can't support decrement;
it is always at best a forward iterator. _But,_ in the special case where `rg` is a sized random-access range,
`views::common(rg)` does _not_ use `common_iterator`: in that case `views::common(rg).end()` is represented
as `rg.begin() + rg.size()`. [Godbolt](https://godbolt.org/z/W9rjxh93f):

    struct UnsizedRA {
      int *begin();
      std::unreachable_sentinel_t end();
    };
    UnsizedRA r1;
    std::forward_iterator auto it1 = std::views::common(r1).begin();
    static_assert(sizeof(it1) > sizeof(int*));

    struct SizedRA {
      int *begin();
      std::unreachable_sentinel_t end();
      int size() const;
    };
    SizedRA r2;
    std::same_as<int*> auto it2 = std::views::common(r2).begin();

So, from the C++ user's perspective, the way to deal with the absence of `any_view::back()`
in this odd corner case is simply to feed your sized random-access `any_view` to `views::common`:

    void h(std::ranges::any_view<int, random_access | sized, int> rgin) {
      auto rg = std::views::common(std::move(rgin));
      static_assert(std::random_access_range<decltype(rg)>);
      static_assert(std::sized_range<decltype(rg)>);
      assert(rg.back() == 5);  // OK
    }

---

[LWG 4003](https://cplusplus.github.io/LWG/issue4003) asked whether we should "improve" the
definition of `view_interface::back()` so that it'd work if _either_ of the expressions
`*prev(rg.end())` or `rg[rg.size()-1]` were valid. However, that direction was rejected, because
it was rightly pointed out that while this would solve the inconsistency in the API of `view_interface`
(and therefore `any_view`), it wouldn't solve the basic inconsistency that right now the C++ user
can create a range type (like `SizedRA` above) that is

- random-access,
- sized,
- but not common.

Unfortunately, the library can't exactly _forbid_ such a type. At best it could amend some concept
definitions so that `SizedRA` would fail to be a `sized_range` — but that seems silly, because
`SizedRA` definitely _does_ provide `size()`, and failing to recognize that fact would just
silently pessimize a bunch of Ranges algorithms.

> I have the same pet peeve about `random_access_range`'s requiring `operator[]`. If you forget
> to provide it, then your type just quietly degrades to a bidirectional range, pessimizing
> some algorithms but not necessarily failing to compile.

Alternatively — but this is a crazy idea — the library might amend the definition of
[`ranges::end(rg)`](https://eel.is/c++draft/range.access#end) to return a different type in this case,
so that `SizedRA` would become a common range! Given the current wording:

> Given a subexpression E with type `T`, let `t` be an lvalue that denotes the reified object for `E`. Then:
> - [...] if `T` is an array type, `ranges::end(E)` is expression-equivalent to `t + extent_v<T>`.
> - Otherwise, if `auto(t.end())` is a valid expression whose type models `sentinel_for<iterator_t<T>>`, then `ranges::end(E)` is expression-equivalent to `auto(t.end())`.
> - Otherwise, if `T` is a class or enumeration type and `auto(end(t))` is a valid expression
>     whose type models `sentinel_for<iterator_t<T>>` [...] then `ranges::end(E)` is expression-equivalent to that expression.

we might remove the first bullet point and then add one at the end like:

- [...] if `ranges::begin(t) + ranges::size(t)` is a valid expression whose type
    is `iterator_t<T>`, then `ranges::end(E)` is expression-equivalent to that expression.

I say "remove the first" because `ranges::size(t)` is already equivalent to `extent_v<T>` when `T` is an array type;
this amendment would just generalize the array case. Core-language arrays are already a kind of type
that provide a `begin` (sort of) but no iterator-returning `end` (in fact, no `end` at all); this amendment
would handle other kinds of ranges that provide a `begin` but no iterator-returning `end`.

I say "add one at the end" because [`ranges::size`](https://eel.is/c++draft/range.access#range.prim.size-2.5) itself
already has a bullet point defining `ranges::size(rg)` in terms of `ranges::end(rg) - ranges::begin(rg)`, and
we don't want a circular dependency there. The bullet point in `ranges::size` basically enforces that any
_random-access common_ range is by definition _sized_ (as long as you go through `ranges::size` instead of `t.size()`);
the proposed bullet point here would enforce that any _random-access sized_ range is by definition _common_
(as long as you go through `ranges::end` instead of `t.end()`).

One obvious problem — and I bet there are also less obvious ones — is that the amendment above would make

    struct NotCurrentlyARange {
      int *begin();
      int size();
    };

into a Ranges `range`, even though it wouldn't be iterable by a core-language `for`-loop. The
mismatch between core-language ranges and library Ranges is already arguably a problem: see
["When `ranges::for_each` behaves differently from `for`"](/blog/2024/12/09/foreach-versus-for/) (2024-12-09).
Let's not make the existing problem worse!

---

Since piping a sized random-access range through `views::common` already Does The Right Thing, and since
trying to use `back()` on an `any_view` is already a super niche problem, I'm inclined to think the
status quo is perfectly acceptable. But, at the same time, since we can't forbid types like `SizedRA` at the language level,
and since trying to "fix" them at the level of `ranges::end` is crazy, and since `view_interface::back`
admittedly doesn't work quite right... why _not_ just fix `view_interface::back` in the way
[LWG 4003](https://cplusplus.github.io/LWG/issue4003) originally proposed?
