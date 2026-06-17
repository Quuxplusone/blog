---
layout: post
title: "C++ idiom of the day: `arrow_proxy`"
date: 2019-02-06 00:02:00 +0000
tags:
  flat-containers
  library-design
  metaprogramming
  pearls
  proxy-references
excerpt: |
  I'm currently working on a reference implementation of
  [P0429R6 `flat_map`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0429r6.pdf).
  Here's a C++ metaprogramming idiom I learned in the process.
---

{% raw %}
I'm currently working on a reference implementation of
[P0429R6 `flat_map`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p0429r6.pdf).
Here's a C++ metaprogramming idiom I learned in the process.

Consider a `zip_range` that refers to two other containers and lets you
iterate them in parallel:

    std::vector<int> keys;
    std::vector<int> values;
    for (auto&& kv : make_zip_range(keys, values)) {
        std::cout << kv.first << ": " << kv.second << "\n";
        kv.second += 1;
    }

> Notice that in C++17 we can use CTAD and structured bindings to rewrite this as
>
>     for (auto&& [k, v] : zip_range(keys, values)) {
>         std::cout << k << ": " << v << "\n";
>     }
>
> I don't _recommend_ it, but you can _do_ it.

We can implement most of `zip_range` easily:

    template<class Keys, class Values>
    class zip_range {
        using KeyIterator = typename Keys::iterator;
        using ValueIterator = typename Values::iterator;

        Keys *keys_;
        Values *values_;
    public:
        using iterator = zip_iterator<KeyIterator, ValueIterator>;

        iterator begin();
        iterator end();

        // boilerplate and const versions omitted
    };

    template<class KeyIterator, class ValueIterator>
    class zip_iterator {
        KeyIterator kit_;
        ValueIterator vit_;
    public:
        using KeyType = typename KeyIterator::value_type;
        using ValueType = typename ValueIterator::value_type;

        using iterator_category = std::common_type_t<
            nonstd::iterator_category_t<KeyIterator>,
            nonstd::iterator_category_t<ValueIterator>
        >;
        using difference_type = ptrdiff_t;
        using value_type = std::pair<KeyType, ValueType>;

...But now we hit a stumbling block! What should our `reference` type be?
This is the type returned by `operator*`, which is also the type of `auto&& kv`
in our sample code above. So it needs to be something with `.first` and `.second`
members, but somehow `kv.second` must refer to a particular element in `values`!
This gives us the right idea:

        using reference = std::pair<KeyType&, ValueType&>;

        reference operator*() const {
            return {*kit_, *vit_};
        }

In fact, to deal correctly with `vector<bool>`, we should use the
member typedefs provided by `KeyIterator` and `ValueIterator`:

        using KeyReference = typename KeyIterator::reference;
        using ValueReference = typename ValueIterator::reference;
        using reference = std::pair<KeyReference, ValueReference>;

        reference operator*() const {
            return {*kit_, *vit_};
        }

So far so good.


## The real stumbling block: `operator->`

In the preceding paragraph, we succeeded at making this snippet work:

    std::vector<int> keys = ...;
    std::vector<int> values = ...;
    auto zit = make_zip_range(keys, values).begin();
    int& x = (*zit).first;
    assert(&x == &keys.front());

Now we hit the _real_ issue, which is what I really want to talk about!

    int& x = zit->first;
    assert(&x == &keys.front());

Recall the way an overloaded `operator->` works in C++. For a standard smart pointer,
it would look like this:

    T *operator->() const {
        return rawptr_;
    }

That is, `int& x = zit->first` will be evaluated the same as if we'd written

    decltype(auto) temp = zit.operator->();
    int& x = temp->first;

So we need our `operator->` to return a pointer to a type with a `first` member.
Basically, we need to return a `pair<KeyReference, ValueReference>*`. Okay, but
where does that `pair` object live? Who controls its lifetime?

The crazy unsafe way to solve our problem is:

    using reference = std::pair<KeyReference, ValueReference>;
    using pointer = reference*;

    reference operator*() const {
        return {*kit_, *vit_};
    }

    pointer operator->() const {
        static reference r;
        r = {*kit_, *vit_};
        return &r;
    }

This is unsafe because if we wrote something like `use(zit->first, (zit+1)->first)`
we'd have undefined behavior: two modifications of `r` without an intervening sequence point.

> Note that the above snippet actually won't compile because `reference` is not
> default-constructible, but [we can fix that](https://wandbox.org/permlink/5iIj56yesxdOxB1L)
> by using `std::optional<reference> r`.


## C++ weirdness to the rescue!

Notice that our expansion of `zit->first` is expressed
in terms of `temp->first`. That is, `operator->` is in some sense "recursive": if our
`zip_iterator`'s overloaded `operator->` returns an object with an overloaded `operator->`,
then the compiler will call _that_ `operator->`, and so on, all the way down, until
it (hopefully) bottoms out at a call to the built-in `->` that works on native pointers.

Therefore a safe, but still crazy, way to solve our problem is:

    using reference = std::pair<KeyReference, ValueReference>;
    using pointer = std::unique_ptr<reference>;

    reference operator*() const {
        return {*kit_, *vit_};
    }

    pointer operator->() const {
        return std::make_unique<reference>(*kit_, *vit_);
    }

[This works](https://wandbox.org/permlink/SpvizOPuBeggNuzr)
because `zit->first` evaluates `zit.operator->()` to produce a `unique_ptr temp1`,
and then evaluates `temp1.operator->()` to produce a raw pointer `reference *temp2`, and finally
evaluates `temp2->first` to produce a `KeyReference`. However, if we do it this way,
every single use of `zit->first` causes a heap allocation! This is certainly not okay.
([Godbolt.](https://godbolt.org/z/QkL_tv))

So what we do is a little trick called an _arrow proxy_. It looks like this:

    using reference = std::pair<KeyReference, ValueReference>;
    using pointer = arrow_proxy<reference>;

    reference operator*() const {
        return {*kit_, *vit_};
    }

    pointer operator->() const {
        return pointer{{*kit_, *vit_}};
    }

where `arrow_proxy` is defined as this seven-line helper struct:

    template<class Reference>
    struct arrow_proxy {
        Reference r;
        Reference *operator->() {
            return &r;
        }
    };

[This works](https://wandbox.org/permlink/ewGDMfRV91FTCzX5) because `zit->first`
evaluates `zit.operator->()` to produce an `arrow_proxy<reference> temp1`, and then
evaluates `temp1.operator->()` to produce a raw pointer to the `pair` stored inside
the `arrow_proxy` object itself. The `arrow_proxy`, just like the `unique_ptr` in
our previous example, lives just long enough to make this code work — it lives
until the end of the _full-expression_ containing the `->` operator, and then
is destroyed, taking the `pair<KeyReference, ValueReference>` with it.
([Godbolt shows perfect codegen.](https://godbolt.org/z/FZA0P9))


## Non-const `operator->()`

> UPDATE, 2023-04-30: As of C++23, proxy reference types such as
> [`vector<bool>::reference`](https://en.cppreference.com/w/cpp/container/vector_bool/reference#std::vector.3Cbool.3E::reference::operator.3D),
> [`pair<K&, V&>`](https://en.cppreference.com/w/cpp/utility/pair/operator%3D),
> and [`tuple<Ts&...>`](https://en.cppreference.com/w/cpp/utility/tuple/operator%3D)
> are required to support a const-qualified `operator=`. So this whole section
> is obsolete; you can mark your `arrow_proxy::operator->() const` without fear,
> as long as you use C++23 or later ([Godbolt](https://godbolt.org/z/Mxa1Kbjva)).

Notice that in our finished `arrow_proxy`, the `operator->()` is a non-const member function.
This goes against everything we've been taught! (See
["`const` is a contract" (2019-01-03)](/blog/2019/01/03/const-is-a-contract/).)
Normally, `operator->` represents the "dereferencing" operation, and you don't
need to modify an iterator in order to dereference it. That's why
`zip_iterator::operator->() const` is declared `const`.

Yet [Godbolt shows](https://godbolt.org/z/KMmA4e) that if we add `const` to `arrow_proxy::operator->()`,
we get weird compiler errors. This is because on all three major library implementations,
`vector<bool>::reference::operator=(bool)` is a non-const member function! [UPDATE: Until C++20,
that is.]

> We could eliminate the compiler error by making `r` a `mutable` member of
> `arrow_proxy`, but in my opinion that cure would be worse than the disease.

My intuition here is that `zip_iterator::operator->() const` is properly declared `const`
because it represents the "dereferencing" operation; but `arrow_proxy::operator->()`
arguably can be declared non-const because it doesn't "represent" any semantic operation. An
`arrow_proxy` is not a pointer-like thing that is semantically "dereferenced."
We needn't bother trying to make it "const-correct" according to _semantic_ rules,
because it _has no semantics;_ it's nothing but an invisible implementation detail of `zip_iterator`.

So there you are. When implementing `zip_iterator`, you're going to need an `arrow_proxy`.

----

Various implementations of the `arrow_proxy` idiom — not all 100% conforming to my
recommendations here — can be found in
[`pybind11`](https://github.com/pybind/pybind11/blob/ccbe68b084806dece5863437a7dc93de20bd9b15/include/pybind11/pytypes.h#L638-L645),
[Paul Fultz's `hmr`](https://github.com/pfultz2/hmr/blob/3966a4a94f76a94c681a0dba9ec64bcaabf09919/include/hmr/detail/operators.hpp#L18-L55),
[Ryan Haining's `cppitertools`](https://github.com/ryanhaining/cppitertools/blob/af1e317864baeb7dee913b7219ffe4382ed885c7/internal/iterbase.hpp#L171-L186),
[Boost `iterator_facade`](https://www.boost.org/doc/libs/1_69_0/boost/iterator/iterator_facade.hpp)
(which calls it `operator_arrow_dispatch::proxy`),
[Boost `iterator_archetypes`](https://www.boost.org/doc/libs/1_69_0/boost/iterator/iterator_archetypes.hpp),
and
[SG14 `flat_map`](https://github.com/Quuxplusone/SG14/blob/master/include/sg14/flat_map.h).
{% endraw %}
