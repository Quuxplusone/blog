---
layout: post
title: On `raise` versus `throw`, and the Mandela Effect
date: 2018-03-14 00:01:00 +0000
---

I should really buy a copy of [_The Design and Evolution of C++_](http://amzn.to/2FSIYkl). It apparently
contains a rationale for C++'s choice of `throw` and `catch` (as opposed to, say, Python's `raise` and
`except`). The rationale in _D&E_ is presumably similar to 
[this one from Stroustrup in 1989](http://www.stroustrup.com/except89.pdf):

> We will use the phrase "throwing an exception" to denote the operation of causing an exception to occur.
> The reason we don't use the more common phrase "raising an exception" is that `raise()` is a C standard
> library function and therefore not available for our purpose. The word `signal` is similarly unavailable.
> Similarly, we chose `catch` in preference to `handle` because `handle` is a commonly used C identifier.

Upon reading this paragraph, I immediately noticed the parallel to the currently active Coroutines TS.
The Coroutines TS has to use keywords like `co_await` and `co_yield` because for example `yield()` is a
common system call that signals the OS to switch to a different thread for a little while.

...*Or is it?* I went looking for a link to the man page for `yield (2)`, and found that the function
in `<sched.h>` is actually named [`sched_yield (2)`](http://man7.org/linux/man-pages/man2/sched_yield.2.html).
And then in `<pthread.h>` there's [`pthread_yield (3)`](http://man7.org/linux/man-pages/man3/pthread_yield.3.html).
There is no `yield (2)` listed in [The Open Group's documentation of `<unistd.h>`](http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/unistd.h.html)
at all!

Is this like [the Berenstein Bears and that Sinbad genie movie](https://www.snopes.com/news/2016/07/24/the-mandela-effect/)?
Have I been imagining the existence of `yield (2)` all these years?

(I note that `yield (2)` *does* exist on [Solaris](https://docs.oracle.com/cd/E36784_01/html/E36872/yield-2.html).
It is also commonly [named that way in university OS courses](http://web.cs.ucla.edu/classes/fall09/cs111/scribe/7/index.html),
such as the one I took circa 2005, so that's *probably* where I picked up the impression that `yield` was the
Unix/Linux name for the operation. Solaris's use of `yield` as a reserved name
[has caused issues in real life](https://bugs.webkit.org/show_bug.cgi?id=75657),
indicating that there are plenty of people out there who *don't* consider the use of `yield` as a user-space
identifier to be problematic... until it bites them, that is!)
