---
layout: post
title: "Recursive `make` and `-j`"
date: 2026-09-30 00:01:00 +0000
tags:
  concurrency
  makefiles
  sre
---

Today I looked into "recursive" invocations of `make`; that is, cases in which a Makefile
recipe itself invokes `make`.
[The official documentation](https://www.gnu.org/software/make/manual/make.html#Recursion)
is quite good, but since I'm working with a range of different GNU Make versions, I looked
at how the behavior has changed over time, from GNU Make 3.81 to 4.3.

Basically, the issue is that if you have a recipe in your Makefile like this:

    ser:
        make -C subdir

and then you run `make -j4 ser`, recent versions of GNU Make will give a warning
(`jobserver unavailable: using -j1`). And you'll lose the parallelism you wanted,
because the "jobserver" (basically, the `-j` option) isn't forwarded to the child `make`.
And, incidentally, it won't necessarily use the same `make` binary that _you_ invoked;
it'll run whatever `make` happens to be in your path.

You can fix all three of those issues by prefixing `+` to the recipe line,
or (since that feels uncomfortably magical) replacing `make` with `$(MAKE)`.
More on this at the end of the post. Now that we're forwarding the "jobserver"
properly, I wondered, how _exactly_ do the various `-j` options interact,
when you specify one degree of parallelism on the command line and another
in the recursive recipe?

Here's the pair of Makefiles I used for my test:

    detab -E -4 - >Makefile <<EOF
    ser:
        make -C subdir
    par2:
        make -j2 -C subdir
    par4:
        make -j4 -C subdir
    par:
        make -j -C subdir
    serplus:
        +make -C subdir
    par2plus:
        +make -j2 -C subdir
    par4plus:
        +make -j4 -C subdir
    parplus:
        +make -j -C subdir
    sermake:
        $(MAKE) -C subdir
    par2make:
        $(MAKE) -j2 -C subdir
    par4make:
        $(MAKE) -j4 -C subdir
    parmake:
        $(MAKE) -j -C subdir
    EOF
    mkdir subdir
    detab -E -4 - >subdir/Makefile <<EOF
    all: one two three four five
    one:
        echo 'one' && sleep 1 && echo 'ONE'
    two:
        echo 'two' && sleep 1 && echo 'TWO'
    three:
        echo 'thr' && sleep 1 && echo 'THR'
    four:
        echo 'fou' && sleep 1 && echo 'FOU'
    five:
        echo 'fiv' && sleep 1 && echo 'FIV'
    EOF

Then I tested running `make ser`, `make -j2 ser`, etc., on
various versions of Make available to me.

Here's GNU Make 3.81 (on OS X). The number in each cell indicates
the number of recipes running in parallel. The four rows marked with a dagger †
differ among the versions of Make I tested; those idioms are
"non-portable" and IMHO should be avoided in practice.

| `make`...  |   | `-j2` | `-j4` | `-j` |
|------------|---|-------|-------|------|
| `ser`<sup>†</sup>     | 1 |   2   |   4   |   ∞  |
| `serplus`  | 1 |   2   |   4   |   ∞  |
| `sermake`  | 1 |   2   |   4   |   ∞  |
| `par2`     | 2 |   2<sup>b</sup>  |   2<sup>b</sup>  |   2  |
| `par2plus` | 2 |   2<sup>b</sup>  |   2<sup>b</sup>  |   2  |
| `par2make` | 2 |   2<sup>b</sup>  |   2<sup>b</sup>  |   2  |
| `par4`     | 4 |   4<sup>b</sup>  |   4<sup>b</sup>  |   4  |
| `par4plus` | 4 |   4<sup>b</sup>  |   4<sup>b</sup>  |   4  |
| `par4make` | 4 |   4<sup>b</sup>  |   4<sup>b</sup>  |   4  |
| `par`<sup>†</sup>     | ∞ |   2   |   4   |   ∞  |
| `parplus`<sup>†</sup> | ∞ |   2   |   4   |   ∞  |
| `parmake`<sup>†</sup> | ∞ |   2   |   4   |   ∞  |

In the table above, "b" indicates this warning:

    $ make -j2 par2
    make[1]: warning: -jN forced in submake: disabling jobserver mode.

GNU Make 4.0 (on Debian 8.11):

| `make`...  |   | `-j2` | `-j4` | `-j` |
|------------|---|-------|-------|------|
| `ser`<sup>†</sup>     | 1 |   1<sup>a</sup>  |   1<sup>a</sup>  |   ∞  |
| `serplus`  | 1 |   2   |   4   |   ∞  |
| `sermake`  | 1 |   2   |   4   |   ∞  |
| `par2`     | 2 |   2<sup>b</sup>  |   2<sup>b</sup>  |   2  |
| `par2plus` | 2 |   2<sup>b</sup>  |   2<sup>b</sup>  |   2  |
| `par2make` | 2 |   2<sup>b</sup>  |   2<sup>b</sup>  |   2  |
| `par4`     | 4 |   4<sup>b</sup>  |   4<sup>b</sup>  |   4  |
| `par4plus` | 4 |   4<sup>b</sup>  |   4<sup>b</sup>  |   4  |
| `par4make` | 4 |   4<sup>b</sup>  |   4<sup>b</sup>  |   4  |
| `par`<sup>†</sup>     | ∞ |   1<sup>a</sup>  |   1<sup>a</sup>  |   ∞  |
| `parplus`<sup>†</sup> | ∞ |   2   |   4   |   ∞  |
| `parmake`<sup>†</sup> | ∞ |   2   |   4   |   ∞  |

In the table above, "a" and "b" indicate these two warnings, respectively:

    $ make -j2 ser
    make[1]: warning: jobserver unavailable: using -j1.  Add '+' to parent make rule.

    $ make -j2 par2
    make[1]: warning: -jN forced in submake: disabling jobserver mode.

GNU Make 4.2.1 (on Rocky 8.10) and GNU Make 4.3 (on Red Hat 9 or Ubuntu 22.04)
tweak the latter error message to include the value of `N`:

    $ make -j2 par
    make[1]: warning: -j0 forced in submake: resetting jobserver mode.
    $ make -j2 par2
    make[1]: warning: -j2 forced in submake: resetting jobserver mode.

and also change the behavior of rows `par`, `parplus`, and `parmake`:

| `make`...  |   | `-j2` | `-j4` | `-j` |
|------------|---|-------|-------|------|
| `ser`<sup>†</sup>     | 1 |   1<sup>a</sup>  |   1<sup>a</sup>  |   ∞  |
| `serplus`  | 1 |   2   |   4   |   ∞  |
| `sermake`  | 1 |   2   |   4   |   ∞  |
| `par2`     | 2 |   2<sup>b</sup>  |   2<sup>b</sup>  |   2  |
| `par2plus` | 2 |   2<sup>b</sup>  |   2<sup>b</sup>  |   2  |
| `par2make` | 2 |   2<sup>b</sup>  |   2<sup>b</sup>  |   2  |
| `par4`     | 4 |   4<sup>b</sup>  |   4<sup>b</sup>  |   4  |
| `par4plus` | 4 |   4<sup>b</sup>  |   4<sup>b</sup>  |   4  |
| `par4make` | 4 |   4<sup>b</sup>  |   4<sup>b</sup>  |   4  |
| `par`<sup>†</sup>     | ∞ |   ∞<sup>b</sup>  |   ∞<sup>b</sup>  |   ∞  |
| `parplus`<sup>†</sup> | ∞ |   ∞<sup>b</sup>  |   ∞<sup>b</sup>  |   ∞  |
| `parmake`<sup>†</sup> | ∞ |   ∞<sup>b</sup>  |   ∞<sup>b</sup>  |   ∞  |

Warning "a," which explicitly tells you to "Add `'+'` to parent make rule," will indeed
always vanish if you prefix the `make`-containing line with `+`, like this:

    serplus:
        +make -C subdir

I find that solution as uncomfortably magical as [prefix `@`](https://www.gnu.org/software/make/manual/make.html#Echoing)
or [prefix `-`](https://www.gnu.org/software/make/manual/make.html#Errors).
(Personally, I'd rather see “`|| true`” at the end of a recipe line than a magic “`-`” at the beginning.)
A solution that's less magic-looking (although equally magic under the hood, apparently)
is to use `$(MAKE)`, like this:

    sermake:
        $(MAKE) -C subdir

Neither method silences warning "b," about "resetting jobserver mode," though.

What we really want is a row that would look like `fantasy` below.
But at the moment, it seems we have to choose our preferred tradeoff from
one of the quoted rows, instead.

| `make`...  |        | `-j2` | `-j4` | `-j` |
|------------|--------|-------|-------|------|
| `fantasy`  | nprocs |   2   |   4   |   ∞  |
| `sermake`  | 1      |   2   |   4   |   ∞  |
| `par4make` | 4      |   4b  |   4b  |   4  |
