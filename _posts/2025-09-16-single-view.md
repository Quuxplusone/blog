---
layout: post
title: "PSA: `views::single` doesn't really view"
date: 2025-09-16 00:01:00 +0000
tags:
  cpplang-slack
  library-design
  pitfalls
  ranges
  sum-types
excerpt: |
  Someone on the [cpplang Slack](https://cppalliance.org/slack/) asks:
  How can I view a `std::pair<T, T>` as if it were a range of two `T`s?
  That is, fill in the blank in this sample program:

      template<std::ranges::range R>
      void increment_all(R&& rg) {
        for (auto&& elt : rg) {
          elt = elt + 1;
        }
      }

      template<class T>
      auto F(std::pair<T, T>& kv) { ~~~~ }

      int main() {
        std::pair<int, int> kv = {1, 2};
        increment_all(F(kv));
        assert(kv.first == 2 && kv.second == 3);
        std::ranges::fill(F(kv), 4);
        assert(kv.first == 4 && kv.second == 4);
      }
---

Someone on the [cpplang Slack](https://cppalliance.org/slack/) asks:
How can I view a `std::pair<T, T>` as if it were a range of two `T`s?
That is, fill in the blank in this sample program:

    template<std::ranges::range R>
    void increment_all(R&& rg) {
      for (auto&& elt : rg) {
        elt = elt + 1;
      }
    }

    template<class T>
    auto F(std::pair<T, T>& kv) { ~~~~ }

    int main() {
      std::pair<int, int> kv = {1, 2};
      increment_all(F(kv));
      assert(kv.first == 2 && kv.second == 3);
      std::ranges::fill(F(kv), 4);
      assert(kv.first == 4 && kv.second == 4);
    }

The most obvious solution, unfortunately, relies on undefined behavior: while it is
true that there is no physical padding between `kv.first` and `kv.second`, C++
does not consider the two objects to form an "array" and therefore we cannot just
use pointer arithmetic to step from one to the other. That is, we cannot just do this:

    template<class T>
    auto F(std::pair<T, T>& kv) {
      return std::span<T>(&kv.first, 2);
    }

Both GCC and Clang diagnose the UB here if you constant-evaluate
`increment_all` ([Godbolt](https://godbolt.org/z/x8rPf363K)). Compilers are
(on paper) required to track all potential UB at constexpr time; for pointer-related
UB in particular they're very good at doing so. This is a double-edged sword for
use-cases like ours: it means that the obvious solution works fine at runtime,
but we must seek a different solution if we want it to work at constexpr time.

---

Since the two objects don't currently belong to the same range, let's put each of them
into a one-element range and use `views::concat` to concatenate them.
That first step sounds deceptively like a use-case for C++20's `views::single(x)`:

    template<class T>
    auto F(std::pair<T, T>& kv) {
      return std::views::concat(std::views::single(kv.first), std::views::single(kv.second));
    }

Unfortunately, _this doesn't work at all!_
After calling `increment_all(F(kv))`, [we find](https://godbolt.org/z/x9x4j6rqE)
that the values stored in `kv` have not changed. The problem:

> `views::single(x)` doesn't view.

Oh, its return type models C++20's `concept view` all right;
but only by exploiting the same technicality that allows us to write

    auto rg = std::views::all(std::vector<int>{1,2,3});
    static_assert(std::ranges::view<decltype(rg)>);

The data from the argument (`kv.first` in the former case; that rvalue vector in the latter case)
is _copied_ into the "view" object (`ranges::single_view<int>` in the former;
`ranges::owning_view<vector<int>>` in the latter). These "views" don't _view_, but _wrap_ —
into something the Standard calls a [_movable-box_](https://en.cppreference.com/w/cpp/ranges/copyable_wrapper.html).
Their implementation is essentially this:

    template<class ElementType>
    struct single_view {
      ElementType t_;
      ElementType *begin() const { return &t_; }
      ElementType *end() const { return &t_ + 1; }
    };

    template<class RangeType>
    struct owning_view {
      RangeType t_;
      auto begin() const { return t_.begin(); }
      auto end() const { return t_.end(); }
    };

So when you write “`std::views::single(x)`,” you're not actually saying "give me a view of object `x`."
You're saying "make _another copy of `x`'s value_ and pass it around masquerading as a view."
There is no connection at all between this "view" and your original object `x`.

> Could we use `std::views::single(std::ref(kv.first))`? Yes, but that would give us a range
> of `reference_wrapper`s, which isn't substitutable for the range of `int` we actually want.
> For example, `std::ranges::fill(F(kv), 3)` would not compile. ([Godbolt.](https://godbolt.org/z/xnMEEdzxj))

---

So it seems to me the best C++26 solution is the tedious construction of a
single-element `span` ([Godbolt](https://godbolt.org/z/K9z3x4c8s)):

    template<class T>
    auto F(std::pair<T, T>& kv) {
      return std::views::concat(
        std::span<int, 1>(&kv.first, 1),
        std::span<int, 1>(&kv.second, 1)
      );
    }

Replacing `<int, 1>` with just `<int>` (or omitting it altogether and relying on CTAD) is safe,
but doubles `sizeof(F(kv))` from 16 bytes to 32.

---

`views::concat` is new in C++26. In C++20 and C++23, you might try substituting `views::join`,
which takes a _range of ranges_ and concatenates its element ranges. (One downside is that
`views::join` [cannot produce](https://eel.is/c++draft/range.join#iterator-1) a random-access range;
it gives a bidirectional range instead.)
But you must be careful about lifetimes:

    template<class T>
    constexpr auto F(std::pair<T, T>& kv) {
      std::array<std::span<int, 1>, 2> a = {
        std::span<int, 1>(&kv.first, 1),
        std::span<int, 1>(&kv.second, 1)
      };
      return std::views::join(std::move(a));
    }

is safe, but

    template<class T>
    constexpr auto F(std::pair<T, T>& kv) {
      std::array<std::span<int, 1>, 2> a = {
        std::span<int, 1>(&kv.first, 1),
        std::span<int, 1>(&kv.second, 1)
      };
      return std::views::join(a);
    }

yields undefined behavior ([Godbolt](https://godbolt.org/z/j1saWG44j)). The reason is that `views::join`
actually _does_ behave like a proper view adaptor: it does _not_ make a copy of its argument range `a` when
`a` is an lvalue (because [Ranges uses value category as a proxy for lifetime](/blog/2023/08/13/non-const-iterable-ranges/#a-third-example-of-a-non-const-i)),
and so the latter returns a dangling reference. In the former case, `std::move(a)` is an rvalue non-view range,
so `views::join` wraps it in a `ranges::owning_view` just like `views::all` did in our previous section —
and that happens to be what we want here. We _want_ the returned `join_view` to hold within itself a
materialized `std::array<std::span<int, 1>, 2>`.

> We got bitten because we tried to return a view from `F`, and the view contained dangling references.
> In fact the very idea of function `F`, all the way from the very beginning of this post, violates the
> cardinal rule that view types are to be treated as "parameter-only types." In principle we shouldn't
> even be trying to return a view from `F`, any more than we would return a `string_view` from a function.
> And so I can't blame `views::join` for biting us here — it's our own fault, really.
> `views::join` is just the messenger. We were incredibly lucky to make it this far into the post
> before getting bitten by lifetime issues!

---

Also new in C++26 are [P2988](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2988r12.pdf) `optional<T&>`,
and (unless [P3168](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3168r2.html)
is removed at the eleventh hour, which IMHO is still conceivable)
the ability to treat any `optional` as a zero-or-one-element range. We could use that here.
The runtime performance penalty is slightly higher (since `optional` is nullable
while `span<int, 1>` isn't), but the aesthetics are significantly cleaner.

    template<class T>
    constexpr auto F(std::pair<T, T>& kv) {
      return std::views::concat(
        std::optional<int&>(kv.first),
        std::optional<int&>(kv.second)
      );
    }

As of this writing, no library vendor yet ships all the C++26 pieces
needed by this snippet. libstdc++ has `concat` but not `optional<T&>`; [my P1144 fork of libc++](https://godbolt.org/z/Tf7onn9zs)
has `optional<T&>` but not `concat`. (Microsoft, and libc++ trunk, have neither piece.)

## Suggested implementation

If I needed this in a real production codebase, I'd implement just a reference-semantic `single_view`,
leaving my caller to deal with `views::concat`:

    namespace my {
      template<class T>
      constexpr std::span<T, 1> single_view(T& t) {
        return std::span<T, 1>(&t, 1);
      }
    } // namespace my

Then the caller, using another rising C++26 feature, might write:

    auto& [...parts] = kv;
    auto rg = std::views::concat(my::single_view(parts)...);

...Except that that C++26 feature currently works only inside templates ([Godbolt](https://godbolt.org/z/e5rx9Pe1o)).
But that's a story for another day.

## Takeaways

- You should know that some "views" actually wrap copies of their arguments. This _frequently_ happens when the argument is an rvalue.
- PSA: `views::single` never views; it always copies.
- There is a gap in the design space for a properly reference-semantic single-view.
- `span<T, 1>` provides a reference-semantic single-view with awkward syntax. C++26's iterable `optional<T&>` may yield improved syntax, at a slight runtime cost.
