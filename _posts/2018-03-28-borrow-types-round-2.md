---
layout: post
title: 'Borrow types, Round 2'
date: 2018-03-28 00:01:00 +0000
tags:
  c++-style
  paradigm-shift
---

I wasn't planning to write another post about "borrow types" quite so soon as this,
but my [previous post on the subject](/blog/2018/03/27/string-view-is-a-borrow-type/)
generated some commentary, including this suggestion from Herb Sutter (with
credit also given to Bjarne Stroustrup for prior observations on the subject):

> Key concept, "generalized-parameter-declaration": It has been independently
> rediscovered a number of times that a range-for loop variable is very much like
> a function parameter [...] To this same set we should add lambda capture.
> Finally [...] a catch-clause [declaration] also behaves as a parameter that
> binds to the thrown object [...]
>
> Arthur, I would add lambda-capture and catch-declaration to your list of
> `string_view` recommended uses for the above reasons.

Number one, I'm extremely flattered. Number two, you're wrong. :)

Let's take the catch-handler case first, because I actually
considered that case and decided to tactfully ignore it when I was writing up
that original post. Let's open that can of worms!


## Do not catch `string_view`

Here's the kind of code we're talking about:

    try {
        throw std::string("hello");
    } catch (const std::string& ex) {
        std::cout << ex << std::endl;
    }

If you run this code, you'll see it print "hello", which should not be surprising
at all. The `const std::string& ex` declaration (which C++ officially calls an
_exception declaration_) behaves basically just like a function parameter in all
respects: it syntactically looks like a function parameter, of course, but it also
behaves like a function parameter in certain respects related to return value
optimization (RVO), and also in the sense that when it is a reference, it refers
to an object (the _exception object_) whose lifetime is greater than the scope
of the variable `ex` itself. So in a very real sense, here, `ex` is of "borrow type"
just the same way that it would be if it were a function parameter.

*However.*

Let's try our "C++17 modernization" refactoring on the code above...

    try {
        throw std::string("hello");
    } catch (std::string_view ex) {
        std::cout << ex << std::endl;
    }

This code no longer prints "hello" — instead, it terminates with an uncaught exception!
This is because C++'s rules for catch-block selection do not match its rules for
overload resolution; they are more like its rules for `dynamic_cast`. (I have
a whole CppCon talk on *that* subject! [(YouTube)](https://www.youtube.com/watch?v=QzJL-8WbpuU))

For the same reason, we will have a baaad time if we try to write this:

    std::any a = std::make_any(std::string("hello"));
    std::string s1 = std::any_cast<const std::string&>(a);  // OK
    std::string s2 = std::any_cast<std::string_view>(a);  // Throws bad_any_cast

What we see in both of these examples is that — even though I stand by my assertion
that native reference types such as `const std::string&` *are* borrow types — they
are *also* native reference types, and this gives them special core-language
superpowers that are not granted to arbitrary user-defined types such as
`std::string_view` and `std::reference_wrapper`. In the first example we saw
essentially that C++'s internal mechanism for selecting catch-block handlers 
understands catch-by-reference as a special case; and in the second example we saw
essentially that `typeid(T) == typeid(const T&)`.

So what does this little digression mean for our simple rule of thumb? *Should* we
permit "borrow types" in catch-block exception declarations?

*No.* We should certainly permit and encourage catching by reference —
there are horrible pitfalls for catching by anything-except-`const&` —
but that's about catching by *native reference*. It has absolutely
nothing to do with *borrow types* as a general rule. If you are trying to
catch a `string_view`, you are doing it wrong and your code *will not work.*

**Do not catch `string_view`!**


## Do not capture `string_view`

Okay, now what about lambda captures? This is an interesting case to me because...
well, because I hadn't considered it before! The observation being made
by Bjarne and Herb is that another place we idiomatically use native references
in C++ is in lambda-captures:

    struct StringSet {
        vector<string> elements_;

        template<class F>
        void for_each_element(const F& f) const;
    };

    StringSet ss;
    std::vector<std::string> result;
    ss.for_each_element([&result](auto&& elt) {
        result.emplace_back(elt);
    });

Here we've got so many referencey things going on that I've omitted the body of
`for_each_element` for clarity; check [the previous post](/blog/2018/03/27/string-view-is-a-borrow-type/)
if you care. The *important* referencey thing that Bjarne and Herb are observing
is that our lambda-function is capturing `result` *by reference*. If it had captured
`result` by value, then the code wouldn't do what we want. It is important and
idiomatic that lambdas often capture things by reference.

In situations like this, where we're passing a short-lived lambda as a callback
to a "for-each"-style function (whether it's this obviously named, or whether it's
a subtler "for-each" such as `std::copy_if` or `std::sort`), the most foolproof
thing to do is to capture everything by reference: use a default capture of `[&]`.
The only time it makes sense to capture anything by *value* is if you're expecting
your lambda to be stored somewhere or otherwise outlive its current context (for
example, if you're passing it to the constructor of `std::thread` or
[`std::packaged_task`](http://en.cppreference.com/w/cpp/thread/packaged_task)).

The preceding paragraph should be tickling your brain. Short-lived... no ownership...
function parameter... Yes, that's absolutely correct: we have a *borrow type*
going on here! But the borrow type is not the type of the lambda *capture*; the
borrow type is the type of *the callback lambda itself!*

    auto x = [&](auto&& elt) {
        result.emplace_back(elt);
    };

`decltype(x)`, here, is a borrow type: it refers to things it doesn't own, it makes
sense only if it is short-lived, and (by an utter but happy coincidence) it doesn't
have an assignment operator.

The important takeaway here is that *lambdas which capture `[&]` are borrow types,*
and should follow all the usual borrow-type rules: don't return such lambdas from
functions, or capture them in long-lived variables, or otherwise allow them to
*escape* the local context.

So that's an interesting point (I hope)... but it's not what Herb meant. He was
suggesting that the capture itself — `&result` — was of the borrow type `std::vector<int>&`,
and therefore I should relax my rule of thumb and permit borrow types to appear in lambda
capture-lists.

Is this true? Should we ever write something like this?...

    template<class Strings>
    auto get_words_beginning_with(
        const Strings& lst, std::string_view prefix)
    {
        std::vector<std::string> result;
        auto inserter = std::back_inserter(result);
        std::copy_if(
            lst.begin(), lst.end(), inserter,
            [prefix](auto&& s) {
                return s.starts_with(prefix);
            }
        );
        return result;
    }

Here we are capturing a copy of `prefix` — a `string_view` variable — in the lambda
capture list of our filter. Is this good practice? Should we encourage this?

*No, we should not.*

Our lambda captures `[prefix]`, when it *should* be capturing `[&]`. There is no reason
to do anything "special" or "out of the ordinary" here; we are making a short-lived
callback lambda for use with a "for-each"-style function, and so we should capture
no more and no less than `[&]`. Capturing by value would make sense if we knew that
the lambda was going to outlive the current scope... but if we knew that, then capturing
the borrowed `prefix` by value would actually lead us straight into dangling-pointer-land!

*Capturing a borrow type by value is usually a bug, and never helpful. Don't do it.*

In almost all cases, the correct thing to capture is `[&]`. If you are diverging
from that, you are in expert territory and should tread carefully... and you *still*
shouldn't be capturing borrow types!

**Do not capture `string_view`!**


## Conclusion

My previous post was muddled a little bit by my positing that native reference types
were borrow types (which is true), but then my making a "rule of thumb" that claimed
only two valid uses for "borrow types," which implied that there were only two valid
uses for native references as well (since native references are a proper subset of
the set of borrow types).

My rule of thumb remains the same:

- Borrow types (other than native references) must appear *only* as
  function parameters and for-loop control variables.

With an exception for return types:

- A function may have a borrow type as its return type, but if so, the function must
  be explicitly annotated as returning a potentially dangling reference. That is, the
  programmer must explicitly acknowledge responsibility for the annotated function's
  correctness.

- Regardless, if `f` is a function so annotated, the result of `f` must not be stored
  into any named variable except a function parameter or for-loop control variable.
  For example, `auto x = f()` must still be diagnosed as a violation.

...Now, when I disallow `auto x = f()`, you might think that this is also problematic
for native references. That's true! See, native reference types have a built-in
[`operator auto`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0672r0.pdf)
that happens to return `T` instead of the more "natural" `T&`. Native references also
have an assignment operator; remember that in the previous post I claimed that
borrow types generally don't need assignment operators. But these "warts" aren't
really warts; they're just because C++ native references are not *just* borrow types;
they're also an important part of the core language and have several deeply
magical features.

So here are my belated caveats for native reference types in particular:

- When catching exceptions, always catch by `const T&`. (Never catch `string_view`
  or any other non-magical borrow type!)

- Lambdas should capture exactly `[&]` unless that would be semantically incorrect;
  in which case it is okay to capture any *value type* by value.
  (Never capture `string_view`, or any other non-magical borrow type, by value!)
