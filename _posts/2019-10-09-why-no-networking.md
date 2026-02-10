---
layout: post
title: "Why doesn't C++ have networking support?"
date: 2019-10-09 00:01:00 +0000
tags:
  concurrency
  library-design
  mailing-list-quotes
  networking
  wg21-folkloristics
---

Recently, in a discussion of the Networking TS on one of WG21's internal "reflector" mailing lists,
Bjarne Stroustrup wrote:

> One of the most frequent questions that I get from C++ programmers and
> others alike is "why doesn't C++ have networking support?"
> It is not a question that is easy to answer effectively.

Billy O'Neal wrote a very long and detailed answer to the stated question.
(This was not a reply targeted at Bjarne specifically; this was a detailed answer
suitable for people who ask questions like "why doesn't C++ have networking support?")
I think Billy's answer is great and deserves wider viewership, so I asked Billy and Bjarne
for permission to share it here, and they said "Feel free."
So: Billy, why doesn't C++ have networking support in the standard library?

> (1) We don't know how to fix mistakes we put into `std::`. The existing containers/algorithms library,
> when it was 'fixed in the stone of the standard', was standardizing behavior we as an industry knew
> how to do for decades before that shipped. Sure, different languages and systems have different
> interfaces to get at those things, but linked lists, dynamic arrays, and quicksort were
> comparatively solved problems. We do not have that equivalent for concurrency and parallelism, and
> networking is an inherently concurrent environment.
>
> (1a) This is extreme hearsay, take with mountains of salt: I have been told that even the
> 'BSD Sockets' bits exposed by the TS are problematic in a lot of environments. For example, I have
> been told that you can't write something like an HTTP client that is well behaved on iOS on top of
> such an interface, because as soon as the user switches to another app, any sockets you have open
> are closed. But if you use the platform HTTP library, the platform knows what HTTP is and can do
> your download or similar in the background while your app is suspended.
> (Apple folks, please correct me if I misunderstood this one.)
>
> (2) We have implementation experience attempting to standardize and ABI-stabilize complicated
> machinery for which we as an industry don't have a complete, accepted solution, in the form of
> `std::regex`. The result is that I can't reasonably recommend anyone use `std::regex`, because the 3
> major standard library implementations of that are atrocious (as in 2300% to 83000% slower than a
> quality implementation for some inputs). Examples:
>
> * [Boost.Regex Table 13 (MSVC 14.1)](https://www.boost.org/doc/libs/1_71_0/libs/regex/doc/html/boost_regex/background/performance/section_id3752650613.html)
>
> * [Boost.Regex Table 9 (GCC 6.3.0)](https://www.boost.org/doc/libs/1_71_0/libs/regex/doc/html/boost_regex/background/performance/section_id1675827111.html)
>
> (3) Different concurrency and parallelism models don't usually compose very well. For example, a
> program that wants to use ASIO and libuv needs to burn a separate thread for each and pay at least
> one context switch whenever crossing "universes." This means the traditional response "don't like
> `std::vector`? write your own" is effectively unavailable here, and anyone who has a large body of
> code that doesn't map into whatever ends up in `std::` needs to rewrite all of their code if the `std::`
> model gains wide adoption. This makes the (2) problem *much* worse, since even making up another
> `std2` name might not let us fix such mistakes.
>
> (4) We don't have any entity that can behave as the 'benevolent dictator' and encode any particular
> model / solution to these problems and force everyone else to comply. C++ effectively has several
> de-facto networking models, and choosing one means we are making some of our customers unhappy.
>
> (5) The advantage of putting things into `std::` is that they are available everywhere; a corollary to
> that is anything we put into `std::` must be able to go anywhere. A lot of environments that have good
> networking support also reduce the complexity quite a bit by refusing to cater to some use cases.
> For example, Node.JS is effectively a single threaded environment that doesn't care about being a
> 'guest in someone else's process' or otherwise being unloadable, but things we put in `std::` might be
> used by a print driver or shell extension that must be. It seems unlikely that such simplifying
> assumptions would cater to the full set of customers we have.
>
> (6) C++'s lack of memory safety makes it an expensive language to use for anything that crosses trust
> boundaries, and networking has that all over the place. If it's impossible to use the standard
> networking interface to implement anything reasonable to put in production (which in the modern era
> means at least something like TLS) this would further cement the 'C++ doesn't care about security'
> stigma.
>
> My being weakly against standardizing networking doesn't come from thinking networking is an
> unimportant problem. It comes from being unable to reconcile the "ABI stabilized forever, can't fix
> mistakes ever" world we've created for ourselves with this relatively rapidly changing problem
> domain.
>
> (Also please note that the above 'weakly against' is my personal position, not that of Microsoft.)

Disclaimer: I don't fully understand point (6). As best I understand it (with some further help from
Billy off-list), it's basically the point that if programmers come to believe that "C/C++ networking"
is equivalent to "[Heartbleed](https://en.wikipedia.org/wiki/Heartbleed) vulnerabilities,"
then they might reasonably avoid C++ for networking — use a "safer" language for all their
networking needs — at which point it doesn't matter whether C++ ever provides standard networking,
because nobody would trust such a standard networking library in production.

Other than perhaps point (6), I think I agree wholeheartedly with everything Billy said.
