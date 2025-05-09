---
layout: post
title: 'Why do I get a linker error with `static const` and `value_or`?'
date: 2020-09-19 00:01:00 +0000
tags:
  c++-learner-track
  cppcon
  pitfalls
  sum-types
  war-stories
---

In my Back to Basics talk on "Algebraic Data Types," I cut some slides at the
last minute. (I still ended up going over time a bit.) Once you're able
to download the
slides from CppCon's slides repository, you'll see three bonus slides at the
end. One was on the fact that you can (but shouldn't!) use `std::variant<T,E>` as a poor man's
version of `Expected<T>`. The other two were on a minor pitfall that seems to
bite me every time I update a piece of legacy code to use `std::optional`.

Consider the following C++ class ([Godbolt](https://godbolt.org/z/EEG4o4)):

    struct Connection {
        static const int DefaultTimeoutMs = 100;
        int timeoutMs() const {
            return hasTimeoutMs_ ? timeoutMs_ : DefaultTimeoutMs;
        }
    private:
        bool hasTimeoutMs_;
        int timeoutMs_;
    };

When you upgrade this codebase to C++17, you should update this particular
class to use an `optional`; it'll be just as performant, and significantly
more type-safe (because you'll never be able to use the integer value of `timeoutMs_`
when it isn't meant to be valid). [Godbolt](https://godbolt.org/z/8se47q):

    struct Connection {
        static const int DefaultTimeoutMs = 100;
        int timeoutMs() const {
            return timeoutMs_.has_value() ? *timeoutMs_ : DefaultTimeoutMs;
        }
    private:
        std::optional<int> timeoutMs_;
    };

And then you should change that ternary operator to use the convenience
method `.value_or()`:

    struct Connection {
        static const int DefaultTimeoutMs = 100;
        int timeoutMs() const {
            return timeoutMs_.value_or(DefaultTimeoutMs);
        }
    private:
        std::optional<int> timeoutMs_;
    };

And [suddenly your program fails to link!](https://godbolt.org/z/oqvMr5)

    /bin/ld: test.o: in function `Connection::timeoutMs() const':
    test.cpp:6: undefined reference to `Connection::DefaultTimeoutMs'

The root cause here doesn't really have anything to do with `optional`,
but I can't recall any time I've run into this problem _except_ when
updating a piece of legacy code to use `.value_or()` (and then it's happened
multiple times).


## What's going on here?

The trick here is that when you use the ternary operator `x ? y : DefaultTimeoutMs`,
it needs only the _value_ of `static const Connection::DefaultTimeoutMs`. The
compiler's optimizer can see that the value of `DefaultTimeoutMs` is 100 and
rewrite the expression as `x ? y : 100`. But when you say `x.value_or(DefaultTimeoutMs)`,
you're calling a member function defined as

    template<class U>
    constexpr T value_or(U&& default_value) const&;

which takes its argument _by reference._ If you compile with optimizations, then
again the compiler's optimizer will inline `value_or` and your code will link fine;
but in debug mode, the compiler assumes that you might want to set a breakpoint
inside `value_or`, and therefore it outputs a call to the function. As the argument
to the call, it passes a reference to `DefaultTimeoutMs` — which requires that variable's
address!

In C++, ever since C++98 and still today in C++20, when you have a
static data member of a class, it works the same way as any other global variable.
You must not only _declare_ it (inside the body of the class, which goes in a header
file and ends up duplicated in many places) but also _define_ it
(in some .cpp file that will be compiled only once).

    // in connection.hpp
    struct Connection {
        static const int DefaultTimeoutMs;
    };

    // in connection.cpp
    const int Connection::DefaultTimeoutMs = 100;

Just like any other global variable, if you define it as `constexpr`
or `inline` then you can put the definition in the header file and
don't have to give it any out-of-line definition.

The weird thing about `static const` member variables of integral type
is that you are allowed to move their initializing expression from the
definition to the declaration!
The out-of-line definition is still required, because some .o file needs to
reserve space for the variable itself, but moving the initializer into the
header file allows inlining the value, like we saw at the beginning of
this post.

    // in connection.hpp
    struct Connection {
        static const int DefaultTimeoutMs = 100;
    };

    // in connection.cpp
    const int Connection::DefaultTimeoutMs;

In legacy code, it is _very common_ for people to write `static const` members
like this, and then simply never write the `connection.cpp` part. As long as
you never pass `DefaultTimeoutMs` by reference, you'll never notice the
problem.

Only when you start using `optional::value_or()`, which takes its
parameter by reference, does the linker force you to notice this quirk
of the language!


## How do I fix it?

The appropriate fix, almost certainly, is to replace `static const`
with `static constexpr`.

    struct Connection {
        static constexpr int DefaultTimeoutMs = 100;
        int timeoutMs() const {
            return timeoutMs_.value_or(DefaultTimeoutMs);
        }
    private:
        std::optional<int> timeoutMs_;
    };

Since C++17 (not coincidentally, the same release which introduced `optional`),
`static constexpr` data members are implicitly `inline` and do not need
redundant out-of-line definitions.
[cppreference has a good page](https://en.cppreference.com/w/cpp/language/static)
on the subject.

----

So there you are: A pitfall of C++'s overly complicated `static const` rules,
which in my experience _everyone_ runs into when upgrading old code to use
`optional::value_or`. Find this whole blog post recapitulated in two slides
at the end of my "Back to Basics: Algebraic Data Types" when it arrives in
[the CppCon 2020 GitHub repository](https://github.com/CppCon/CppCon2020)
later this month.
