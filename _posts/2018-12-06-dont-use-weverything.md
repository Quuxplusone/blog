---
layout: post
title: "Don't put `-Weverything` in your build flags"
date: 2018-12-06 00:01:00 +0000
tags:
  c++-style
  compiler-diagnostics
  llvm
  makefiles
  sd-8-adjacent
---

What warning flags should you use to compile your project under GCC and/or Clang?
Well, everyone should be compiling with `-W -Wall` (and probably also `-Wextra`).
But some misguided devs are out there compiling with
[Clang's `-Weverything` flag](https://clang.llvm.org/docs/UsersManual.html#diagnostics-enable-everything).
*Stop that!*

Clang developer Chandler Carruth [writes](https://softwareengineering.stackexchange.com/questions/122608/clang-warning-flags-for-objective-c-development/124574#124574)
(December 2011):

> `-Weverything`: This is an insane group that literally enables every warning in Clang.
> Don't use this on your code. It is intended strictly for Clang developers or for
> exploring *what warnings exist*.

Clang developer James Y. Knight [writes](http://lists.llvm.org/pipermail/cfe-commits/Week-of-Mon-20181203/253703.html)
(December 2018):

> Nobody should be using `-Weverything` in their default build flags! It
> should only be used as a way to find the names of interesting warning
> flags. I'd note that `-Weverything` even has warnings that _conflict_ with
> each other...

Mark Dalrymple [describes a correct workflow](https://www.bignerdranch.com/blog/a-bit-on-warnings/) (August 2012):

> `-Wall`, and friends like `-Wextra` and `-pedantic`, include a well-defined set of warnings,
> so you can assured that your code won’t generate new warnings with new versions of your compiler.
>
> With Xcode 4.4, the Apple LLVM compiler introduced `-Weverything`, which really means,
> "warn me about everything" ... In the real world, I probably won't turn on `-Weverything`
> for day-to-day work on a large project with lots of programmers. ... But I’ve started going
> on occasional warning fix-its — turn it on, build, see what's warned, and then fix it
> if appropriate. Then turn it off before committing the changes.

And yet, the misinformation that "`-Weverything` is a good idea for nightly builds"
stubbornly refuses to die. Here's a partial list of misguided devs promoting the notion:

- Two commenters in [this Reddit thread](https://www.reddit.com/r/programming/comments/1m263m/turn_on_your_damn_warnings/) (September 2013)

- A blogger in ["Xcode Warnings: Can You Turn Them Up to Eleven?"](https://qualitycoding.org/xcode-warnings/) (December 2014)

- Several commenters in [this HackerNews thread](https://news.ycombinator.com/item?id=15400396) (October 2017)

- A developer on [mapbox/cpp issue #37](https://github.com/mapbox/cpp/issues/37) (October 2017)

----

To be absolutely clear: *Don't use `-Weverything` in production!* Use it temporarily in special
one-off builds solely to discover whether a particular warning exists.

For example, if you want to see a warning every time you use `long long` in your code,
start by writing a little test program:

    void foo(long long) {}

[Then compile it with `-Weverything`](https://godbolt.org/z/ej6zTh):

    test.cpp:1:10: warning: 'long long' is incompatible with C++98 [-Wc++98-compat-pedantic]
    void foo(long long) {}
             ^
    test.cpp:1:23: warning: C++98 requires newline at end of file [-Wc++98-compat-pedantic]
    void foo(long long) {}
                          ^
    test.cpp:1:6: warning: no previous prototype for function 'foo' [-Wmissing-prototypes]
    void foo(long long) {}
         ^
    3 warnings generated.

Then try to pull out the warning option you care about. (In this case you can't: we can see at a glance that
there's no way to get *just* the `long long` warning without also getting the "no newline at end of file"
warning.)

Clang provides `-Weverything` *solely* to enable this workflow. We see from this example that
it would be a bad idea to try to compile our whole project with `-Weverything`. If we did, then
not only would we have to pass `-Wno-c++98-compat` and `-Wno-missing-prototypes` right off the bat,
but every time we upgraded our compiler, we'd have to update our build flags to include a whole slew
of additional `-Wno-...` options.

[GCC does not consider the "warning option discovery" workflow to be important, and therefore does not
provide `-Weverything`.](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=66293)
Or, at least, GCC has weighed the *benefit* of `-Weverything` for discovery
against the *risk* of misguided devs applying `-Weverything` in their build options,
and in their opinion that risk is not worth the benefit.
([See further discussion here.](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=53313))

----

Bottom line: Trust Clang's compiler engineers. Trust them in two ways:

- Put `-W -Wall` in your build flags. The Clang devs will take care of making sure this is a
    sensible warning level for all codebases, including yours.

- *Listen to the Clang devs* when they say, "Don't put `-Weverything` in your build flags."
