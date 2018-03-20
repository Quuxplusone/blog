---
layout: post
title: Customization point design for library functions
date: 2018-03-19 00:03:00 +0000
tags:
  customization-points
  library-design
  memes
  templates
---

This is a repost and huge expansion of something I sent to John McFarlane a while back
for use in his [CNL](https://github.com/johnmcfarlane/cnl) numerics library. Let's talk
about *customization points*.

(FYI, another good blog post on this subject is [this one by Andrzej Krzemieński](https://akrzemi1.wordpress.com/2016/01/16/a-customizable-framework/).
I highly recommend it. It's probably clearer than this one, overall. I think he comes
to the same general conclusions.)

In library design, a *customization point* is a place where you-the-library-programmer
are deliberately delegating the behavior of some operation to your user-programmer.
Or at least you're giving the user the *option* of providing the operation's behavior.
Maybe they're overriding a behavior that you would otherwise provide by default.
In the STL, `swap` and `std::hash` are examples of customization points.

Every well-designed customization point has two pieces:

* A, the piece the user is required to specialize; and

* B, the piece the user is required to _invoke_ (and therefore must _not_ specialize).

A very good example is found in `std::pmr::memory_resource`:

    class memory_resource {
    public:
        void *allocate(size_t bytes, size_t align = alignof(max_align_t)) {
            return do_allocate(bytes, align);
        }
    private:
        virtual void *do_allocate(size_t bytes, size_t align) = 0;
    };

    class users_resource : public std::pmr::memory_resource {
        void *do_allocate(size_t bytes, size_t align) override {
            return ::operator new(bytes, std::align_val_t(align));
        }
    };

The user puts their new behavior in a member function named `users_resource::do_allocate` (this is
Part A). The library invokes that behavior by calling `memory_resource::allocate` (this is part B).
Notice that parts A and B have different names!

`std::swap`, on the other hand, is *not* a very well-designed customization point.
(To be fair, it was designed 20-something years ago.) Here's how the user would
customize `swap`:

    namespace std {
        template<class T>
        void swap(T& a, T& b) {
            T temp(std::move(a));
            a = std::move(b);
            b = std::move(temp);
        }
    }

    namespace users {
        class Widget { ... };

        void swap(Widget& a, Widget& b) {
            a.swap(b);
        }
    }

The user implements their custom behavior in a function named `swap`
in *their own* namespace (that is, in an associated namespace of `users::Widget`):
this is part A. Then the library invokes that behavior via a little dance
known as [ADL](http://en.cppreference.com/w/cpp/language/adl):

    using std::swap;  // pull `std::swap` into scope
    swap(ta, tb);

If `ta` or `tb` is a `users::Widget`, then ADL will prefer to find `users::swap`;
otherwise, it will find the extremely generic function template `std::swap`.
(This is part B. Notice that part B is awfully cumbersome; and notice that part A
is easy for novice programmers to get wrong.)


Customization point objects
---------------------------

Essentially, John's question that kicked this all off was, "When should I bother
with CPOs (customization point objects) and when should I not worry about it?"
My answer was,

> You need to have a CPO if-and-only-if you're designing some behavior that
> you want the user to be able to use as a *noun* in their code (as opposed to
> using it as a *verb* directly).

![std::transform(first, last, std::toupper);](/blog/images/2018-03-19-what-does-toupper-look-like.jpg){: .float-right}

Abseil has an excellent rule that "thou shalt not 
do anything with a library function except call it like a function" —

> **You may not depend on the signatures of Abseil APIs.**
> You cannot take the address of APIs in Abseil (that would prevent us from adding
> overloads without breaking you). You cannot use metaprogramming tricks to depend
> on those signatures either. [(Source)](https://abseil.io/about/compatibility)

If all you want is for people to be able to call your customization point's part B
like a plain old function — let's say, `y = cnl::quarter(x);` where `cnl::quarter` is
the name of the customization point's part B — well, then, a plain old function
(or function template) is fine. But if you want people to be able to use your part B
as a _noun_ — taking its address, passing it as an argument to a higher-level algorithm,
and so on — then you should make it a customization point object.

For new code with no unusual concerns about compile time or portability backward to C++11,
I think I would currently recommend that *basically all* your library APIs should be
provided as CPOs. Here's the C++14 CPO design I sent to John.

First, we declare the utility type `priority_tag`. Everyone should have this utility type
in their codebase somewhere. Its job is to help when we need to rank some overloads from
"highest priority" to "lowest priority" for overload resolution.

    template<size_t I> struct priority_tag : priority_tag<I-1> {};
    template<> struct priority_tag<0> {};

We're going to implement a customization point for "halving." We promise the user that whenever
we need to "take half of" some user-provided value `t`, we'll do it by ADL-calling `do_halve(t)`
if it exists — and otherwise we'll fall back on the sensible default of `(t / 2)`.
Step 1 is to wrap up those semantics in an overload set named `CNL_detail::detail_halve`.

    namespace CNL_detail {
        template<class T>
        auto detail_halve(T t, priority_tag<0>) -> decltype(t / 2)
        {
            // lower priority (priority 0)
            return t / 2;
        }

        template<class T>
        auto detail_halve(T t, priority_tag<1>) -> decltype(do_halve(t))
        {
            // higher priority (priority 1)
            return do_halve(t);
        }
    }

Then, we write a little generic-lambda function whose job is simply to call `detail_halve`.
This will be an *API entry point* for the user, in case they want to halve something — that is,
this is our part B.

    namespace CNL {
        inline constexpr auto cpo_halve = [](auto&& t)
            -> decltype(CNL_detail::detail_halve(std::forward<decltype(t)>(t), priority_tag<1>{}))
        {
            return CNL_detail::detail_halve(std::forward<decltype(t)>(t), priority_tag<1>{});
        };
    }

Notice that in each of these cases, we are doing SFINAE in the return type, so that
`CNL::cpo_halve(t)` will be well-formed *if and only if* `t` is actually halveable.
(The standard library gets this wrong with `std::swap`... but that's not surprising, as
`std::swap` actually predates SFINAE!)

Now that we have our entry point, our part B, we can write library code that invokes it:

    namespace CNL {
        inline constexpr auto cpo_quarter = [](auto&& t)
            -> decltype(CNL::cpo_halve(CNL::cpo_halve(std::forward<decltype(t)>(t))))
        {
            return CNL::cpo_halve(CNL::cpo_halve(std::forward<decltype(t)>(t)));
        };
    }

And because it's a CPO, not merely an overload set or function template, we can do things with it
that Jules Winnfield would disapprove of. [For example:](https://wandbox.org/permlink/ZA1HA6OW2cifnVsX)

    std::vector<int> vec = {1, 2, 3, 4, 5};
    std::transform(vec.begin(), vec.end(), vec.begin(), CNL::cpo_halve);
    assert((vec == std::vector<int>{0, 1, 1, 2, 2}));

As for part A, we do part A the same way as we did for `std::swap`. (`swap` was poorly designed,
but only in *other* ways: its part B was cumbersome and its part B was also named the same thing
as its part A, leading the user into temptation to specialize or overload `std::swap`. With
`CNL::halve`, the user flatly *cannot* overload `CNL::halve` because it is not a function; and
if they try to provide an ADL `halve`, they'll quickly see that our library doesn't care. Our
library is looking specifically for a part A named `do_halve`, not just `halve`.)

So here's an example of part A:

    namespace users {
        struct Number {
            int value;
            explicit Number(int v) : value(v) {}
            bool operator==(Number rhs) const { return value == rhs.value; }
        };
        Number do_halve(const Number& n) {
            return Number(n.value / 2);
        }
    }
    
And part B:

    std::vector<users::Number> vec = { users::Number(1), users::Number(2), users::Number(3) };
    std::transform(vec.begin(), vec.end(), vec.begin(), CNL::cpo_halve);
    assert((vec == std::vector<users::Number>{users::Number(0), users::Number(1), users::Number(1)}));


Add behaviors to a class you don't own
--------------------------------------

Now, frequently, the user-programmer would like to use a class from Library 1 with a generic algorithm
from Library 2. And the libraries might not know about each other. Let's say that we want to use
`class Other::Bignum` with our `CNL` library. So we write

    namespace Other {
        class Bignum {
            int value;
        public:
            Bignum(int v) : v(value) {}
            bool operator==(Bignum rhs) const { return value == rhs.value; }
            void divide_by(int v) { value /= v; }
        }
    }

    Other::Bignum whole(12345);
    Other::Bignum half = CNL::cpo_halve(whole);

and of course it doesn't compile, because neither `whole / 2` nor `do_halve(whole)` is a valid expression.
What should the poor user do? The only thing that CNL would accept is if `do_halve` were visible in some
associated namespace of `Other::Bignum`. So the poor user probably opens up `namespace Other` and writes:

    namespace Other {
        Bignum do_halve(Bignum b) {
            b.divide_by(2);
            return b;
        }
    }

However, we do not want to encourage our users to open up other libraries' namespaces willy-nilly!
So when we design our library API, we should make sure that we provide a designated *import area*
for customization point parts "A", kind of like an "uploads" directory on an FTP site. Let's add
a high-priority overload to `detail_halve`:

    namespace CNL_customization {
        void do_halve();  // Users add customization points here, if necessary
    }
    namespace CNL_detail {
        template<class T>
        auto detail_halve(T t, priority_tag<0>) -> decltype(t / 2) {
            return t / 2;
        }

        template<class T>
        auto detail_halve(T t, priority_tag<1>) -> decltype(do_halve(t)) {
            return do_halve(t);
        }

        // Uh-oh! Read below.
        template<class T>
        auto detail_halve(T t, priority_tag<2>) -> decltype(CNL_customization::do_halve(t)) {
            return CNL_customization::do_halve(t);
        }
    }

We intend that the user can make `Other::Bignum` usable by `CNL` simply by adding their
new behavior to the `CNL_customization` area:

    // This snippet of code unmistakably "glues together" components from `Other` and `CNL`.
    namespace CNL_customization {
        Other::Bignum do_halve(Other::Bignum b) {
            b.divide_by(2);
            return b;
        }
    }

This ensures that they don't mess up anything in `Other` by adding new overloads into it.
(Furthermore, it ensures they don't unintentionally mess up anything in any *other* generic
libraries — say, `DNL` or `ENL` — which might get confused if they find a function named `do_halve`
in namespace `Other`!)

However, there is a big problem with this: [it does not work](https://wandbox.org/permlink/AB9uQxO2MymNDDtt),
because it falls foul of [two-phase lookup](http://blog.llvm.org/2009/12/dreaded-two-phase-name-lookup.html).

When our template code used `CNL_customization::halve(t)`, since it was
a qualified name, it was not considered "dependent" on the types of `x` and `y` and therefore was looked
up immediately — and not found (because the relevant function definition appears near the bottom of the file).
If the compiler had considered `CNL_customization::halve` to be a "dependent" name, then it would have been looked up in the second phase,
at instantiation time, and it would have been found. Unqualified (ADL) names are considered "dependent";
but we were trying _not_ to use an ADL name, because ADL names are looked up only
in associated namespaces (such as `namespace users`), and we need this name `halve` to get looked up
in `namespace CNL_customization`.


Attempted fix #1: ADL tagging
-----------------------------

We could make it work by adding a "tag" to the function call and using ADL, so that its lookup will
be delayed until the second phase. (Andrzej's blog post calls this idea "`adl_tag`".)
That idea looks like this:

    namespace CNL_customization {
        struct tag {};
    }

    namespace CNL_detail {
        // Overloads 0 and 1 look the same as always

        template<class T>
        auto detail_halve(T t, priority_tag<2>) -> decltype(do_halve(CNL_customization::tag{}, t)) {
            return do_halve(CNL_customization::tag{}, t);
        }
    }

    // This snippet of code unmistakably "glues together" components from `Other` and `CNL`.
    namespace CNL_customization {
        Other::Bignum do_halve(CNL_customization::tag, Other::Bignum b) {
            b.divide_by(2);
            return b;
        }
    }

This successfully circumvents two-phase lookup, and we get the behavior we want!... *except*.

Except that now we've put this really high-priority alternative into play. So when we go back
and try our code with plain old `int`s, [we find it's been broken!](https://wandbox.org/permlink/piuzhZA6UkI4E3ut)

    std::vector<int> vec = {1, 2, 3};
    std::transform(vec.begin(), vec.end(), vec.begin(), CNL::cpo_halve);

The compiler complains:

    algorithm:1963:21: error: assigning to 'int' from incompatible type
            'decltype(CNL_detail::detail_halve(std::forward<decltype(t)>(t), priority_tag<2>{}))'
            (aka 'Other::Bignum')
        *__result = __op(*__first);
                    ^~~~~~~~~~~~~~

See, our CPO's highest-priority alternative is to call `do_halve(CNL_customization::tag{}, 1)`;
and when the compiler looks in all the associated namespaces, it finds that there is exactly
one overload of `do_halve` in the associated `CNL_customization` namespace. It takes parameters
of types `CNL_customization::tag` and `Other::Bignum`; we're passing arguments of types
`CNL_customization::tag` and `int`. It sees that `int` is implicitly convertible to `Other::Bignum`
(because of `Bignum`'s non-explicit constructor), and ta-da! — it decides that we _clearly_ desired
to call `do_halve(tag{}, Other::Bignum(1))`.

So *this* approach is no good, either — we can't encourage our users to add functions to
`namespace CNL_customization` if that's going to break the simple `int` use-case!


Attempted fix #2: Template specialization
-----------------------------------------

Fine, let's go back to the bad old idea of giving our user a template to specialize. Except
that we'll make it a class template (like `std::hash`), not a function template.

    namespace CNL_customization {
        template<class T> struct do_halve;
    }
    namespace CNL_detail {
        // Overloads 0 and 1 look the same as always

        template<class T>
        auto detail_halve(T t, priority_tag<2>) -> decltype(CNL_customization::do_halve<T>::_(t))
        {
            // highest priority (priority 2)
            return CNL_customization::do_halve<T>::_(t);
        }
    }

    // This snippet of code unmistakably "glues together" components from `Other` and `CNL`.
    namespace CNL_customization {
        template<>
        struct do_halve<Other::Bignum> {
            static Other::Bignum _(Other::Bignum b) {
                b.divide_by(2);
                return b;
            }
        };
    }

That's an awful lot of boilerplate in `namespace CNL_customization` at this point, but
[it gets the job done, as far as I can tell](https://wandbox.org/permlink/i0lmM6692Lh50vUk).



A modern std::swap
------------------

So what would `swap` look like, in this design idiom? I think it might look something like [this](https://wandbox.org/permlink/fP2ehbEMI6G2tFLE)...

    #define FWD(x) std::forward<decltype(x)>(x)

    namespace std2::customization {
        template<class T, class U> struct swapper;
    }
    namespace std2::detail {
        template<class T>
        auto detail_swap(T& a, T& b, priority_tag<0>)
            -> decltype(void(a = std::move(b)), void(T(std::move(a))))
        {
            T temp(a);
            a = std::move(b);
            b = std::move(temp);
        }

        template<class T, class U>
        auto detail_swap(T&& a, U&& b, priority_tag<1>)
            -> decltype(void(swap(FWD(a), FWD(b))))
        {
            swap(FWD(a), FWD(b));
        }

        template<class T, class U>
        auto detail_swap(T&& a, U&& b, priority_tag<2>)
            -> decltype(void(FWD(a).swap(FWD(b))))
        {
            FWD(a).swap(FWD(b));
        }

        template<class T, class U>
        auto detail_swap(T&& a, U&& b, priority_tag<3>)
            -> decltype(void(std2::customization::swapper<T&&,U&&>::_(FWD(a), FWD(b))))
        {
            std2::customization::swapper<T&&,U&&>::_(FWD(a), FWD(b));
        }
    }

    namespace std2 {
        inline constexpr auto swap = [](auto&& a, auto&& b)
            -> decltype(std2::detail::detail_swap(FWD(a), FWD(b), priority_tag<3>{}))
        {
            return std2::detail::detail_swap(FWD(a), FWD(b), priority_tag<3>{});
        };
    }

And then to swap two objects `a` and `b`, you'd write this:

    std2::swap(a, b);


Unfinal thoughts
----------------

I started out with something relatively simple. But then trying to implement this "namespace for
importing behaviors" idea, I ran into several different arcane quirks of C++. Each quirk added
more boilerplate to the worst-case path for users of my library. And there may yet
be some critical problem even with the solution I came up with, requiring even *more* boilerplate
to work around!

So, customization points in C++ are hard. They're much less hard if you simply omit the ability
to "glue together" unrelated libraries... but I'm not sure that that's a good tradeoff. Most of
what we do, as programmers, is to glue unrelated things together. Our libraries should make
that *easy*, not hard.

* The complete code for `CNL::halve` is available [here](/blog/code/2018-03-19-cnl-halve.cc).

* The complete code for `std2::swap` is available [here](/blog/code/2018-03-19-std2-swap.cc).
