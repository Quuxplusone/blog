---
layout: post
title: "Thoughts on Herb Sutter's CppCon keynote"
date: 2019-09-24 00:01:00 +0000
tags:
  contracts
  cppcon
  exception-handling
---

Herb Sutter's closing keynote from CppCon 2019,
["De-fragmenting C++,"](https://www.youtube.com/watch?v=ARYP83yNAWk)
is now up on YouTube. Here are some thoughts I had while watching it live:

----

[@8:36](https://www.youtube.com/watch?v=ARYP83yNAWk&t=8m36s): Herb asks
if the audience would like to see WG21 put more effort into "simplifying"
C++, and less effort into "adding things" to C++. Wild applause.
Immediately Herb acknowledges the bait-and-switch: you see, in order to
"simplify" things, clearly we need to add metaclasses, which is "a thin
layer over reflection and generation," which _also_ needs to be added.

We saw the same thing play out with `operator<=>` in C++2a, right?
"Let's write just one member comparison operator, instead of six hidden-friend
comparison operators" sounds like a simplification, until you realize that
we need to _add_ to the language in order to do that — not just add one new operator
to every lexer in the world, but also

- add rewrite rules so that the compiler can interpret `a < b` as `(a <=> b) < 0`

- add a new kind of candidate (_rewritten candidates_) for overload resolution of some operators

- add a new kind of candidate (_synthesized candidates_) for overload resolution of some operators

- add Yet Another meaning for literal `0` in C++ (besides its existing uses as integral zero, the null pointer constant, and the pure-virtual-method indicator)

- add a new taxonomy (`strong_ordering`, `weak_ordering`, etc.)

- require programmers to learn when the new taxonomy matters and when it doesn't (for example, _technically_ `float` is now only partially ordered, but no library algorithm actually cares)

- require programmers to consider whether the convenience of a defaulted member `operator<=>` outweighs the goal of consistency with all other operators (which must still be hidden-friend nonmembers)

Contrast this modern view of "simplification" with some of C++'s success stories:

- removal of trigraphs

- removal of `gets`

- addition of `inline` variables (except for the part where we had to take our teaching that "things defined in a class body are implicitly `inline`" and add a caveat "...except for variables")

Consider also the perennial request to replace `std::invoke`'s complicated metaprogramming, the
library abomination [`std::mem_fn`](https://en.cppreference.com/w/cpp/utility/functional/mem_fn),
and C++'s weird `(obj->*pmf)()` syntax by just making `pmf(obj)` work in the core language.
(Peter Dimov's [N1695](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2004/n1695.html) (2004).
Barry Revzin's [P0312](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0312r1.html) (2017).
JeanHeyd Meneide's [P1214](http://open-std.org/JTC1/SC22/WG21/docs/papers/2018/p1214r0.html) (2018).)
I call that a simplification. But as Herb says, C++ isn't good at simplifications. C++ is good at
additions; and C++ is _very very_ good at additions which are _marketed_ as simplifications.

Claiming "simplification" is a good way to get applause, but I think we (as an audience)
also have a responsibility to evaluate the change being proposed and see whether it is
_in fact_ a simplification, or merely an addition.

----

But that was a sidebar. Herb really wanted to talk about error-handling mechanisms.
He has some really great statistics on the subject. Notably, he's got _multiple_ surveys
that asked coders whether they were allowed to handle errors via exceptions, via numeric error codes,
or via expected/outcome types. [@18:19](https://www.youtube.com/watch?v=ARYP83yNAWk&t=18m19s):

> Perhaps most disturbingly, every one of these is banned outright in more than ten percent of projects,
> according to these surveys. And that is a measure of how fragmented the [C++] world has become.

I haven't seen the raw responses to these surveys. I would be interested to know whether the
ten percent of respondents who can't use numeric error codes is in fact _disjoint_ from the
twenty percent who can't use exceptions and the thirty percent who can't use expected/outcome.
My kneejerk reaction, though, was to wonder if these groups are all the same people. See, this
was a survey about how your code handles _errors_... and in some codebases, AFAIK,
that notion is considered an oxymoron!

An error is ([@24:54](https://www.youtube.com/watch?v=ARYP83yNAWk&t=24m54s)) "an act that fails
to achieve what should be done." This is an interesting definition because it forces a _moral dimension_
onto the situation: the human in charge has decided that something _should_ be done —
Herb gives the example of parsing an arbitrary string into an integer value — and the poor C++ code
must deal with the inevitable clash between what _should_ be done and what physically _can_ be done.
That clash produces what we call _errors_.

One completely valid engineering response (which I particularly associate with Odin Holmes,
after seeing him field a similar question on a CppCon embedded-systems panel one time) is to
eliminate that moral dimension. Don't think about what _should_ be done with that arbitrary string;
think about what situations your code needs deal with in order to accomplish its end goal.
For example, maybe there are two possible situations: "the input string matches one of these
4294967296 possible inputs" or "the input string doesn't match any of them."
(In this formulation we don't need to deal with "overflow" as a separate situation;
the string `10000000000` falls into that second category just as naturally as `xyzzy` does.)
So you write some code to deal with inputs in the first category, and you write some more code to
deal with inputs in the second category. Look, ma, no "error handling" — because we don't have
"errors"! Our code isn't allowed to _err_, and therefore it doesn't need to handle _errors_.

To put it another way, an "error" is what you get when your function cannot perform the task
that it advertised — it failed to do what it said on the tin. You could treat this as an
opportunity to throw an exception (or return `std::unexpected` or whatever), or you could treat
this as a bug in your documentation and/or design. A function should never fail to do what it
advertises. Maybe it's just advertising the wrong thing!

I know that a lot of code out there is written using this philosophy when it comes to
dynamic memory allocation, or even when it comes to stack allocation. It doesn't make sense to ask
an embedded programmer "How are you allowed to deal with `malloc` failure — exceptions, error codes,
or expected/outcome types?" The answer will be "I'm not allowed to do any of that. I have to avoid
getting into that situation in the first place." (Usually by avoiding `malloc` entirely.)
How do you deal with stack exhaustion? "I don't exhaust my stack. I measure the size of each
stack frame, I have a static call graph, and I avoid recursion."

So when I saw Herb's graph, I wondered if perhaps some non-negligible group of respondents
had answered "No" to all three methods of error handling simply because their code was not
allowed to err in the first place.

----

Herb taxonomizes errors in a very sensible way:

- "Abstract machine corruption," such as stack exhaustion.

- "Programming bugs," such as null dereference or precondition violation.

- "Recoverable errors," such as not finding the file you're trying to open.

This basically matches the taxonomy in an article I was reading on the plane home from CppCon:
Eric Lippert's ["Vexing Exceptions"](https://blogs.msdn.microsoft.com/ericlippert/2008/09/10/vexing-exceptions/)
(September 2008). Eric labels the first two categories "fatal" (e.g. stack exhaustion)
and "boneheaded" (e.g. null dereference). He splits the third category into

- "Vexing" exceptions, such as not parsing the string into an integer.

- "Exogenous" exceptions, such as not finding the file you're trying to open.

The distinction is unclear, but is something like "Your program controls the string input
to `Int32.Parse`; so there might be some cases where you needn't handle parse errors because
there logically can't be any. But your program can't possibly control the whole file system,
so you _always_ have to handle file-not-found; that's just physically unavoidable." I don't
think I agree with that distinction, but I might be mischaracterizing it.

Herb's presentation goes beyond just _taxonomizing_, though. He proposes that C++ should
idiomatically use three different sets of tools to handle each of these three different kinds
of errors.

- For abstract machine corruption, just ignore it. That's an implementation problem. There's nothing the application programmer can do about stack exhaustion.

- For programming bugs, use a (Contracts-style) _precondition_, and abort the program on precondition violation.

- For recoverable errors, throw a (Herbceptions-style) _exception_.

So when we look at a piece of code such as

    File openfile(const char *fname) [[pre: fname != nullptr]];

    File openfile(const char *fname) [[pre: fname != nullptr]] {
        FILE *fp = fopen(fname, "r");
        if (fp == nullptr) throw FileNotFoundException();
        return File(fp);
    }

we can immediately observe that if the file is not openable, that's (intended to be)
a recoverable error; whereas if the client programmer passes in a null filename, that's
a bug — a contract violation — that should fail-fast and be fixed by the client programmer.

I think this idea is reasonable. On the other hand...

- Python seems to get along just fine using "fail-slow" exceptions for assertion failures.
    In Python, the `assert` statement raises `AssertionError`.
    The exception simply unwinds all the way back up to `__main__`, where the unhandled
    exception aborts the program. If they can do it, why not C++?

- Having two utterly different core-language mechanisms for two subtly different use-cases
    may increase the difficulty of teaching and using C++. "Oh, you used X, but you should have
    used Y." Before taxonomizing — or rather, before forcing every student of the language
    to taxonomize — we should make sure that the taxonomy is actually adding value.
    (See also: `std::weak_ordering`.)

- It is still mildly weird that we're listing our contract violations into the function interface,
    but listing our recoverable errors only in the function implementation. Do we actually help
    the client programmer by exposing the former and hiding the latter? I'm not sure.

Of course every idea has both benefits and costs.
Do the benefits of this particular taxonomy outweigh the costs? Maybe. I'm not sure.

----

Finally, if you were there, you might have noticed that I wanted to ask a question.
My question was going to be:

> This low-cost exception stuff sounds very interesting. What are the chances that
> we'll see Microsoft Visual Studio implement any of this within, say, the next five years,
> so that the industry can experiment with it and give feedback?
>
> Alternatively, if the plan is to propose it for standardization before implementing it,
> how are you going to make sure it actually delivers on what it promises?

(One of CppCon 2019's hottest talks, which I do plan to
catch on YouTube, was Stephan Lavavej's talk about how MSVC now implements `<charconv>`.
Their implementation of the 2017 standard took until late 2019... and they're one of the quick ones!
C++2a, with its mass of difficult features, is almost certain to land this coming year,
which means the MSVC team — along with every other C++ compiler team — will be very busy
playing catch-up; again leaving little time for experimentation with pre-standard features.
"Standardize first, implement later" is a vicious cycle.)

Furthermore, I think that C++2a should be delayed.
