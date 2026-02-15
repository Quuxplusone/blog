---
layout: post
title: "The style guide must say how to do what you mustn't do"
date: 2023-08-11 00:01:00 +0000
tags:
  c++-style
  compiler-diagnostics
  language-design
  library-design
  nodiscard
excerpt: |
  Consider the following snippets of C++ code. Which ones do you think a good compiler would warn about?

      [[nodiscard]] int f();

      f();
      (void)f();
      (int)f();
      int dummy = f();
      [[maybe_unused]] int dummy = f();
      int _ = f();

  I recently heard someone complaining that the last two lines didn't warn. "Aren't those
  declarations basically saying that we plan to sometimes let that value go unused? But the
  author of `f` said not to ignore its return value!"

  The thing is: We must distinguish between the roles of a _compiler warning_, which says "Hey,
  your syntax probably doesn't match your intent!"; and of a _style guide_, which says "Hey,
  your intent doesn't match your boss's intent!"
---

Consider the following snippets of C++ code. Which ones do you think a good compiler would
warn about? (The final line is C++26: see
[P2169 "A nice placeholder with no name"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2169r4.pdf)
(Jabot & Park, 2023).)

    [[nodiscard]] int f();

    f();
    (void)f();
    (int)f();
    int dummy = f();
    [[maybe_unused]] int dummy = f();
    int _ = f();

I recently heard someone complaining that the last two lines didn't warn. "Aren't those
declarations basically saying that we plan to sometimes let that value go unused? But the
author of `f` said not to ignore its return value!"

The thing is: We must distinguish between the roles of a _compiler warning_, which says "Hey,
your syntax probably doesn't match your intent!"; and of a _style guide_, which says "Hey,
your intent doesn't match your boss's intent!"

## The compiler's job

The compiler's job here is to prevent the programmer from accidentally discarding
`f`'s return value. The programmer might make such an accident by:

* Forgetting that `f` returns something that needs checking.
    For example, most codebases can get away with discarding the return value of `puts`,
    but maybe shouldn't discard the return value of `fwrite`, because maybe if `fwrite`
    fails you should handle the error, or retry, or something. "Needs checking" is
    _highly_ context-dependent and may differ from codebase to codebase.

* Misunderstanding what `f` does. The classic example here is `v.empty()`: a beginner
    might plausibly write that on a line by itself thinking that it would "empty `v`,"
    but in fact that's spelled `v.clear()`. Another example is `std::move(x)`: I
    don't know what the beginner might think that does on a line by itself, but whatever
    they think, they're certainly wrong, because it does nothing. That merits a compiler
    warning: their intent doesn't match what they wrote.

* Combination of the two: Inside a member of `class MyFS` I typed `write(1, buf, 8)`,
    thinking that I was calling a member function `write` that never fails;
    but actually no such member exists, and I was getting the global `write`, which can
    fail. Another example might be to type `new T()` when you meant `new (&buf) T()`.

When we mark `f` as `[[nodiscard]]`, we're essentially telling the compiler that its
return value is significant, and we don't want it to be _accidentally_ discarded.
We're helping the compiler help us catch real bugs:

    puts("hello world");  // OK to discard this return value
    f();                  // but not OK to discard this one

We leave open the possibility that a programmer might intentionally _ignore_
`f`'s return value:

    (void)f();  // OK: not accidentally discarded, but intentionally ignored

The syntax `(void)f()` isn't just a hack that happens to work. (C++ is full of those, but this
isn't one.) No, "cast to void" is a specific pattern that is intentionally
built into the front-ends of all major C and C++ compilers. It is the _recognized idiom_ for "I
intend to ignore this expression's result, even though you might think that's unwise."

> Another such pattern, built into all major compilers' front-ends, is "double the parentheses
> to indicate unconventional operator usage": `if (x = y)` is diagnosed but `if ((x = y))` is not.
> This meaning of doubled parentheses isn't natural; it's conventional. I've previously called
> these conventions [suppression mechanisms](/blog/2020/09/02/wparentheses/).

If the programmer types something unusual that is _not_ that specific suppression mechanism,
such as:

    (int)f();
    f(), void();

then it's plausible that the programmer _intended_ to use the result; the compiler might
reasonably produce a warning if it can prove that the result is going unused. Whether the compiler
produces such a warning is a [QoI](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#qoi) issue.
On the other hand, if the programmer types something that seems _intended_ to mark the result as
used — such as putting the result into a variable...

    globalVariable = f(); // probably shouldn't warn

...you probably agree that the compiler shouldn't warn. But what if the global variable
were [`std::ignore`](https://en.cppreference.com/w/cpp/utility/tuple/ignore) — would
that change your mind? Should the compiler warn in that case? Still, no.
The job of compiler warnings is to prevent _mistakes_, not to prevent _intentional effects;_
and assigning to `std::ignore` is about as intentional as you can get.

> Assigning to `std::ignore` is unmistakably intentional; but you still shouldn't do it. See the next section.

## The style guide's job(s)

A C++ style guide has two jobs: one that we can delegate to "outside experts," and one
that we can't. The non-outsourceable job of a C++ style guide is to spell out _behavioral rules_:
"Here are the things that we don't do (or sometimes, prefer to do) in our codebase." By "do," I mean
"intend."

The outsourceable job is to spell out _syntactic rules_: "When you intend behavior X,
then you should use syntax Y." For example, when you intend one certain kind of behavior
in C++, you have a choice to write it as either:

    struct Animal {
        void (*speak)();
        Animal(void (*s)()) : speak(s) {}
    };
    struct Cat {
        void (*speak)();
        Cat() : speak(&Cat::speak_impl) {}
        Animal *as_animal() { return (Animal*)this; }
        static void speak_impl() { std::cout << "Meow"; }
    };

or:

    struct Animal {
        virtual void speak() = 0;
    };
    struct Cat : public Animal {
        void speak() override { std::cout << "Meow"; }
    };

Any C++ expert could tell you that _when_ you are trying to do this kind of thing,
you should prefer the second syntax, not the first. They could even pick nits with
the latter, like probably `speak` should be const-qualified, and probably we should
use the keyword `class` (and `public:`) instead of `struct`. We can say these things
because we know essentially _how to express_ a certain behavior or intent fluently
in the syntax of C++.

This is completely orthogonal to the style guide's second job, the non-outsourceable
part: to say whether that behavior or intent is even something we ought to desire.
The style guide might say "This codebase _does not use_ classically polymorphic
inheritance hierarchies."
(I think that would be a particularly unusual rule, but let's go with it for the sake
of argument.) If your codebase's style guide does ban inheritance hierarchies, then
you shouldn't expect to be able to "get around" that rule simply by replacing the second
syntax above with the first syntax. That's not changing the behavior of the code to
conform with the behavioral guideline; it's simply _obfuscating_ the behavior,
which is a double sin.

Returning to our `[[nodiscard]]` example, let's suppose that the behavioral chapter of
our style guide says, "Thou shalt never ignore the result of `important_function()`."
Notice that the rule is phrased not in terms of _accidentally discarding_ the result —
our programmers can't help accidents! — but rather in terms of _intentionally ignoring_ it.
So it would equally be a violation of the style guide to write

    important_function();
    (void)important_function();
    std::ignore = important_function();
    int _ = important_function(); // and then never use _ again

The compiler will only warn us about the first of these (and even then,
only if `important_function` is in fact marked `[[nodiscard]]`). But the style guide
forbids them all!

"How can the style guide possibly forbid all those different syntaxes?" That's the neat
part: it doesn't. The job of the outsourceable part of the style guide, the syntactic rules,
is to define the idiomatic mapping from intent to syntax: to say, "_When_ you want to
intentionally ignore a function's result, _then_ you write `(void)` in front of it."

> "And without a space in between, by the way."

So `std::ignore = important_function()` should also be flagged in code review — probably
not by the compiler, but by a human. "Hey, you wrote this weird syntax where I think either
you made a mistake and this isn't doing what you intended, or else it's just ignoring the
result and you should write `(void)important_function()` instead because that's the preferred
C++ syntax to ignore a result."

And then all the behavioral part of the style guide has to say is, "Thou shalt
never ignore the result of `important_function()`." As long as people follow the syntactic
rules (the mapping from intent to syntax), we can quickly grep the codebase for
forbidden intent (because it correlates with a specific syntactic construct).

## Other examples

The syntactic part of the guide says: "Whenever you're doing a cast that is tantamount
to a `reinterpret_cast`, you should use the keyword `reinterpret_cast`,
not a functional-style or C-style cast." (See ["Hidden `reinterpret_cast`s"](/blog/2020/01/22/expression-list-in-functional-cast/)
(2020-01-22).) The behavioral part of the guide says: "Don't `reinterpret_cast`."
Now, we can't always get rid of one hundred percent of the `reinterpret_cast`s in our code.
But if we dogmatically follow the _first_ rule, then at least we can measure our degree of
compliance with the _second_ rule — just `grep reinterpret_cast`.

The syntactic part of the guide might say: "Mark virtual functions as `=0` in the base class,
`final` in the leaf class, and `override` in between." The behavioral part of the
guide says: “There should never be an ‘in between.’&thinsp;” (To be fair, I still mark leaf classes'
functions as `override`, although I do mark leaf classes themselves as `final`.)

I run out of clear-cut two-part examples pretty quick, but a lot of my style mantras have
this basic idea behind them.
For example, "Pass out-parameters by pointer, and never intentionally take anything by non-const `&`"
is a good way to detect violations of the behavioral rule "Always be const-correct." I'm
perpetually cleaning up signatures like `void f(Widget&)` that turn out to be careless mistakes
for `void f(const Widget&)` — and often should have been simply `void f(Widget)` anyway.

Similarly, "Never take parameters 'by const value' nor return results 'by const value'"
is a good way to detect accidental forgetting-of-the-`&`.

The syntactic rule "Always use `*` syntax for pointer parameters, never `[]`" is a good way to discourage
subtle violations of the behavioral rule "Never take a pointer-to-buffer out-parameter without also taking
a length-of-buffer parameter." (See ["_Contra_ `char *argv[]`"](/blog/2020/03/12/contra-array-parameters/) (2020-03-12).)

## What I originally said

In case this version will be clearer than everything I said above, here's (more or less)
how I phrased my original response to the `[[nodiscard]]`-complaining style-guide user:

> It sounds like your current coding style guideline is misguided. You ban
>
>     (void)important_function();
>
> but you fail to extend that ban to
>
>     [[maybe_unused]] int dummy = important_function();
>
> This leads to people incorrectly writing the latter when they mean the former, purely in order to get around the ban.
> With all the bad side effects that entails:
>
> - the latter extends the lifetime of the return value for the rest of the scope; that is at least unnecessary and at worst incorrect
>
> - the latter is longer to type
>
> - the latter introduces "noise" in the form of `[[maybe_unused]]` — normally we would consider `[[maybe_unused]]` a code smell
>     and try to eliminate it, but you've got programmers adding `[[maybe_unused]]` all over the place for this reason,
>     so each individual usage needs to be checked and re-checked
>
> - the latter requires inventing new names like `dummy`
>
> That's not at all what you want. What you want is for people to actually check the return value of `important_function`.
> So you should put that in your coding guidelines.
> Then, if any programmer in your organization actually does want to discard the result of `important_function`, they should do it like this:
>
>     (void)important_function();  // with a comment explaining why
>
> This is easy to grep for, and doesn't have any of the disadvantages above.
>
> We must distinguish between:
>
> - Here is a semantic, human-level behavior B which our programmers should not engage in;
>
> - Here is a syntactic construct S that corresponds 1:1 to that behavior B.
>
> In our case here, B is "discard the result of a nodiscard function `f`" and S is "`(void)f()`".
> Our coding style guide must not forbid writing S.
> In fact, it must <b>require</b> the programmer to write S, exactly when the programmer <b>intends</b> B.
> That way, we can find all the (undesirable) instances of B, simply by grepping for S.
> If we forbid S itself, then programmers will simply find novel convoluted ways of expressing B, such that we lose our ability to easily find instances of it.
> Instances of B will proliferate in the codebase.
>
> In order to forbid B, we must require that B is always expressed in terms of S.

## And lastly...

For the record, here's how the big four compiler vendors treat our original examples
([Godbolt](https://godbolt.org/z/ss44Wja5r)):

| Code | Clang | GCC | MSVC | EDG |
|------|-------|-----|------|-----|
| `f();` | `-Wunused-result` | `-Wunused-result` | warning C4834 | `-Wunused-result` |
| `(void)f();` | — | — | — | — |
| `(int)f();` | `-Wunused-result` | — | — | `-Wunused-result` |
| `int dummy = f();` | `-Wunused-variable` | `-Wunused-variable` | warning C4189 | — |
| `[[maybe_unused]] int dummy = f();` | — | — | — | — |
| `int _ = f();` | — | `-Wunused-variable` (for now) | warning C4189 (for now) | — |
{:.smaller}
