---
layout: post
title: _Contra_ built-in library types
date: 2018-04-15 00:01:00 +0000
tags:
  language-design
  operator-spaceship
  rant
---

C++ has several guiding principles or mantras. The most famous one is probably "zero-cost abstraction,"
but for my money the most important one is "C++ does not have a string type."

Let me explain. In C++, the syntax `"hello"` gives you a value of type `const char*` (well, really
`const char[6]`, but never mind); this is required for C compatibility. Stroustrup *couldn't* make
`"hello"` give you an object of some built-in type `str`, as it does in Python. But clearly C++
needed a real string type! So we got `std::string` — a _class_ type that behaves just like a string
type should. This meant that the core language had to be flexible enough to handle all the things
that we wanted strings to do, _generically_.

* Because strings require memory management, we had to have destructors and RAII.

* Because strings can be concatenated `s + t` and indexed `s[42]`, we had to have operator overloading.

* Because strings can be function parameters `func_taking_string("hello")`, we had to have implicit
  conversions.

(Yes, these _are_ in order from best idea to worst idea. Why do you ask?)

Of course `std::string` isn't the only reason, or even the primary reason, that we got all these
flexibility-producing features in C++. But it's a great showcase for them. And the mantra generalizes:
C++ doesn't have Python's `str`, but you can build `std::string` as a class. C++ doesn't have a
built-in `list` type, but you can build _both_ `std::vector` and `std::list` as classes and then pick
the best one for the job. (And then build `fixed_capacity_vector` when `std::vector` turns out to
still be wrong.) C++ doesn't have C's `_Atomic T` or `_Complex T` types, but you can build
`std::atomic<T>` and `std::complex<T>` as classes. C++ doesn't have a built-in regex
matcher like Javascript, but you can build `std::regex` as a class. (And then build [`"regex"_pre`](https://github.com/hanickadot/compile-time-regular-expressions)
when `std::regex` turns out to still be wrong.) C++ doesn't have built-in reference counting
like Objective-C, but you can build `std::shared_ptr` as a class.

So, being able to build our own [software tools](https://amzn.to/2H2PbYq) is a bedrock principle of C++.
We don't build "magic types" into the core language; we give programmers the tools, and the freedom,
to build their own types from scratch. You can (and usually should!) use `std::string` in your own
programs, but there is nothing special about `std::string` from the compiler's point of view.


## But there are some magic types

When C++ forgets this guiding mantra — "C++ does not have a string type" — it tends to fall off the rails
badly.

    auto x = typeid(42);  // core language syntax

This returns a reference to an object of the built-in type `std::type_info`, which has virtual functions
and stuff.

    struct A { virtual ~A(); }
    struct B : A {};
    auto y = dynamic_cast<B&>((A*)nullptr);

This throws an exception of the built-in type `std::bad_cast`, which has virtual functions and stuff.
And `std::bad_cast` also has a `noexcept` copy constructor, which means that it is not allowed to
allocate heap memory during a copy, which means that its `what()` string must be either static or
reference-counted. (In practice it will be reference-counted.) This is a lot of heavyweight code we're
pulling in, just to use a core language feature!

That reminds me: another famous C++ mantra is "You don't pay for what you don't use." These magic
built-in types are a perfect example of _having to pay for things we're not intending to use_.

So, I suggest that a good guiding design principle for C++ is: _Avoid magic built-in types._


## Nuancing my position

But I have recently added some nuance to my views on "magic types", following a breakfast discussion
at the WG21 meeting in Albuquerque (November 2017).  This discussion was about the then-proposed C++2a
`operator<=>` (which is now solidly in the working draft). I didn't like that it seemed to be adding new 
"magic" library types, which as we've seen is usually a terrible idea.

I asked, what if I wanted to define my own `operator<=>` for one of my own types, and I wanted it to
return a custom result type; wouldn't it be troublesome that my custom type would not play well with
Herb's magic baked-in library type?

We definitely have this problem today with the `std::exception` hierarchy (e.g. `std::bad_cast` above).
Suppose I want all my thrown exceptions to inherit from `MyCustomRootObject` (say, an object whose
constructor takes a stack trace). I cannot ever achieve that goal, unless I abandon the STL (`out_of_range`)
and abandon `typeid` and `dynamic_cast` (`bad_cast`, `bad_typeid`) and abandon `operator new` (`bad_alloc`).
All of which I should arguably be abandoning _anyway_, but still, this is a horrible state of affairs.
In order not to bathe in dirty water, I _must_ throw out the baby as well.

> "C++: It should always be possible to separate the baby from the bathwater."

So, I said at breakfast, if we let `std::strong_ordering` into the core language, we're basically replicating
the horrible state of affairs we have with `std::exception` — but now instead of abandoning all those crappy
features like `typeid`, you're going to make me abandon _comparison operators_?  This seems _insane_, I said.

But I was successfully convinced otherwise!  After all, the existence of `std::nullptr_t` in the library
does not make me want to abandon null pointers.  The existence of `std::ptrdiff_t` in the library does not
make me abandon subtraction.  Psychologically, we don't even tend to think of these features as "library"
features _baked into_ the core language; instead, we think of them as "core language" features which are
_exposed_ via convenient library typedefs. The big difference between `std::nullptr_t` and `std::exception`
is that `std::nullptr_t` is awesome enough, and cheap enough, that nobody ever _wants_ to replace it.
Therefore it is not a problem that `std::nullptr_t` is inflexibly a part of the core language. `std::nullptr_t`
is _clean bathwater_.

I decided that there was a decent chance that `std::strong_equality` would probably be more like `nullptr_t`
than like `bad_alloc`, and that I was willing to take that risk.

One (logically weak but psychologically compelling) argument in favor of `std::strong_equality` is that
it's possible to write a library typedef that "exposes" the core-language type. Just as we can imagine writing

    using nullptr_t = decltype(nullptr);

(and in fact I _do_ write things like `bool operator==(decltype(nullptr)) const` in my own code, in order
to eliminate a header dependency; and I even find it _more readable_ than the alternative) — we can also
imagine writing

    using strong_ordering = decltype(1 <=> 2);
    using strong_equality = decltype(nullptr <=> nullptr);
    using partial_ordering = decltype(1. <=> 2.);

(Now, horribly, this snippet is [ill-formed unless you have already `#include`d `<compare>`](http://eel.is/c++draft/expr.spaceship#10),
which is absolutely unconscionable, but let that pass for now.)

Whereas it is not possible to write a similar library typedef for, let's say, `std::bad_alloc`.

On the other hand, it _is_ possible to write the typedef

    template<class T> struct S;
    template<class T> struct S<const T&> { using type = T; };
    using type_info = typename S<decltype(typeid(1))>::type;

(again, ignoring the horrible requirement to `#include <typeinfo>` before doing so), and this should not
be taken as compelling evidence that `std::type_info` is awesome!

Trivia: I notice that it is not currently possible to write a typedef for

    using weak_ordering = ???;

because you cannot create a C++ type with that comparison category using only core-language features.
It is also not *currently* possible to write a typedef for `weak_equality`; but there has been
a suggestion of

    union U { int a; int b; };
    using weak_equality = decltype(&U::a <=> &U::b);

because it is [guaranteed](http://eel.is/c++draft/expr.eq#3.5) that `U::a` and `U::b` must be equal,
yet they are not substitutable.

However, none of the "big three" compilers implement C++ pointers-to-union-members correctly,
so I think it is likely that the Committee will end up just "fixing" pointers-to-union-members somehow
so that they become sane, rather than trying to pretend that they are actually a good example of
`weak_equality`.

[A fun test program for your compiler of choice is:](https://wandbox.org/permlink/7TcTDdFS4kmTlVtD)


    #include <iostream>

    union U { int a; int b; };

    int main() {
        constexpr auto pa = &U::a;
        constexpr auto pb = &U::b;

        constexpr bool x = (pa == pb);
                  bool y = (pa == pb);

        std::cout << std::boolalpha << x << ' ' << y << ' ' << (pa == pb) << std::endl;
    }


GCC prints "false false true"; Clang and MSVC print "false true true".
Intel's ICC compiler correctly prints "true true true".
