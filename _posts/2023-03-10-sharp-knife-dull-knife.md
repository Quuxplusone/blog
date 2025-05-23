---
layout: post
title: "Should the compiler sometimes reject a `[[trivially_relocatable]]` warrant?"
date: 2023-03-10 00:01:00 +0000
tags:
  language-design
  library-design
  relocatability
  wg21
excerpt: |
  This is the third in a series of at least three weekly blog posts. Each post will explain
  one of the problems facing [P1144 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)
  and [P2786R0 "Trivial relocatability options"](https://isocpp.org/files/papers/P2786R0.pdf) as
  we try to (1) resolve their technical differences and (2) convince the rest of the C++ Committee
  that these resolutions are actually okay to ship.

  Today's topic is under what circumstances the compiler should "overrule" a `[[trivially_relocatable]]` warrant
  applied by the programmer.
---

This is the third in a series of at least three weekly blog posts. Each post
([I](/blog/2023/02/24/trivial-swap-x-prize/),
[II](/blog/2023/03/03/relocate-algorithm-design/),
[III](/blog/2023/03/10/sharp-knife-dull-knife/),
[IV](/blog/2023/06/03/p1144-pmr-koans/))
will explain
one of the problems facing [P1144 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)
and [P2786R0 "Trivial relocatability options"](https://isocpp.org/files/papers/P2786R0.pdf) as
we try to (1) resolve their technical differences and (2) convince the rest of the C++ Committee
that these resolutions are actually okay to ship.

Today's topic is under what circumstances the compiler should "overrule" a `[[trivially_relocatable]]` warrant
applied by the programmer.


## What is a warrant?

The feature defined in P1144 (and P2786R0) has two sides: There's the _query_ or _type-trait_ side, which lets the
programmer of a library function _query_ whether a template parameter type `T` is trivially relocatable or not;
and there's the _warrant_ or _attribute_ side, which lets the programmer of a library type _warrant_ that a specific
type (under their control) is in fact trivially relocatable, in cases where the compiler can't figure that out on its own.

The closest existing analogue to P1144's system is C++11 `noexcept`. It has both a _query_ side (you can ask
`noexcept(T::f())` for an arbitrary expression `T::f()`) and a _warrant_ side (you can declare your own `A::f() noexcept`).
Notice that the compiler treats the `noexcept` warrant with the utmost respect: you can write

    void f() noexcept { throw 42; }

Despite the "obvious" contradiction, the compiler will accept this code and report that `noexcept(f()) == true`.
Also, the compiler can infer the correct warrant in simple cases: `A(const A&) = default;` will be recognized by the
compiler as noexcept, without the programmer's needing to use the keyword. Explicit manual warrants are _only rarely_
needed.

For comparison, the existing notion of "trivial copyability" has a _query_ side (you can ask whether `is_trivially_copyable_v<T>`),
but it has no _warrant_ side: If the compiler doesn't infer that your type is trivially copyable, then there's no
way for you to convince it otherwise.

The Itanium ABI's notion of "trivial for purposes of ABI" has a _warrant_ side (you can mark your type
[`[[clang::trivial_abi]]`](/blog/2018/05/02/trivial-abi-101/)), but no _query_ side (as far as I know,
there is no way to ask Clang whether an arbitrary type `T` is trivial for purposes of ABI or not).

In C++ pre-’20, iterator types also followed this model: you could _query_ the category of an iterator with
`iterator_traits<T>::iterator_category`, and you could _warrant_ the category of your own iterator by setting
`T::iterator_category` appropriately. The STL would generally respect your warrant: if it claimed
`bidirectional_iterator_tag`, you'd get the bidirectional versions of STL algorithms, even if your
type didn't satisfy the requirements of a bidirectional iterator in detail.

> In C++20, the Ranges STL distrusts user-provided warrants by default: a type that explicitly warrants
> `bidirectional_iterator_tag` can be sent down `forward_iterator` codepaths if it's not careful. [Godbolt.](https://godbolt.org/z/YaeMGo95W)

A few lucky features are purely syntactic. Consider _three-way comparability._ You can query a type's
comparability with `std::three_way_comparable<T>`, and you can make your own type three-way comparable by
giving it an `operator<=>`. I wouldn't call that a _warrant_, though, because it doesn't stand for anything
beyond itself. There's no (sensible) way for a type to provide `<=>` and not _actually_ be comparable, in the
way that a function can call itself `noexcept` but _actually_ throw, or call itself `bidirectional_iterator_tag`
but _actually_ fail to support bidirectional operations.

Here's an example of a type that calls itself `[[trivially_relocatable]]` but is not _actually_ trivially relocatable:

    struct [[trivially_relocatable]] BadVector {
        std::array<int, 10> data_;
        const int *end_ = data_.data();
        BadVector(BadVector&& rhs) : data_(rhs.data_), end_(data_.data() + rhs.size()) {}
        void operator=(BadVector&& rhs) { data_ = rhs.data_; end_ += rhs.size() - size(); }
        int size() const { return end_ - data_.data(); }
    };

Both P1144 and P2786 permit `BadVector` to compile, simply because there's no way to forbid it.
This code is well-formed as long as you (and any libraries you call) never attempt to relocate `BadVector`;
if you do, the behavior is simply undefined. ([P2786R0 §9.2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2786r0.pdf):
"If a user explicitly (and erroneously) marks as trivially relocatable a class with an invariant that
stores a pointer into an internal structure, then relocation will typically result in UB."
[P1144R6 §5.5](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1144r6.html#wording-dcl.attr.trivreloc):
"If a class type is declared with the `trivially_relocatable` attribute, and the program relies on observable
side-effects of [its] relocation other than a copy of the underlying bytes, the behavior is undefined.")


> Reading this post will be easier if you keep in mind the distinction between "a type that <b>is</b>
> trivially relocatable" and "a type that <b>the compiler knows</b> is trivially relocatable." P1144 exists
> because today the former set is vastly larger than the latter. But P1144 only shrinks that gap;
> it doesn't close the gap entirely. By definition, P1144's `std::is_trivially_relocatable_v` identifies
> only types that <b>the compiler knows</b> are trivially relocatable. It will always be possible to
> create a type that <b>is</b> (in principle) trivially relocatable without the compiler's <b>knowing</b>
> that it is.
>
> The following `GoodVector` is (in principle) not only trivially relocatable but
> trivially copyable; yet the compiler doesn't recognize it as either.
> ([Godbolt.](https://godbolt.org/z/WP5hjjsd6))
>
>     struct GoodVector {
>         std::array<int, 10> data_;
>         boost::interprocess::offset_ptr<const int> end_ = data_.data();
>         GoodVector(GoodVector&& rhs) : data_(rhs.data_), end_(data_.data() + rhs.size()) {}
>         void operator=(GoodVector&& rhs) { data_ = rhs.data_; end_ += rhs.size() - size(); }
>         int size() const { return end_ - data_.data(); }
>     };


## What are warrants for, in practice?

Recall that simple Rule-of-Zero class types don't need warrants; the compiler can detect whether a Rule-of-Zero type
is trivially relocatable just like it can detect whether it's trivially copyable. So 99% of your types won't
need (and therefore _shouldn't use_) explicit warrants. Here's an example of a type that doesn't need any
warrant.

    struct UniqueFunc {
        std::unique_ptr<Base> ptr_;
        template<class T> UniqueFunc(T t) : ptr_(std::make_unique<Derived<T>>(std::move(t))) {}
        void operator()() const { ptr_->call(); }
    };
    static_assert(std::is_trivially_relocatable_v<UniqueFunc>);

P1144 calls such types _naturally trivially relocatable_; P2786R0 §7.1 calls them _implicitly trivially relocatable_.

There are only two times I can think of when you do need a warrant. One is when your type isn't Rule-of-Zero,
but you know (you _warrant_) that it is in fact trivially relocatable. For example:

    struct [[trivially_relocatable]] UniquePtr {
        int *p_;
        explicit UniquePtr(int *p) : p_(p) {}
        UniquePtr(UniquePtr&& p) : p_(p.p_) { p.p_ = nullptr; }
        void operator=(UniquePtr&& p) { p_ = p.p_; p.p_ = nullptr; }
        ~UniquePtr() { delete p_; }
    };

    struct [[trivially_relocatable]] GoodVector {
        // as above
    };

The other time is when your type _is_ Rule-of-Zero, but the compiler can't tell that it's trivially
relocatable because one of its members is of a third-party type that you — but not the compiler —
know is trivially relocatable. For example:

    struct [[trivially_relocatable]] BoostedFunc {
        boost::movelib::unique_ptr<Base> ptr_;
        template<class T> BoostedFunc(T t) : ptr_(new Derived<T>(std::move(t))) {}
        void operator()() const { ptr_->call(); }
    };

As humans, we know that `boost::movelib::unique_ptr<Base>` can be trivially relocated;
but the compiler doesn't know it, because the author of `boost::movelib` didn't mark it
with the attribute. (Unsurprising, since the attribute doesn't exist yet. :) This will
be one of warrants' major use-cases for years, if not decades, after the proposal is adopted.)

The "purist" approach is to leave your `BoostedFunc` warrantless;
sacrifice the performance benefit in the short term; and submit a patch to the authors of
`boost::movelib` to add warrants upstream. Then, sometime after upstream accepts your patch,
you can upgrade your third-party code and finally get that performance improvement you've been waiting for.
But that's a complicated plan that doesn't appeal to the average working programmer, compared
to "add the warrant to `BoostedFunc`, maybe file a feature request upstream, and move on."


## What types are definitely never trivially relocatable?

Are there certain kinds of types where, even if a warrant is applied, the compiler should
step in and say "Whoa, you can't possibly mean that!"?

Certain kinds of types, such as
[types with vtables](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1144r6.html#non-trivial-sample-polymorphic),
mustn't be considered _naturally_ trivially relocatable. But if the programmer puts an explicit warrant
on the type, is there ever a time when the compiler should overrule their decision?


### Non-movable, non-destructible types?

P1144R6 (but not P2786R0, and not P1144R7) would reject a `[[trivially_relocatable]]` warrant
applied to types:

- that aren't move-constructible at all, or

- that aren't destructible at all.

Since P1144 defines relocation as "moving from A to B, then destroying A," it seemed obvious that
a non-movable non-destructible type can't be relocatable (at all, let alone trivially so).

However, P2786 points out that "trivially copyable" isn't a kind of "copyable": the following
`NC` is trivially copyable without being either copy-constructible or destructible.
([Godbolt.](https://godbolt.org/z/qE4TqWfKP))

    struct NC {
        int i;
        NC();
        NC(const NC&) = delete;
        NC& operator=(const NC&) = delete;
        ~NC() = delete;
    };
    static_assert(std::is_trivially_copyable_v<NC>);

So why should "trivially relocatable" be a kind of "relocatable"?

I'm still leery of this idea, possibly because "relocate" is such a new verb.
If we can have types that are "trivially relocatable" without actually being relocatable,
then what does that imply for library facilities like `std::vector`?

    struct [[trivially_relocatable]] NR {
        explicit NR(int);
        NR(NR&&) = delete;
        NR& operator=(NR&&) = delete;
    };

    std::vector<NR> v;
    v.emplace_back(42);             // OK?
    v.reserve(v.capacity() + 1);    // OK?

`emplace_back` and `reserve` will in practice use trivial relocation for types that are
trivially relocatable; does that mean that vendors will _in practice_ support the calls
above, even though they're not blessed by the Standard? Or, worse, will adopting this
notion of trivial relocatability lead to the Standard changing its specification of `vector`
to _force_ vendors to support these "relocate-only" use-cases? That sounds like work,
and non-backward-compatible feature work at that, which is antithetical to P1144's whole
direction.

We can look to `std::copy` for precedent. [As we discussed last week](/blog/2023/03/03/relocate-algorithm-design/),
`copy` will use `memmove` for trivially copyable types. But it won't use `memmove`
if you try to `std::copy` from one array of `NR` to another — it'll simply fail to compile.

This suggests that it is reasonable for P1144R7 to permit non-movable non-destructible types to be
`is_trivially_relocatable` — as long as the library users of that trait are carefully written
never to assume that `is_trivially_relocatable<T>` implies `is_move_constructible<T>`, just like
they're carefully written today never to assume that `is_trivially_copyable<T>` implies
`is_copy_assignable<T>`.

In particular, P1144's library algorithms `std::relocate` and `std::relocate_at` aren't
intended to magically work for non-move-constructible types. They don't need to SFINAE away,
but they aren't required to `memmove` things that can't be move-constructed normally.


### Types with non-trivially-relocatable data members?

Look again at our `GoodVector` and `BadVector` examples from above.

    struct GoodVector {
        std::array<int, 10> data_;
        boost::interprocess::offset_ptr<const int> end_ = data_.data();
        GoodVector(GoodVector&& rhs) : data_(rhs.data_), end_(data_.data() + rhs.size()) {}
        void operator=(GoodVector&& rhs) { data_ = rhs.data_; end_ += rhs.size() - size(); }
        int size() const { return end_ - data_.data(); }
    };

    struct BadVector {
        std::array<int, 10> data_;
        const int *end_ = data_.data();
        BadVector(BadVector&& rhs) : data_(rhs.data_), end_(data_.data() + rhs.size()) {}
        void operator=(BadVector&& rhs) { data_ = rhs.data_; end_ += rhs.size() - size(); }
        int size() const { return end_ - data_.data(); }
    };

`GoodVector` is a trivially relocatable type (in the practical, Platonic sense); `BadVector` is not.
But if you were to crack them open and look inside —

- `boost::interprocess::offset_ptr<const int>` itself is not trivially relocatable, even though `GoodVector` is

- `const int*` itself is trivially relocatable, even though `BadVector` is not

This is why both P1144 and P2786 place the `[[trivially_relocatable]]` annotation on the class type
as a whole, rather than framing it as a property of one member (e.g. the move constructor) in isolation.
Trivial relocatability is a holistic property of the whole type taken together.

Now, [P2786R0 §7.6](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2786r0.pdf)
proposes that if the programmer attaches the `[[trivially_relocatable]]` annotation to a class
type with any non-trivially-relocatable data member, the compiler should reject that annotation — either
noisily (e.g. by emitting an error) or quietly (by quietly ignoring the attribute). This would affect
types such as our `GoodVector` (because `interprocess::offset_ptr` isn't t.r.) and `BoostedFunc`
(because `movelib::unique_ptr`, while t.r. in theory, is not recognized as such by the compiler).
P1144 of course doesn't propose this wrinkle.

Very early in P1144's history, when I implemented the first Clang patch, there was some discussion
on [the review](https://reviews.llvm.org/D50119) of precisely P2786R0's design; in fact, I implemented
that design in my fork under the name `[[clang::maybe_trivially_relocatable]]`. The semantics of
`[[maybe_trivially_relocatable]]` are essentially the same as `[[trivially_relocatable(bool)]]`:
The code

    template<class T>
    struct [[maybe_trivially_relocatable]] X {
        T t;
    };

is exactly equivalent to P1144's

    template<class T>
    struct [[trivially_relocatable(std::is_trivially_relocatable_v<T>)]] X {
        T t;
    };

But `[[maybe_trivially_relocatable]]` by itself is a "dull knife" —
it's strictly less useful than the "sharp knife" of `[[trivially_relocatable]]`. Specifically, without a "sharp" version of
`[[trivially_relocatable]]` there's no way to implement either `GoodVector` or `BoostedFunc`.
And besides being less practically useful, a dull knife is also [dangerous](https://blog.knife-depot.com/knife-myths-dull-knives-are-safer-than-sharp-knives/):

> The main reason why a dull knife is more dangerous is that it requires the
> wielder to use significantly more force when cutting than a sharp knife.
>
> [...]
>
> The safety of a knife really depends on the person using it
> and not the blade. But if you’re trying to minimize injury, remember that proper technique
> is important and a sharp knife is a safe knife.

Consider each of our `FooVector` classes again:

    struct SimpleVector {
        std::array<int, 10> data_;
        int size_ = 0;
        SimpleVector(SimpleVector&&);
    };

    struct BadVector {
        std::array<int, 10> data_;
        const int *end_ = data_.data();
        BadVector(BadVector&&);
    };

    struct GoodVector {
        std::array<int, 10> data_;
        boost::interprocess::offset_ptr<const int> end_ = data_.data();
        GoodVector(GoodVector&&);
    };

As humans, we can see that `SimpleVector` and `GoodVector` can be trivially relocated, and `BadVector` cannot.
So we expect that the Right Thing will happen if we mark `SimpleVector` and `GoodVector` with `[[trivially_relocatable]]`,
and don't mark `BadVector`. (We are correct!) This follows the basic etiquette for knife (and gun, and tool) use:
Place the blade on the things you intend to cut. Don't place it on the things you don't intend to cut.
(For "blade," read "warrant"; for "cut," read "advertise as trivially relocatable.")

If the "dull knife" version, `[[maybe_trivially_relocatable]]`, is available to us, then we have
two ways of going wrong:

- Mark `SimpleVector` and `GoodVector` with `[[maybe_trivially_relocatable]]`. `SimpleVector` does the Right Thing.
    `GoodVector` remains non-trivially relocatable, because its `end_` member (in isolation) is not trivially relocatable.
    P2786R0 proposes to give us a hard error on `GoodVector` — cf. `struct Error` in §7.6.1.

- Mark `BadVector`, also, as `[[maybe_trivially_relocatable]]`. After all, the compiler clearly has strongly held
    opinions about what makes a type non-trivially relocatable — let's trust the compiler's judgment! This
    attitude is good in general, but is awfully dangerous when it comes to `[[maybe_trivially_relocatable]]`
    in particular. The compiler doesn't detect anything wrong with `BadVector`'s marking, and we get UB at runtime —
    exactly as we would have if we had applied the "sharp knife" of `[[trivially_relocatable]]` to `BadVector`.
    But the sharp knife commands respect: we know not to apply it unless we _intend_ to cut.

Here's a table of all the examples from this blog post, in one place. (Scroll up to find the definitions
of each, or see [this Godbolt](https://godbolt.org/z/sa991Mn6x).)

<table class="smaller">
<tr><td></td> <td>Don't mark</td> <td>Mark (P1144)</td> <td>Mark (P2786R0)</td></tr>
<tr><td><code>BadVector</code></td> <td>OK</td><td>UB</td><td>UB</td></tr>
<tr><td><code>BoostedFunc</code></td> <td>OK</td><td>✓</td><td>Error</td></tr>
<tr><td><code>GoodVector</code></td> <td>OK</td><td>✓</td><td>Error</td></tr>
<tr><td><code>SimpleVector</code></td> <td>OK</td><td>✓</td><td>✓</td></tr>
<tr><td><code>UniqueFunc</code></td> <td>✓</td><td>✓</td><td>✓</td></tr>
<tr><td><code>UniquePtr</code></td> <td>OK</td><td>✓</td><td>✓</td></tr>
</table>
