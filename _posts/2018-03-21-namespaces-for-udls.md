---
layout: post
title: Namespaces for UDLs
date: 2018-03-21 00:01:00 +0000
tags:
  jacksonville-2018
  proposal
  wg21
---

Vittorio Romeo has posted [a very detailed trip report](https://vittorioromeo.info/index/blog/mar18_iso_meeting_report.html)
from the just-concluded WG21 meeting in Jacksonville, Florida. I'm just going to
comment on one very tiny piece of it:

> [...] we discussed [P0921R0 "Standard Library Compatibility Promises"](http://open-std.org/JTC1/SC22/WG21/docs/papers/2018/p0921r0.pdf)
> which, similarly to the previous paper, proposed a set of guidelines that
> the evolution working groups should follow when debating on potential breaking changes
> to the Standard Library. The general ideas were agreed upon by everyone, but the second
> half of the paper was quite controversial. Some of the guidelines for the users, such as
>
>> Do not add `using namespace std` (or any `using` directive for sub-namespaces of `std`)
>> to your code.
>
> were overly restrictive. The above, as an example, would prevent UDLs from being usable.

Preliminary reassurance: Notice that if you want to use deeply nested `std` types, it
would still be totally fine — and in fact, recommended! — to use the little-known
[*namespace alias*](http://en.cppreference.com/w/cpp/language/namespace_alias) syntax for that purpose:

    namespace fs = std::filesystem;

    fs::path p("/hello/world.txt");

Okay, so what about UDLs ([user-defined literals](http://en.cppreference.com/w/cpp/language/user_literal))?
The problem with UDLs is that they are indicated by a single identifier-ish suffix on the
end of an integer or string literal:

    auto hello = "hello"s;  // defines a variable of type std::string
    auto timeout = 1s;  // defines a variable of type std::chrono::seconds

And yes, I deliberately picked the craziest example in the standard library to demonstrate with.

The traditional way to pull these suffixes into scope is with a `using`-directive:

    using namespace std::literals::string_literals;
    auto hello = "hello"s;  // defines a variable of type std::string
    auto timeout = 1s;  // does not compile

But if these are global (`inline`) variables, or otherwise at global scope in a header file,
then there is simply *no* way to get at them without a `using`-directive. We're reduced to
complicated workarounds such as

    namespace detail {
        using namespace std::literals;
        inline constexpr auto initializer_for_timeout() { return 1s; }
    };
    inline auto timeout = initializer_for_timeout();

Or, of course, we could write

    inline std::seconds timeout(1);

but we're pretending that we have a plausible use-case for UDLs in the first place. :)


Namespace all the things
------------------------

The obvious thing to do here would be to change the core language to permit namespace-qualified
UDL suffixes. There are two reasonable syntax options for this:

    inline auto timeout = std::1s;  // A
    inline auto timeout = 1std::s;  // B

Personally, I'm partial to syntax `A`, even though it looks pretty funny at first.
I initially thought that `B` would be knocked out immediately, because it would conflict
with existing syntax such as

    auto negative_one = "hello"s::npos;

But it turns out that that's a syntax error in today's C++! The only acceptable way to get at
the static members of an object's type is via `.`, not via `::`. So:

    auto negative_one = "hello"s.npos;  // OK

Of course C++ is [nothing if not inconsistent](https://wandbox.org/permlink/fpOLGdKTzmamkLFX):

    auto one = 1s.count();  // Error: invalid suffix 's.count' on integer literal

(This happens because any pp-token starting with a digit extends all the way to the next
non-period-plus-or-minus punctuation character: `0x1`, `1.e10`, and `1s.count` are all
equally treated as pp-numbers by the compiler.)

So anyway. Should someone write a WG21 proposal for namespaced UDL suffixes, so that we
can remove that rationale for hanging onto `using namespace ...` directives in C++ code?
