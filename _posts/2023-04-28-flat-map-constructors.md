---
layout: post
title: "Fun with `flat_map`'s non-explicit constructors"
date: 2023-04-28 00:01:00 +0000
tags:
  constructors
  cpplang-slack
  flat-containers
  initialization
  library-design
  standard-library-trivia
excerpt: |
  Earlier this month I wrote: ["Most constructors should be explicit"](/blog/2023/04/08/most-ctors-should-be-explicit/) (2023-04-08).
  This week, while writing unit tests for libc++ `flat_map` (now available [on Godbolt!](https://godbolt.org/z/so1KK9vbv))
  I encountered the following quirk of `flat_map`'s constructor overload set:

      void print_map(std::flat_map<int, int>);

      print_map({ {1, 2, 3}, {10, 20, 30} });
      print_map({ {1, 2},    {10, 20} });
      print_map({ {1},       {10} });
      print_map({ {},        {} });

  As currently specified, `flat_map` has [twenty-six](https://eel.is/c++draft/flat.map.defn) constructors;
  the resolution of [LWG3802](https://cplusplus.github.io/LWG/issue3802) will add another twelve. The two
  constructors relevant to this puzzle are:

      flat_map(initializer_list<value_type>,
               const key_compare& = key_compare());

      flat_map(key_container_type, mapped_container_type,
               const key_compare& = key_compare());

  Our `flat_map`'s `value_type` is `pair<const int, int>`; its `key_container_type` and
  `mapped_container_type` are both `vector<int>`. Before you read on, try to guess the
  elements of each of the four maps above. All four lines compile; none of them is an error.
---

{% raw %}
Earlier this month I wrote: ["Most constructors should be explicit"](/blog/2023/04/08/most-ctors-should-be-explicit/) (2023-04-08).
This week, while writing unit tests for libc++ `flat_map` (now available [on Godbolt!](https://godbolt.org/z/so1KK9vbv))
I encountered the following quirk of `flat_map`'s constructor overload set:

    void print_map(std::flat_map<int, int>);

    print_map({ {1, 2, 3}, {10, 20, 30} });
    print_map({ {1, 2},    {10, 20} });
    print_map({ {1},       {10} });
    print_map({ {},        {} });

As currently specified, `flat_map` has [twenty-six](https://eel.is/c++draft/flat.map.defn) constructors;
the resolution of [LWG3802](https://cplusplus.github.io/LWG/issue3802) will add another twelve. The two
constructors relevant to this puzzle are:

    flat_map(initializer_list<value_type>,
             const key_compare& = key_compare());

    flat_map(key_container_type, mapped_container_type,
             const key_compare& = key_compare());

Our `flat_map`'s `value_type` is `pair<const int, int>`; its `key_container_type` and
`mapped_container_type` are both `vector<int>`. Before you read on, try to guess the
elements of each of the four maps above. All four lines compile; none of them is an error.

----

Recall that any constructor without the `explicit` keyword is a candidate for the implicit conversion
from a braced initializer list; and a viable constructor taking `std::initializer_list` will be
considered the best match for such a conversion (but only if it's viable).

    std::flat_map<int, int> fm = { {1, 2, 3}, {10, 20, 30} };

Here the compiler sees that `{1, 2, 3}` isn't a valid initializer for a `pair`, so the
`initializer_list` constructor isn't viable. Instead, we'll make two `vector<int>`s,
and then implicitly convert that couple-of-vectors to a `flat_map` of size 3.
The puzzle's third line behaves similarly:
`{1}` isn't a valid initializer for a `pair`, so we'll make two one-element vectors
and convert them to a `flat_map` of size 1.

But the second line does something different! `{1, 2}` _is_ a valid initializer for a `pair`,
so `{{1, 2}, {10, 20}}` is treated as an `initializer_list<pair>` instead
of `(vector<int>, vector<int>)`. As a result, _this_ `flat_map` is initialized with the
pairs `{1, 2}` and `{10, 20}` — that is, `fm.keys()` is `{1, 10}` and `fm.values()` is `{2, 20}`.

Finally, the fourth line: You might think that `{}` isn't a valid initializer for a `pair`,
since, after all, it doesn't contain two elements. But in fact `std::pair`'s default constructor
is non-explicit, so it's available for uniform initialization from `{}`! When we're expecting
a `pair<int, int>`, `{}` is valid and treated as equivalent to a default-constructed pair,
i.e. `{0, 0}`. So:

    std::flat_map<int, int> fm = { {}, {} };
    assert(fm.size() == 1);  // namely {0,0}

    std::flat_multimap<int, int> fmm = { {}, {} };
    assert(fmm.size() == 2); // namely two copies of {0,0}

----

This puzzle shows the mess you can get into by not following the simple guideline to make
your constructors `explicit` by default. The STL doesn't follow this guideline, so we end up
with silly things like `{}` being a valid `pair` and `{ ks, vs }` being a valid `flat_map`.

> You might say that the STL's original sin in this case was to give a single type two dozen
> constructors. I wouldn't fight you on that one. Still, each of the constructors seems useful
> enough in isolation.

If the STL followed [our guidelines](/blog/2023/04/08/most-ctors-should-be-explicit/#a-stab-at-a-complete-guideline) —
which, to be crystal clear, it _does not now_ and
_will never_, because backwards compatibility; but _if it did_ — then we'd see the following
behavior instead:

- A `pair` always has two elements, never zero, so `{}` shouldn't be a valid initializer for a `pair`.
    `pair`'s zero-argument constructor would be `explicit`.
    If you wanted a default-constructed pair of ints, you'd have to write explicitly either
    `std::pair<int, int>()` or `{0, 0}`.

- A `flat_map` is conceptually a sequence of pairs, not a pair of containers, so
    `{key_cont, mapped_cont}` shouldn't be a valid initializer for a `flat_map`.
    `flat_map`'s two-argument constructor would be `explicit`.
     See ["Is your constructor an object-factory or a type-conversion?"](/blog/2018/06/21/factory-vs-conversion/) (2018-06-21):
     `flat_map`'s two-argument constructor is clearly an object-factory.

- As before, `{{1, 10}, {2, 20}}` would be a valid initializer for a `flat_map` whose elements
    were those two pairs. But `{{}, {}}` wouldn't be a valid initializer for `flat_map` at all
    (because it would have to mean "a map whose elements are `{}` and `{}`," and `{}` is no longer
    a valid initializer for a pair). Likewise, `{{1}, {2}}` and `{{1, 2, 3}, {10, 20, 30}}`
    wouldn't be valid because their elements aren't key-value pairs.

- As before, `flat_map<int, int>({~~~}, {~~~})` would be a valid initializer for a `flat_map`
    constructed from two vectors, no matter how many elements you put in for “`~~~`.” In particular,
    `flat_map<int, int>({}, {})` would still construct an empty `flat_map`. And since
    ordinary constructor syntax (with parentheses) doesn't trigger `initializer_list` constructors,
    `flat_map<int, int>({1, 2}, {10, 20})` would still construct a `flat_map` containing the
    key-value pairs `{1, 10}` and `{2, 20}`.

In tabular form:

| Construct                           | Actual STL               | Guideline-following library |
|:------------------------------------|:-------------------------|:----------------------------|
| `pair(int, int)`                    | implicit                 | implicit (by [point 3](/blog/2023/04/08/most-ctors-should-be-explicit/#whenever-std-tuple_size_v-a-exis)) |
| `pair()`                            | implicit                 | explicit                    |
| `vector(initializer_list<int>)`     | implicit                 | implicit (by [point 2](/blog/2023/04/08/most-ctors-should-be-explicit/#a-std-initializer_list-t-should)) |
| `vector()`                          | implicit                 | implicit (by point 2)       |
| `flat_map(initializer_list<pair>)`  | implicit                 | implicit (by point 2)       |
| `flat_map()`                        | implicit                 | implicit (by point 2)       |
| `flat_map(vector, vector)`          | implicit                 | explicit                    |
| `pair<int, int> p = {}`             | `{0,0}`                  | error                       |
| `FM fm = {}`                        | `{}`                     | `{}`                        |
| `FM fm = {{}, {}}`                  | `{{0,0}}`                | error                       |
| `FM fm = {{1}, {10}}`               | `{{1,10}}`               | error                       |
| `FM fm = {{1,2}, {10,20}}`          | `{{1,2},{10,20}}`        | `{{1,2},{10,20}}`           |
| `FM fm = {{1,2,3}, {10,20,30}}`     | `{{1,10},{2,20},{3,30}}` | error                       |
| `auto fm = FM({}, {})`              | `{}`                     | `{}`                        |
| `auto fm = FM({1}, {10})`           | `{{1,10}}`               | `{{1,10}}`                  |
| `auto fm = FM({1,2}, {10,20})`      | `{{1,10},{2,20}}`        | `{{1,10},{2,20}}`           |
| `auto fm = FM({1,2,3}, {10,20,30})` | `{{1,10},{2,20},{3,30}}` | `{{1,10},{2,20},{3,30}}`    |
{:.smaller}

Again I'll stress: You should expect the C++ STL to continue making all its constructors implicit
(and suffering the consequences), for historical reasons. But hopefully this example shows
how _following_ the rule leads to simpler, clearer code with fewer "gotchas." A well-designed
library won't lend itself to puzzle-making.
{% endraw %}
