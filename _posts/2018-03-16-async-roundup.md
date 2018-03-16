---
layout: post
title: 'Async/await, and coloring schemes in general'
date: 2018-03-16 00:01:00 +0000
tags:
  blog-roundup
  coroutines
  exception-handling
  language-design
  paradigm-shift
---

This is from a while back, but now I have a blog to put it on! I found these essays on async/await
to be particularly helpful and relevant and understandable to me personally. They're
all internally linked, too — so, I'll post them in the order that I read them
(chronologically backwards), but you might alternatively read them in the order they
were posted (bottom to top).

* ["Timeouts and cancellation for humans", Nathaniel J. Smith (2018-01-11)](https://vorpus.org/blog/timeouts-and-cancellation-for-humans/)
* ["Some thoughts on asynchronous API design in a post-async/await world", Nathaniel J. Smith (2016-11-05)](https://vorpus.org/blog/some-thoughts-on-asynchronous-api-design-in-a-post-asyncawait-world/)
* ["The Function Colour Myth, or: async/await is not what you think it is", Cory Benfield (2016-07-29)](https://lukasa.co.uk/2016/07/The_Function_Colour_Myth/)
* ["What Color is Your Function?", Bob Nystrom (2015-02-01)](http://journal.stuffwithstuff.com/2015/02/01/what-color-is-your-function/)

The concept of "function coloring" applies essentially everywhere we have a type system.
So: functions in the Coroutines TS can be sync-colored or `async`-colored, but in C++ at large
they can already be, for example, `int`-returning-colored or `void`-returning-colored. They can already
be `noexcept`-colored or `noexcept(false)`-colored. (This causes no end of difficulty when trying to use
`std::vector`, the one template in the language that *cares* whether its type parameter is `noexcept`-colored
or not. Also, notice that the current efforts to introduce a *lightweight error handling* mechanism into
C++, whether it's the chaos of `<filesystem>` or Boost.Outcome or D0709, effectively introduce a *third* color
in this particular dimension.) Compile-time-ness (`constexpr`) is a color.

Allocator-awareness might be considered a color. Heck, *constness* might reasonably be considered a color!

    // grr, function colors are so annoying! Wish we didn't have them!
    V& at(const K& key) { return some_complicated_expression; }
    const V& at(const K& key) const { return const_cast<Self&>(*this).at(key); }

And in many places we *lack* a coloring scheme but really wish we had one. Memory-allocation comes to mind;
so does locking. C++ is so abysmally desperate for a "thread-safety-colored" coloring scheme that we actually
took and *reused* the "const-colored" scheme *as* our "thread-safety-colored" scheme, because it was just too
painful to keep programming threads without *some* kind of coloring scheme!

I caricature Nystrom's thesis as, "Function colors are bad because they make you think about colors all the time."
I caricature Benfield's thesis as, "*Yes, but* they allow you to expose asynchrony to the programmer, so they
are sometimes a good tradeoff, and you don't need to use them if you don't like them."
I caricature Smith's thesis as, "No! They are the *primary way* in which we signal that an operation is blocking;
they are an absolute good; you should not even attempt to avoid them!"

That is, one more place C++ lacks a coloring scheme is along the dimension of "blocking." How many times have
you called a function (let's say `localtime`) *thinking* that it was going to be quick, but in fact observed
that it was taking a global mutex lock — or even going all the way to disk sometimes (let's say, to see whether
your system timezones have been updated in the last few milliseconds)? Wouldn't it be cool if C++ had a way
to mark each function call as either *blocking-colored* or *nonblocking-colored*?

We can live in this world! All we need to do is enforce the rule that *nonblocking-colored* functions are not
allowed to call *blocking-colored* functions. My interpretation of Smith's thesis is that the `async` keyword
gives us (almost?) exactly this coloring scheme.

## Coloring schemes don't stack

There is a problem with coloring schemes, though: it feels like they don't *compose* very well. You
can't stack too many color-related syntaxes on top of one another before you're writing things like

    async std::optional<int> bar() throw;

    int foo() noexcept {
        auto result = try (co_await bar()).value();
        return result;
    }

(And thank you for asking, but it does *not* help if you allow me to write `?*(... bar())` instead,
or whatever crazy punctuation your proposal co-opts.)


## Prior work on ad-hoc coloring schemes

For an attempt to generalize the `const` (and `volatile`) "coloring scheme" to cover other dimensions,
such as thread-safety and taint analysis, see the [CQUAL](http://www.cs.umd.edu/~jfoster/cqual/) project.


## Completely off-topic

[This blog post by Ted Unangst](http://web.archive.org/web/20161007032855/https://www.tedunangst.com/flak/post/accidentally-nonblocking) was an
interesting read on "things you expect to block not blocking, and vice versa."

Above I used the phrase "Yes, but..." (not to be confused with "[Yes, and...](https://en.wikipedia.org/wiki/Yes,_and...)"),
which is also the title of a [French movie](https://en.wikipedia.org/wiki/Yes,_But...)
about a variety of psychotherapy called "brief therapy." Wikipedia's article [on that](https://en.wikipedia.org/wiki/Brief_psychotherapy)
quotes a proponent as saying,

> It's easier to cure a phobia in ten minutes than in five years.

I'm not saying the blog posts linked above *cured* me — the Coroutines TS syntax is still ugly as sin —
but I *do* like the idea of coloring my functions as "blocking" or "non-blocking." At least in Python.
At least in theory.
