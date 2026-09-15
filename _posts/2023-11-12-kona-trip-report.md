---
layout: post
title: "How my papers did at Kona"
date: 2023-11-12 00:01:00 +0000
tags:
  attributes
  exception-handling
  explicit-object-parameters
  implicit-move
  initializer-list
  kona-2023
  lambdas
  library-design
  proposal
  relocatability
  stl-classic
  value-semantics
  wg21
---

I'm now back home (sweet home) in New York after last week's WG21 meeting in Kona, Hawaii.
I'd arrived with three papers in the pre-Kona mailing, and a few more in flight
from earlier. Here's a status report on those papers, and some others.


## P1144R9 `is_trivially_relocatable`

No movement.
There continue to be two essentially competing proposal directions:
my own [P1144](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r9.html)
([Godbolt](https://p1144.godbolt.org/z/7bsP9KvEz)) and Bloomberg's
[P2786](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2786r3.pdf) /
[P2959](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2959r0.html) /
[P2967](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2967r0.pdf)
(cf. [P2685](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2685r1.pdf),
[P2839](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2839r0.html)).
P1144 continues its Pyrrhic dominance of the implementation space,
but since 2022 has been losing hard in the paper space.

> For the areas of contention between P1144 and the Bloomberg proposals,
> see my post-Issaquah blog series
> ([I](/blog/2023/02/24/trivial-swap-x-prize/),
> [II](/blog/2023/03/03/relocate-algorithm-design/),
> [III](/blog/2023/03/10/sharp-knife-dull-knife/),
> [IV](/blog/2023/06/03/p1144-pmr-koans/)).

Bloomberg's [P2786](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2786r3.pdf)
was seen in an EWG Friday session, which lacked quorum and was thus merely "informational."

The main point of this discussion was the wacky behavior of
`std::vector<std::pmr::string>` — although we used `tuple<int&>` as
a stand-in for `pmr::string`. Alisdair Meredith points out in
[P2959](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2959r0.html)
that node-based containers behave predictably for non-regular types like
`tuple<int&>` and `pmr::string`, but non-node-based containers like `vector`
and `deque` (and perhaps one day [`hive`](https://github.com/Quuxplusone/SG14#hive-future--c17))
have uniquely unpredictable behavior. [Godbolt:](https://godbolt.org/z/bz6ndxPjb)

    int data[10] = {0,1,2,3,4,5,6,7,8,9};
    std::deque<std::tuple<int&>> d = { {data[0]} };
    for (int i=1; i < 10; ++i) {
        d.emplace(d.begin() + 1, data[i]);
    }

After this loop, the contents of `d` are more or less arbitrary — as are the
contents of `data` itself! Whenever `deque` internally _move-constructs_ one
of its elements of type `tuple<int&>`, that's essentially a pointer copy;
whenever `deque` internally _move-assigns_ an element, that's a write-through
modification of some element of `data`. The output depends not only on the
`deque` vendor's algorithmic choices (construct, assign, swap) but also on
the `deque`'s block size. For the above example,
libstdc++ ends up with `9`,`3`,`2`,`3`,`4`,`5`,`6`,`7`,`8`,`9` in `data`;
libc++ gives `9`,`2`,`2`,`3`,`4`,`5`,`6`,`7`,`8`,`9`;
Microsoft STL gives `2`,`1`,`3`,`4`,`5`,`6`,`7`,`8`,`9`,`0`.

Alisdair's [P2959 "Relocation within containers,"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2959r0.html)
if I understand correctly, basically asks
for `deque` to "act predictable" here and _forcibly_ treat `tuple<int&>`
as if it were a regular type (i.e. copy its object representations around, rather
than ever delegate that job to a user-provided `tuple<int&>::operator=` that might Do The Wrong Thing).
This being C++, we'd hide that "forcibly" part behind a couple layers of abstraction —
ultimately it would be something like `allocator<tuple<int&>>::relocate` making the
decision — but for ordinary programmers that would all be hidden away.

Less ambitiously,
we could at least stop forcing `deque` to call `tuple<int&>::operator=`, and loosen
the wording to permit `deque` (if it likes) to assume that its element type is regular.
The library wording boils down to rewriting <a href="https://eel.is/c++draft/vector#modifiers-5">[vector.modifiers]</a>
and <a href="https://eel.is/c++draft/deque#modifiers-6">[deque.modifiers]</a> to change
their mentions of "assignment" to something more handwavey, like "alteration of value,"
leaving the exact mechanisms of that alteration up to the vendor.

P1144 has no particular horse in that race. I think it would be great to change
[vector.modifiers] and [deque.modifiers] as suggested in the previous paragraph —
I might even bring such a paper in the December post-Kona mailing — but from P1144's point of view,
`tuple<int&>` is simply _not trivially relocatable_ (due to its user-defined `operator=`).
If you create a `vector<tuple<int&>>` on your hot path then maybe bad performance
is what you deserve. Want something that optimizes well? Use a regular type, such as
`int*` or `reference_wrapper<int>` or `tuple<int*>` (all three of which are trivially
relocatable: [Godbolt](https://p1144.godbolt.org/z/d1W3qbcsP)).

> UPDATE, January 2024: Reader, [I did bring that paper.](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p3055r0.html)<br>
> Previously: ["P1144 PMR koans"](/blog/2023/06/03/p1144-pmr-koans/) (2023-06-03).<br>
> Subsequently: ["Should assignment affect `is_trivially_relocatable`?"](/blog/2024/01/02/bsl-vector-erase/) (2024-01-02).

During the meeting, I submitted to
[Charles Salvia's GitHub repository](https://github.com/charles-salvia/std_error)
implementing [P0709 "Zero-overhead deterministic exceptions"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0709r0.pdf)
("Herbceptions") a patch enabling P1144 library support, so that it now supports using
any (pointer-sized, copyable) trivially relocatable type as its payload type.
([Godbolt.](https://godbolt.org/z/88YK3MGv4))

I'm also pleased to report that the Stellar HPX implementation, Phase 1 of which
Isidoros Tsaousis completed during this year's [Google Summer of Code](https://isidorostsa.github.io/gsoc2023/),
continues to make progress.


## P2447R5 `span` over initializer list

Adopted for C++26! This means that starting in C++26, you'll be able to write:

    void oldf(const std::vector<int>&);
    void newf(std::span<const int>);

    int main() {
        oldf({1,2,3,4}); // still works
        newf({1,2,3,4}); // newly works!
    }

Thanks to [P2752 "Static storage for braced initializers"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2752r3.html)
(adopted as a DR in Varna, further improved by [CWG2753](https://cplusplus.github.io/CWG/issues/2753.html) this week),
the `newf` line will avoid not only the heap-allocation of `vector`,
but even any stack-allocation for the `initializer_list`'s backing array.
This is a great boon for both the reputation of the modern language (as more users are
now able to switch frictionlessly from `const vector&` to `span`) and for performance
(as the `span`-taking version has become more performant).

> Further improving the drop-in-replaceability of `span` for `const vector&`,
> C++26 also adopted [P2821 "`span.at()`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2821r4.html).


## P2767R2 `flat_map`/`flat_set` omnibus

No movement.


## P2848R0 `is_uniqued`

No movement.


## P2952R0 `auto& operator=(X&&) = default`

(That is, a proposal to support `auto` placeholders in the return types of
defaulted special member functions. The compiler knows what type to put there;
it's just a quirk of the wording that you have to spell it out.)

No movement.


## P2953R0 Forbid defaulting `operator=(X) &&`

(That is, if you claim your `operator=` is callable only on rvalues and
never on lvalues, the compiler should disclaim all notion of what its
"default" implementation might look like, and force you to write out
your idea by hand.)

No movement.


## P3016R0 Inconsistencies in `begin`/`end` for `valarray` and `initializer_list`

The core-language part of this paper addressed a tiny wording defect in the treatment of

    for (int i : {1,2,3,4}) ~~~~

(namely, that the _for-range-initializer_ here is not syntactically an _expression_,
so the wording didn't mean what GCC, Clang, and EDG all thought it meant).
This turned into [CWG2825](https://cplusplus.github.io/CWG/issues/2825.html),
and will likely be favorably resolved soon. The library parts of P3016R0 went unseen,
but at least now P3016R1 will affect _only_ the library. Because of the Committee's
structure, it's vastly easier to move a paper that affects "only library" or "only core"
than to move a paper that affects both simultaneously.


## P3031R0 Conversion function for explicit-object lambda

This was a fun one. New in C++23, the idea
of an explicit-object member function is that instead of taking its `this` argument
_implicitly_ (as an _implicit_ object parameter), it takes it _explicitly_ as an
_explicit_ object parameter. (This is unrelated to the `explicit` keyword.)
So you can now write C++ that "looks like Python" in a bad way:

    struct S {
        int x_ = 42;

        int oldf(int y) const { return x_ + y; }
        int newf(this const S& self, int y) { return self.x_ + y; }
    };

Of course you don't _need_ to write `newf` like that; explicit-object syntax isn't "the new way to write functions,"
any more than `std::ranges` is "the new way to write algorithms." But explicit-object
syntax lets you do a few cool things. For example, you can do the CRTP without templates!
([Godbolt.](https://godbolt.org/z/43YGvK8sT))

    struct DoubleSpeaker {
        template<class Derived>
        void speak_twice(this Derived& self) {
            self.speak();
            self.speak();
        }
    };
    struct Mouse : DoubleSpeaker {
        void speak() { puts("squeak"); }
    };
    struct Lion : DoubleSpeaker {
        void speak() { puts("ROAR"); }
    };
    int main() {
        Mouse m; m.speak_twice();
        Lion n; n.speak_twice();
    }

And you can make recursive lambdas! ([Godbolt.](https://godbolt.org/z/38xYcT7cv))

    auto fib = [](this auto fib, int x) -> int {
        return x < 2 ? x : fib(x-1) + fib(x-2);
    };
    static_assert(fib(10) == 55);

But those recursive lambdas are weird. With a normal captureless lambda, I can do
`+f` to convert it implicitly to a function pointer. What happens if I do `+fib` here?
Clang and MSVC both implemented this conversion operation, but both essentially by
accident, without really thinking about what its semantics should be (and it turns out
that the formal wording didn't clarify matters). So Clang just returned `&fib::operator()`,
whereas MSVC returned something like the equivalent of `+[](int x) { return decltype(fib)()(x); }`.
The former is Clearly Just Wrong. The latter is temptingly useful, but produces very
unintuitive behavior if inheriting from a lambda type is allowed to inherit its
conversion-to-function-pointer operator. You can end up in situations where
`x()` and `(+x)()` do different things.

It was perspicaciously noted that captureful lambdas do not convert to function pointers
precisely because they need the lambda object's data in order to work; and explicit-object
lambdas (certainly `this auto` lambdas, but also `this std::any` lambdas)
also basically ask for a reference to the lambda object's data.
So it seems consistent for C++23 to treat explicit-object lambdas the same in this respect
as C++11 treats captureful lambdas.

The C++23 working draft no longer implies that explicit-object lambdas may be implicitly
converted to function pointers. That doesn't mean you couldn't write a proposal to add that
feature back — in fact I'd be happy to advise on such a proposal! But as this issue showed,
it's a very knotty problem that likely doesn't have a simple solution.

Meanwhile, there is a simple workaround if you ever do actually need to turn `fib` into a
function pointer:

    auto fib = [](int x) {
        return [](this auto fib, int x) -> int {
            return x < 2 ? x : fib(x-1) + fib(x-2);
        }(x);
    };
    auto fptr = +fib; // OK


## Other papers

I was pleased to see Gor Nishanov's [P2927 "Inspecting `exception_ptr`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2927r0.html)
(a.k.a. "`try_cast`") make progress in LEWG. The basic idea here is to
permit "throwing" and "catching" an exception without invoking any of the runtime's
invisible-control-flow machinery. Instead of:

    try {
        throw std::logic_error("hi");
    } catch (const std::runtime_error& ex) {
        printf("RE %s\n", ex.what());
    } catch (const std::exception& ex) {
        printf("EX %s\n", ex.what());
    } catch (...) {
        printf("otherwise\n");
    }

you can instead write:

    std::exception_ptr e = std::make_exception_ptr(std::logic_error("hi"));
    if (auto *ex = std::try_cast<std::runtime_error>(e)) {
        printf("RE %s\n", ex->what());
    } else if (auto *ex = std::try_cast<std::exception>(e)) {
        printf("EX %s\n", ex->what());
    } else if (e != nullptr) {
        printf("otherwise\n");
    }

[`make_exception_ptr`](https://en.cppreference.com/w/cpp/error/make_exception_ptr) has been there since C++11;
the novel part is `try_cast`. That function's signature is

    template<class E>
    const E* try_cast(const std::exception_ptr&);

and it acts more or less like `catch (const E& ex) { return &ex; }`, except that
the exception remains in-flight (still owned by the caller's original `exception_ptr`), so that
the returned pointer doesn't dangle. If a catch handler of type `const E&` wouldn't be hit,
then `try_cast<E>` returns `nullptr`.

> Would `std::exception_cast` be a better name? All `exception_ptr`'s other operations involve
> the word `exception`; e.g. `std::rethrow_exception(e)`. On the other hand, the cast operation
> requires the caller to name an exception type inside the angle brackets, which seems pretty
> unambiguous. Do you prefer `std::try_cast<std::runtime_error>(e)` or `std::exception_cast<std::runtime_error>(e)`?
> People with strong opinions should email me and/or Gor with their comments.

I came to Kona thinking that the right design for `try_cast` would be to basically take the
type in the angle brackets and copy-paste it into the `catch` handler, so that
`try_cast<int&>` would act like `catch (int& ex)` and `try_cast<int()>` would act like `catch (int ex())`
(which decays to `int (*ex)()` just like in any other parameter list). But I was soon
convinced that the simpler, stricter signature above was far superior — and that's exactly what
P2927's next revision will propose.

There's many things you can copy-paste to produce a _syntactically valid_ `catch` handler,
but many of them are subtle or flat-out nonsensical. For example, we've already seen that you
can technically write `catch (int())` to catch a function pointer, or `catch (int[])` to catch an
`int*`. Worse, `catch (int(&)())` is syntactically valid but produces an unreachable handler,
because only objects can be thrown, and a function reference can never bind to an object.
(This is [GCC bug 69372](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=112471).)
Also, it's legal to catch `(int&)` but not `(int&&)`. The simpler, stricter signature
(we additionally mandate that `E` be a complete non-array object type, and not a
pointer to an incomplete type either) avoids all of this nonsense and provides "just the
functionality, ma'am."

"But it returns `const E*` — what if I want to modify or augment the exception object in flight?"
No problem! Just `const_cast` that pointer. It's 100% safe; the underlying exception object
is never const. The ugliness and greppability of `const_cast` are features, not bugs.

----

Peter Sommerlad's [P2968 "Make `std::ignore` a first-class object"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2968r0.html)
didn't move much in Kona, but I think it's in good shape and on track to make C++26.
This small paper provides the Standard's blessing to code like

    std::tie(std::ignore) = std::make_tuple(42); // OK
    std::ignore = 42;                            // UB

which is technically UB today: `std::ignore`'s behavior is well-specified by the paper
standard only when it's wrapped inside a `tuple`. Of course it works fine in real life;
the paper standard just hasn't been saying so. Peter fixes that, and also addresses some
minor implementation divergence in libc++ along the way.

    template<template<class> class C, class T>
      constexpr bool f(C<T>) { return false; }
    template<class T>
      constexpr bool f(T) { return true; }
    static_assert(f(std::ignore));
        // libc++ fails

    struct S { int i : 2; } s;
    void f() { std::ignore = s.i; }
        // libc++ fails

----

Brian Bi's [P2748 "Disallow binding a returned glvalue to a temporary"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2748r2.html)
_almost_ made C++26 this meeting, but needs some minor tweaks to its wording. It patches a corner case in the ongoing
saga of `return x`. C++23's [simpler implicit move](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2266r3.html)
successfully disallowed dangerously dangling returns like

    int& f(int i) { return i; } // OK in '20, error in '23+'26

while naturally continuing to permit

    int& f(int& i) { return i; } // always OK
    int&& f(int&& i) { return i; } // error in '20, OK in '23+'26

After Brian's paper, C++26 will additionally disallow

    long&& f(int&& i) { return i; } // OK in '20+'23, error in '26

where the trick is that the returned `long&&` isn't binding to `i` (the `int` object from `f`'s caller's scope) —
it's materializing temporary `long` from the value of `i`, and binding to that temporary!
Directly returning a reference bound to a temporary in this way is no longer permitted; the warning your compiler
already gives on this line will in C++26 become an error.

Now the bad news: even after Brian's paper, C++26 continues to permit you to return a reference bound to
an automatic variable, even a move-eligible variable. For example, this `return` is still legal C++26,
although of course every vendor warns about it:

    const int& f(int i) { return i; } // always OK (oops)

Type-system-based local reasoning will never stamp out _all_ dangling. The following will always be legal C++:

    int& g(int& r) { return r; } // safe in isolation
    int& f(int i) { return g(i); } // dangles in combination

Brian also brought a paper [P2991 "Stop forcing `std::move` to pessimize"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2991r0.html)
which got a relatively (not landslidingly) negative reception. Rightly so, in my opinion,
since that paper basically asked us to encourage more use of `return std::move(x)`
just as the language is finally finding solid footing with `return x`!

----

One paper I paid no attention to, but which is now adopted for C++26 and looks important,
is [P2662 "Pack Indexing"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2662r2.pdf)
(Jabot & Halpern, 2023). In C++26 you'll be able to write a variadic [`cadr`](https://en.wikipedia.org/wiki/CAR_and_CDR) as

    auto cadr(auto... ts) { return ts...[1]; }

Another important paper adopted for C++26 is [P1673 "A linear-algebra interface based on the BLAS"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1673r12.html)
(13 authors). It adds a lot of functions, types, and concepts to the Standard,
all isolated within a new `std::linalg` namespace.
