---
layout: post
title: "A not-so-quick introduction to the C++ allocator model"
date: 2023-06-02 00:01:00 +0000
tags:
  allocators
  relocatability
  value-semantics
---

My paper [P1144R8 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r8.html)
has, in the past few months, been joined by three(!) papers out of Bloomberg:

- [P2786R1](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2786r1.pdf) "Trivial relocatability options" (Gill, Meredith)
- [P2839R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2839r0.html) "Nontrivial relocation via a new owning reference type" (Bi, Berne)
- [P2814R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2814r0.pdf) "Comparing P1144 with P2786" (Gill, Meredith, O'Dwyer)

The reason for this sudden interest is that trivial relocatability turns out to have some
surprising interactions with one of Bloomberg's pet interests: the polymorphic allocator model,
known as "[PMR](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#pmr)."
In [tomorrow's post](/blog/2023/06/03/p1144-pmr-koans/) I'll display
some of those surprising interactions, but today let's lay the groundwork by simply describing
the current allocator model and its customization points.

----

Throughout this post, I'll be referring to "`std::pmr`" or "PMR,"
but it's important to keep in mind that Bloomberg's own implementation
of the standard library (the "[BSL](https://github.com/bloomberg/bde/tree/5c9fbe8/groups/bsl/bslstl)")
does not use `std::pmr`! Instead, the BSL defines
[their equivalent of `std::allocator`](https://github.com/bloomberg/bde/blob/5c9fbe8/groups/bsl/bslma/bslma_stdallocator.h#L58-L69)
to be a stateful, non-propagating allocator à la `std::pmr::polymorphic_allocator`,
so that their `std::vector` is PMR-like by default. You must go far out of your way,
at Bloomberg, to encounter a container that works the same way as in the ordinary C++ STL.

The basic idea is that instead of pulling memory allocations from one big global heap,
PMR lets us create smaller local memory arenas as well. When we create an object (in the classical OOP sense),
we decide which memory arena it'll be associated with. Every allocation related to that
object will come from that same memory arena. If the object contains a `vector<string>`, the vector's
dynamically allocated buffer will come from the object's arena. If it's a `vector<string>`,
then each string in the vector will allocate from the object's arena. And so on.

> The Standard Library uses the term ["memory resources"](https://eel.is/c++draft/mem.res#class.general-1.sentence-1)
> in place of "arenas," but I won't do so in this post, to avoid confusion with "resource management."
> The Resources that are Allocated in RAII might be allocated _from_ an arena (a "memory resource"),
> but they're not arenas ("memory resources") themselves.

In the polymorphic allocator model, "transferring ownership" of a memory allocation
is possible only for objects within the same arena. If object `O1` lives in arena `A1`,
and object `O2` lives in arena `A2`, then an "assignment" like `O1 = std::move(O2);` cannot transfer
any pointers from `O2` into `O1` — they point into the wrong arena! The assignment must
allocate a fresh copy of `O2`'s data in arena `A1`. This completely nerfs C++11 move semantics...
but that's fine, because move semantics are a subset of value semantics, and what we're doing
here _isn't_ value semantics. In PMR-world, we're not concerned with objects' Platonic _values_ but with
their _identities,_ just like in classical OOP.

I believe in this context `O1 = O2;` is a solecism: in a perfect world it would
be written in the classical "sit-`O1`-in-memory-and-poke-at-it" syntax, `O1.populateFrom(O2)`,
and `O2` would advertise itself as immobile rather than pretending to be movable when it's not really.
But then the STL would need two different `vector`-like classes with two completely different APIs,
instead of being able to reuse the same template for both. `std::vector<T,A>` doesn't
change its API depending on whether `A` is `std::allocator` or a stateful PMR-style allocator;
it merely changes that API's _meaning_.

For more information, see
[this Bloomberg GitHub wiki page on the BDE Allocator Model.](https://github.com/bloomberg/bde/wiki/BDE-Allocator-model)
(However, I've been informed it's pretty out-of-date.)

<table class="smaller">
<tr><th>Ordinary C++</th><th>C++17 PMR</th><th>BSL</th></tr>
<tr><td>(no equivalent)</td><td><code>std::pmr::memory_resource</code></td><td><code>bslma::Allocator</code></td></tr>
<tr><td>(the global heap)</td><td><code>std::pmr::get_default_resource()</code></td><td><code>bslma::Default::allocator(nullptr)</code></td></tr>
<tr><td>(no equivalent)</td><td><code>std::pmr::monotonic_buffer_resource</code></td><td><a href="https://github.com/bloomberg/bde/blob/5c9fbe8/groups/bdl/bdlma/bdlma_bufferedsequentialallocator.h#L8"><code>bdlma::BufferedSequentialAllocator</code></a></td></tr>
<tr><td><code>std::allocator&lt;T&gt;</code></td><td><code>std::pmr::polymorphic_allocator&lt;T&gt;</code></td><td><a href="https://github.com/bloomberg/bde/blob/5c9fbe8/groups/bsl/bslma/bslma_stdallocator.h#L479-L495"><code>bsl::allocator&lt;T&gt;</code></a></td></tr>
<tr><td>(no need)</td><td><code>std::uses_allocator_v&lt;C, std::pmr::memory_resource*&gt;</code></td><td><code>bslma::UsesBslmaAllocator&lt;C&gt;</code></td></tr>
<tr><td><code>std::vector&lt;T&gt;</code></td><td><code>std::pmr::vector&lt;T&gt;</code></td><td><code>bsl::vector&lt;T&gt;</code></td></tr>
<tr><td><code><pre>std::vector<std::string> v;
v.push_back("hello world, for example");
</pre></code></td><td><code><pre>char buf[10'000];
auto mr = std::pmr::monotonic_buffer_resource(buf, sizeof(buf));
auto v = std::pmr::vector<std::pmr::string>(&mr);
v.push_back("hello world, for example");
assert(v[0].get_allocator().resource() == &mr);
</pre></code></td><td><code><pre>char buf[10'000];
auto mr = bdlma::BufferedSequentialAllocator(buf, sizeof(buf));
auto v = bsl::vector<bsl::string>(&mr);
v.push_back("hello world, for example");
assert(v[0].get_allocator().mechanism() == &mr);
</pre></code></td></tr>
</table>

> Before we go any further, you should also note and internalize the subtle inefficiency of
> the naïve PMR and BSL code snippets above. `vector::push_back` here takes an rvalue reference to a `pmr::string`
> temporary, which we constructed using the converting constructor from `const char*` and the
> default memory arena (the global heap). Then, `push_back` emplaces a copy of that temporary
> into the vector, using `string`'s allocator-extended move constructor. This copies the characters
> from the default memory arena into `v`'s own memory arena (`mr`). Finally, the original
> temporary is destroyed, freeing the default arena's allocation.
>
> For performance, we could write `v.emplace_back("hello...")`, which eliminates
> the temporary, albeit at the cost of [template bloat](/blog/2021/03/03/push-back-emplace-back/).
> Alternatively, `v.push_back(std::pmr::string("hello...", &mr))`, which
> makes the temporary use the same arena as `v`.
>
> The "Ordinary C++" snippet has no such inefficiency, because the temporary string and the
> vector share the same memory arena: the global heap.


## `allocator_traits::construct`

Recall the difference between `push_back(x)` and `emplace_back(args...)`: the former uses `X`'s
move or copy constructor to insert the new element, and the latter uses `X(args...)`. You could
imagine that `emplace_back` does a placement-new:

    void *where = &data_[size_];
    ::new (where) value_type(std::forward<Args>(args)...);
    size_ += 1;

In fact it adds a layer of indirection instead:

    using AT = std::allocator_traits<allocator_type>;
    AT::construct(get_allocator(), where, std::forward<Args>(args)...);

`allocator_traits::construct(a, where, args...)` will call `a.construct(where, args...)`
if possible; otherwise it'll just placement-new. For most allocators — including
`std::allocator` — it'll placement-new. But an allocator's author is allowed to make
their `construct` method do whatever they want.

In the STL, both `pmr::polymorphic_allocator<T>` and
[`scoped_allocator_adaptor<Allocs...>`](https://en.cppreference.com/w/cpp/memory/scoped_allocator_adaptor)
provide non-trivial `construct` methods.

## Allocator-extended constructors

Recall that `pmr::polymorphic_allocator<T>` wants to enforce the invariant that each "level"
of a `vector<vector<vector<...>>>` uses the same arena. It achieves this by intercepting every
object-construction — via `construct` — and adding itself as an extra constructor argument.
If `std::allocator` does basically this:

    template<class U, class... Args>
    static void construct(U *where, Args&&... args) {
        ::new (where) U(std::forward<Args>(args)...);
    }

then `std::pmr::polymorphic_allocator` does basically this:

    template<class U, class... Args>
    void construct(U *where, Args&&... args) const {
        if constexpr (uses_allocator_v<U, decltype(*this)>) {
            ::new (where) U(std::forward<Args>(args)..., *this);
        } else {
            ::new (where) U(std::forward<Args>(args)...);
        }
    }

Here I've omitted some arcane details of [uses-allocator construction](https://en.cppreference.com/w/cpp/memory/uses_allocator#Uses-allocator_construction).
In real life, `polymorphic_allocator::construct` uses
[`std::make_obj_using_allocator`](https://en.cppreference.com/w/cpp/memory/make_obj_using_allocator),
which takes care of those arcane details; but if I just showed you the real-life code below,
you'd probably say "Hey! That doesn't clarify anything!"

    template<class U, class... Args>
    void construct(U *where, Args&&... args) const {
        ::new (where) U(std::make_obj_using_allocator<U>(*this, std::forward<Args>(args)...));
    }

---

Every allocator-aware class type in C++ provides two versions of each of its constructors:
the ordinary user-facing constructor, and an "allocator-extended" constructor
for use by `std::make_obj_using_allocator`. Each constructor invariably comes in both versions,
even the special ones like the default constructor.

    struct W {
      using allocator_type = A;

      explicit W(); // default ctor
      explicit W(allocator_type); // allocator-extended default ctor

      W(int); // converting ctor from int
      explicit W(int, allocator_type); // allocator-extended version

      W(W&&); // move ctor
      explicit W(W&&, allocator_type); // allocator-extended move ctor
    };

The first version is used by ordinary code:

    W w1;
    W w2 = 42;
    W w3 = std::move(w2);

The second version is used by `std::make_obj_using_allocator`; its purpose
is to create the `W` object and associate it with a specific memory arena.

    std::vector<W, A>
    v.emplace_back(); // constructs W(v.get_allocator())

That's a lot of machinery to produce a simple outcome: Allocator `A` can
(if it likes) ensure that `v[0]` invariably gets constructed with the
same allocator — the same memory arena — as `v` itself.

Notice that `std::allocator` doesn't care to do this; it has no state to
propagate downward. And [`std::scoped_allocator_adaptor`](https://en.cppreference.com/w/cpp/memory/scoped_allocator_adaptor)
has a different goal: it wants to associate each level with a _different_
allocator. Other (third-party) allocators may have yet different goals.


### Sideways propagation

Finally, C++ allocators can propagate not only downward but sideways — from
one container object to another — as if the allocator were part of the container's
value. This applies to all the value-semantic operations that take two arguments:
copy-assignment, move-assignment, and swap. For historical reasons, each of these
three operations is controlled by [its own individual trait](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#pocca-pocma-pocs-soccc);
but really they come as a package deal.

If your allocator propagates on container copy-assignment, move-assignment, and swap,
then it enables traditional move semantics: you can always pilfer the pointers
from the right-hand container, because you're pilfering the allocator right along
with it. On the other hand, this means you don't care about the state of your original
allocator — one allocator is just as good as another — which means your
allocators are effectively stateless, like `std::allocator`.

Since PMR allocators carry important state (the identity of the arena), they're not
interchangeable and therefore don't propagate. They're "sticky." Remember, PMR is
for classical OOP objects: an OOP object sits in one place its whole life, and its
allocator sits right there with it. Assigning into a `std::pmr::vector` with `=`
doesn't change its arena.

> You might ask, what happens if I swap two `pmr::vector`s with different arenas?
> Well, technically that's undefined behavior. In practice every library vendor will
> [swap the pointers](https://godbolt.org/z/bP9WaTdeM) but keep the arenas the same,
> which causes heap corruption when the first vector goes out of scope and asks its
> arena to deallocate a pointer that never came from that arena in the first place.
> This is [widely regarded as a bad move](https://cplusplus.github.io/LWG/issue2153).


### Allocator is not salient state

["Normative Language to Describe Value Copy Semantics"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2479.pdf) (Lakos, 2007)
defined _salient state_ as the stuff inside an object that contributes to its _value_.
The _value_ is the abstract thing that gets copied by the copy operations, compared
by the comparison operators (if any), and so on. For example, the `size` of a vector
is a salient attribute; but the `capacity` of a vector is non-salient.

Is the arena of a `pmr::vector` salient state? No, because it is not copied by
the copy operations. (If you copy-construct a `pmr::vector`, [you get](https://en.cppreference.com/w/cpp/memory/allocator_traits/select_on_container_copy_construction)
the default arena. If you copy-assign a `pmr::vector`, the `size` is copied over
with the rest of the _value_, but the allocator remains unchanged.)

> In fact, it's pretty obvious in present-day C++ that `pmr::vector<int>` isn't a
> value-semantic type at all, unless you add some preconditions on its non-salient state.
> For example, two value-semantic objects of the same type can be swapped; but
> swapping two arbitrary `pmr::vector`s is UB. We may say that `pmr::vector` behaves
> value-semantically as long as all relevant objects come from the same arena as
> `pmr::get_default_resource()`.

This causes PMR no end of growing pains as we evolve the value-semantic
parts of C++. Even [N2479](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2479.pdf)
predated and was partly obsoleted by C++11 move semantics.

For example, [P1825](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1825r0.html) changed the behavior of
[this snippet](https://godbolt.org/z/jPh6Yf7qj) between C++17 and C++20:

    auto mr1 = std::pmr::monotonic_buffer_resource();
    auto &mr2 = *std::pmr::get_default_resource();
    std::pmr::string a[2] = {
      std::pmr::string("hello", &mr1),
      std::pmr::string("world", &mr1),
    };
    std::vector<std::pmr::string> v;
    std::transform(
      std::make_move_iterator(a),
      std::make_move_iterator(a + 2),
      std::back_inserter(v),
      [](auto&& r) { return r; }
    );
    #if __cplusplus >= 202000
    assert(v[0].get_allocator().resource() == &mr1);
    #else
    assert(v[0].get_allocator().resource() == &mr2);
    #endif

Nobody (not even me!) noticed this change as it happened. I'd like to think
that even if we had noticed it, we wouldn't have altered course.

In the next post, we'll see several ways that PMR's lack of value semantics
can bite us when we try to apply value-semantic operations — like
relocation! — to PMR objects.

* ["P1144 PMR koans"](/blog/2023/06/03/p1144-pmr-koans/) (2023-06-03)
