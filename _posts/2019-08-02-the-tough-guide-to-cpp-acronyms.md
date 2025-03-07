---
layout: post
title: "A C++ acronym glossary"
date: 2019-08-02 00:01:00 +0000
tags:
  c++-learner-track
  cpplang-slack
  wg21-folkloristics
---

Someone on Slack recently noted C++'s penchant for cryptic acronyms. So I thought I'd write down
a glossary of the ones I'm aware of. I'm sure I forgot some. If you think of one I've missed, please
let me know! (And no, I'm not adding [DesDeMovA](https://www.youtube.com/watch?v=fs4lIN3_IlA).)

Cppreference also has [a page of acronyms](https://en.cppreference.com/w/cpp/language/acronyms).

## AAA

"Almost Always Auto." A phrase, and coding style, introduced by Herb Sutter
in [Guru of the Week #94](https://herbsutter.com/2013/06/13/gotw-94-special-edition-aaa-style-almost-always-auto/)
(June 2013). It means writing

    auto dx = x1 - x2;
    auto p = std::make_unique<int>(42);
    auto i = 0;

instead of

    double dx = x1 - x2;
    std::unique_ptr<int> p = std::make_unique<int>(42);
    int i = 0;

respectively. (See also ["The Knightmare of Initialization in C++"](/blog/2019/02/18/knightmare-of-initialization/) (2019-02-18).)

## ABC

"Abstract base class." That is, a class with at least one pure virtual function, intended for
use as the root of a class hierarchy in classical [OOP](#oop).

## ABI, API

"Application Binary Interface" and "Application Programming Interface," respectively. The API of a library
is the interface you program against, in a more or less high-level language. This could be as general as
saying "My library provides a Python API" (that is, to interface with my library, you'll use Python); or
as specific as saying "`std::map<K, V>::operator[]` accepts a parameter of type `K`." The API of a library
describes its interface in terms relevant to the human programmer.

[Bob Steagall says](https://www.youtube.com/watch?v=S7gGtYqtNNo&t=5m23s) an API is "a precise and complete
specification of [a component's] guaranteed user-visible behavior." Visible to which user? The human programmer.

[Louis Dionne says](https://youtube.com/watch?v=DZ93lP1I7wU&t=2m04s), "I like to think of ABI as being
like API, but for machine code." The ABI of a library describes its interface in terms relevant to the
_machine_. For example, "Symbol
`_ZNSt3mapI1K1VSt4lessIS0_ESaISt4pairIKS0_S1_EEEixERS5_` identifies a function that expects to be passed
the address of a `map` object in `%rdi` and the address of a `K` object in `%rsi`. It returns the address
of a `V` object in `%rax`."

- If you change the fundamental ideas behind your library,
    then your users may have to re-design their whole system architecture.

- If you leave your library's fundamentals alone but change its API,
    then your users won't have to touch their system architecture;
    but they may have to make changes to their programs (i.e., re-translate their programs into C++ source code).

- If you leave your library's API alone but change its ABI,
    then your users won't have to touch their C++ source code;
    but they may have to recompile everything (i.e., re-translate their programs from C++ into machine code).

An example of an ABI break that is not an API break would be if you changed one of the
function signatures in your library from
`Item getItem(int index)` to `Item getItem(const int& index)`.
At the C++ API level, these functions are called using exactly the same syntax.
Yet the ABI of the former is "pass `index` in register `%rdi`"; the ABI of the latter is
"pass the _address_ of `index` in `%rdi`." If you linked together two object files, one compiled
with the old ABI and one with the new ABI, they wouldn't work together — you'd probably get a segfault.

Vice versa, an example of an API break that is not an ABI break would be if you changed
`extern "C" Item getItem(const int& index)` to `extern "C" Item getItem(const int *index)`.
These functions have the same ABI — they expect the address of an `int` in `%rdi` — but at the C++ API
level they're called with different syntax. Anyone using the old API would have to modify
their C++ code — change `getItem(i)` to `getItem(&i)` — in order to compile it with the new API.

When we talk about "[the Itanium C++ ABI](https://itanium-cxx-abi.github.io/cxx-abi/)"
or "the MSVC ABI," we're talking more broadly about the
collection of rules and relationships that go into defining the ABI of any C++ code — for example,
the rules for name-mangling, for parameter-passing, for class layout and vtable layout, and so on.
Two compilers that adhere to the same ABI (in this sense) can take API-compatible C++ source files
and produce object files that are ABI-compatible (in the previous sense).

## ADL

"Argument-dependent lookup." See ["What is ADL?"](/blog/2019/04/26/what-is-adl/) (2019-04-26).

## ADT

"Abstract data type"; that is, any class type with which the user interacts only via a
high-level ("abstract") interface. In C++, thanks to [linguistic interference](http://www.glottopedia.org/index.php/Interference)
from other meanings of the word "abstract," you might see "ADT" used specifically to refer to
STL-style class templates, such as `std::priority_queue`; that is, any class template which
is parameterized ("abstracted") over an open set of parameter types.

All ADTs are also _user-defined types_ (UDTs). Due to confusion over whether library types such as
`std::string` were or were not "user-defined," the paper standard has mostly stopped using the term
"UDT" in favor of "[program-defined type](http://eel.is/c++draft/definitions#defns.prog.def.type)."
`std::string` is not a "program-defined type."

## ARM

[_The Annotated C++ Reference Manual_](https://amzn.to/31LFuYL) (Ellis and Stroustrup, 1990).
This work — vastly outdated as of the mid-’90s, of course — consists of a reference manual for
pre-standard C++, plus annotations and commentary by Stroustrup which "discuss what is not included
in the language, why certain features are defined as they are, and how one might implement
particular features."

Confusingly for C++ programmers, [ARM](https://en.wikipedia.org/wiki/ARM_architecture)
(originally "[Acorn](https://en.wikipedia.org/wiki/Acorn_Computers) RISC Machine")
is also the name of a processor architecture used by many embedded devices, including
most Android smartphones. Your C++ compiler might produce code to run _on_ ARM, but
your C++ compiler almost certainly does not accept the dialect of C++ described _by_ the ARM!

## ASan, MSan, TSan, UBSan

"Address Sanitizer," "Memory Sanitizer," "Thread Sanitizer," and "[Undefined Behavior](#ub) Sanitizer,"
respectively. These are the colloquial names for the runtime error-detection instrumentation packages
supported by GCC and Clang. [MSVC supports ASan as well](https://learn.microsoft.com/en-us/cpp/build/reference/fsanitize),
but as far as I can tell (as of 2025) doesn't offer the other three yet.

ASan is similar to `valgrind`, but whereas `valgrind` observes an otherwise uninstrumented process,
with ASan you actually tell the compiler to generate special instrumented code so that the program
itself is always trying to detect its own problems.

|:---|:---|:---|:---|
| ASan | [Address Sanitizer](https://clang.llvm.org/docs/AddressSanitizer.html) | `-fsanitize=address` | Use-after-free; buffer overflow |
| MSan | [Memory Sanitizer](https://clang.llvm.org/docs/MemorySanitizer.html) | `-fsanitize=memory` | Read from uninitialized |
| TSan | [Thread Sanitizer](https://clang.llvm.org/docs/ThreadSanitizer.html) | `-fsanitize=thread` | Data race |
| UBSan | [UB Sanitizer](https://clang.llvm.org/docs/UndefinedBehaviorSanitizer.html) | `-fsanitize=undefined` | Integer overflow; null pointer dereference |
{:.smaller}

## BMI, CMI

"Binary Module Interface." Just as .cpp files are compiled into .o files, and some compilers provide
ways to "pre-compile" .h files into [PCHes](#pch), compilers that support C++20 Modules will have to provide
some way to compile C++ modules into some format that is precompiled, perhaps binary, perhaps compressed,
to make `import` statements quick to compile.

The term "BMI" is not used by the paper standard. There is no standard BMI format. Each vendor will have
their own format, just like they do today for precompiled headers and object files (although some of those
formats may be governed by other standards, such as [ELF](https://en.wikipedia.org/wiki/Executable_and_Linkable_Format)).
[P0822 "C++ Modules are a Tooling Opportunity"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0822r0.pdf) (Gaby Dos Reis, October 2017)
gives a very high-level sketch of the BMI format that Microsoft calls "IFC," and which is modeled
on something else called ["Internal Program Representation" (IPR)](https://github.com/GabrielDosReis/ipr).

Because BMIs are not necessarily "binary" (in the sense of being highly compressed), [GCC calls them](https://gcc.gnu.org/wiki/cxx-modules)
"Compiled Module Interfaces" (CMI); and as of October 2020, the as-yet-unpublished
["Modules Ecosystem Technical Report"](https://github.com/cplusplus/modules-ecosystem-tr) (or "METeR")
glosses "BMI" as "_Built_ Module Interface."

_BMI files are not a distribution format._ When you distribute a module, you'll be distributing its source
code (as one or more files [maybe with the extension .mpp](https://www.youtube.com/watch?v=E8EbDcLQAoc&t=11m00s)).
You won't distribute the BMI file (extension .ifc) that MSVC produces, any more than you'd distribute the
object file (extension .obj) that MSVC produces — in fact the urge to distribute .ifc files should be
even _rarer_ than the urge to distribute .obj files. Probably the best analogy really is to .pch files —
one of the things C++20 Modules aim to replace.

## CAS

Almost always stands for "[compare and swap](https://en.wikipedia.org/wiki/Compare-and-swap),"
a primitive atomic operation. The C++ standard library consistently calls this operation
[`compare_exchange`](https://en.cppreference.com/w/cpp/atomic/atomic/compare_exchange),
and provides `_strong` (no-spurious-failure) and `_weak` (spurious-failure-possible) versions.

Sometimes you might see "CAS" used to mean
"[copy and swap](https://stackoverflow.com/questions/3279543/what-is-the-copy-and-swap-idiom),"
a C++ idiom that implements copy-assignment in terms of copy-constructing and then swapping.
Compared to "open-coding" your assignment operator, the copy-and-swap idiom provides a simple,
mechanical way to achieve both correctness and the strong exception guarantee. Some people
say it's overhyped or overused. Personally, I recommend it — and I never abbreviate it!

In specialized contexts, you might see "CAS" used to mean
"[content-addressed storage](https://en.wikipedia.org/wiki/Content-addressable_storage),"
also known as "content-addressable memory" (CAM) or "associative storage."

## CNTTP

"Class non-type template parameter" (see [NTTP](#nttp)). Since C++20 (specifically since
[P0732](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0732r2.pdf) and
[P1907](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1907r1.html)), NTTPs are allowed
to be of class type, as long as that type is [structural](https://eel.is/c++draft/temp.param#7).
A structural type is simply a class type all of whose non-static data members are public
and (recursively) structural — basically, any C-style struct. Thus:

    template<int N> void f();  // OK
    template<std::pair<int, int> NN> void g();  // OK since C++20
    template<std::string S> void h();  // Error

When such a template is instantiated, the CNTTP's "value" (that is, the values of its data members,
recursively) is mangled into the instantiation's linker symbol. On the Itanium [ABI](#abi), that
looks like this.

    struct A { bool one; float two; };
    constexpr A x = {true, 1.0};
    constexpr A y = {false, 3.14};
    template<A Value> float foo() { return Value.two; }
    int main() {
      foo<x>(); // call _Z3fooIXtl1ALb1ELf3f800000EEEEfv
      foo<y>(); // call _Z3fooIXtl1ALb0ELf4048f5c3EEEEfv
    }

> Why `3f800000` and `4048f5c3`? See ["Bit patterns of `float`"](/blog/2021/09/05/float-format/) (2021-09-05).

Inside a function taking a CNTTP, the value of the template parameter is accessible
as a [template parameter object](https://eel.is/c++draft/temp.param#8.sentence-1),
which is more or less an `inline constexpr` variable shared by everyone in the program.
([Godbolt.](https://godbolt.org/z/WczM7PW5q))

Note that the type `A` might be non-comparable (as here), or even define its `operator==` such that `x == y`.
Still, `foo<x>` and `foo<y>` will be distinct instantiations because the "structural" value representations of
`x` and `y` differ, regardless of what `operator==` says. Back in 2019, I disliked this so much that I
[wrote a paper](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2019/p1837r0.html) proposing to remove
CNTTPs from C++20. WG21 didn't buy it.

## CPO

"Customization point object." This is a notion introduced by Eric Niebler's Ranges library,
which means it's new in C++20.

> A customization point object is a function object with a literal class type
> that interacts with program-defined types while enforcing semantic requirements on that interaction.
> —N4810 [[customization.point.object]/1](http://eel.is/c++draft/customization.point.object#1)

That is, a CPO is an _object_ (not a function); it's callable; it's constexpr-constructible
(that's what "literal" means in this context); it's customizable (that's what it means to
"interact with program-defined types"); and it's concept-constrained.

(WG21 has a fetish for describing concept constraints as "semantic requirements,"
even though C++20 Concepts are a purely syntactic feature because
[nobody knows how to specify semantic requirements](/blog/2018/09/08/problems-concepts-should-solve/#okay-so-to-recap-here-are-the-bi).
The compiler can ensure that some `T` provides syntax for both `==` and `!=`, but it won't check their semantics.)

In practice, this looks more or less like

    namespace detail {
        template<class A, class B>
        void swap_helper(A& a, B& b) {
            using std::swap;
            swap(a, b);
        }
    }

    inline constexpr auto swap =
        []<A, B>(A& a, B& b)
            requires Swappable<A> && Swappable<B>
        {
            return detail::swap_helper(a, b);
        };

(The C++20 standard has a lot of wording inherited from Eric's Ranges-v3 to deal with
something colloquially known as the "poison pill"; but I observed, and Eric confirmed,
that the poison pill hasn't been necessary ever since C++17 introduced a SFINAE-friendly `std::swap`.)

The benefit of a CPO over a named function is that it separates [the two pieces of the customization
point](/blog/2018/03/19/customization-points-for-functions/):

- A, the piece the user is required to specialize; and

- B, the piece the user is required to _invoke_ (and therefore must _not_ specialize).

In the above example, "A" would be your ADL overload of `swap`, and "B" would be the `swap` CPO itself.

Also, when you call a CPO, even if you don't qualify
its name, you don't get [ADL](#adl) — which means you get more predictable behavior.

So, to recap, a CPO is a "callable, constexpr-constructible, customizable, concept-constrained object."
Maybe it should have been called a "C<sup>6</sup>O" instead!

If you remove the adjectives "customizable, concept-constrained" from the above, then you have a
function object that turns off ADL — but is not necessarily a customization point.
The C++20 Ranges algorithms, such as `std::ranges::find`,
[are like this](http://eel.is/c++draft/algorithms.requirements#2). Any callable, constexpr-constructible
object is colloquially known as a "niebloid," in honor of Eric Niebler.
A CPO is simply a niebloid that wraps a user-definable customization point.

## CRTP

The "Curiously Recurring Template Pattern." See [Wikipedia](https://en.wikipedia.org/wiki/Curiously_recurring_template_pattern).
Occasionally misquoted as "Curiously Recur<b>sive</b>."

## CTAD

"Class template argument deduction." See [cppreference](https://en.cppreference.com/w/cpp/language/class_template_argument_deduction).
Occasionally misquoted as "<b>Constructor</b> template argument deduction,"
even in [the WG21 papers](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0702r1.html)
that originally added the feature.
(It's much easier to remember the acronym "CTAD"
than to remember what the feature does! It deduces the template arguments to the
_class_ template. The arguments to any particular constructor template are deduced,
as always, via [template argument deduction](https://en.cppreference.com/w/cpp/language/template_argument_deduction).)

## CTRE

"Compile-time regular expressions." Specifically,
[Hana Dusíková's `ctre` library](https://github.com/hanickadot/compile-time-regular-expressions),
which allows you to write things like

    #include <ctre.hpp>
    static_assert(ctre::match<"^h.*[wxyz]orl[^y]">("hello world"));

The current version of CTRE relies on C++20's [class-type NTTPs](#cnttp). Before C++20, it relied on
a compiler extension supported by GCC 8.x and Clang 5–11 (but no longer by either compiler's trunk):

    using namespace ctre::literals;
    static_assert("^h.*[wxyz]orl[^y]"_ctre.match("hello world"));

## CWG, EWG, EWGI, LEWG, LEWGI, LWG

These are the main working groups of the [ISO C++ Committee](https://isocpp.org/std/the-committee)
(a.k.a. WG21). At least in theory, their responsibilities are as follows:

- The Evolution Working Group Incubator (EWGI, pronounced "oogie"; a.k.a. SG17) evaluates proposals for core-language features.

- The Evolution Working Group (EWG) designs core-language features.

- The Core Working Group (CWG) reviews core-language wording.

- The Library Evolution Working Group Incubator (LEWGI, pronounced "lewgie"; a.k.a. SG18) evaluates proposals for standard library facilities.

- The Library Evolution Working Group (LEWG) designs standard library facilities.

- The Library Working Group (LWG) reviews standard library wording.

EWGI and LEWGI are very new in the grand scheme of things; they met for the first time
at the San Diego meeting (November 2018).

By the way, "ISO WG21" stands for Working Group 21 of the
[International Organization for Standardization](https://en.wikipedia.org/wiki/International_Organization_for_Standardization);
and "SG17" means "Study Group 17." For a list of study groups, see [isocpp.org](https://isocpp.org/std/the-committee).

When you see "CWG" or "LWG" followed by a number, as in "[CWG1430](https://cplusplus.github.io/CWG/issues/1430.html)"
or "[LWG3237](https://cplusplus.github.io/LWG/issue3237)," it's referring to an _issue_ on CWG's
or LWG's plate — an open question raised by the wording of the Standard. LWG's FAQ gives
[an exhaustive list](https://cplusplus.github.io/LWG/lwg-active.html#Status) of states
an issue can be in, including resolved states such as "[DR](#dr)" and "NAD" (Not A Defect).

## D&E

[_The Design and Evolution of C++_](https://amzn.to/2obnhVK), a book by Bjarne Stroustrup first published
in 1994. [In Stroustrup's words](http://www.stroustrup.com/books.html),
"D&E discusses why C++ is the way it is. It describes the design of C++.
The emphasis is on the overall design goals, practical constraints, and people that shaped C++."

## DR

"Defect Report." This means a defect or open question raised by the wording of the Standard, which
has been discussed and prospectively resolved by [CWG and/or LWG](#cwg-ewg-ewgi-lewg-lewgi-lwg), resulting in an
amendment or erratum to the Standard of a technical nature (as opposed to a merely editorial fixup).
Formally, I believe the term "DR" refers to the _question_ or _problem_, whereas the ultimately
adopted _solution_ is formally a "technical corrigendum" (TC). In common parlance, you'll hear
"ah, that issue was resolved by DR 409" — "DR 409" being a colloquial shorthand for "the resolution
of the DR arising from [LWG issue 409](https://cplusplus.github.io/LWG/issue409)," and it is only from
context that we can tell it was LWG 409, not [CWG 409](https://cplusplus.github.io/CWG/issues/409.html).
Defect Reports themselves are not numbered, _per se_.

DRs are often applied retroactively. For example,
[N3922 "New Rules for auto deduction from braced-init-list"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2014/n3922.html) (James Dennett, February 2014),
being associated with an (unnumbered) DR,
was not merely adopted into the C++17 working draft, but also retroactively applied
to the already-published C++11 and C++14 standards.
This manifests as a [difference in `-std=c++11` behavior](https://godbolt.org/z/AI7WpW) between Clang 3.5.1
(shipped January 2015) and Clang 3.8 (shipped March 2016) — not as a difference between `-std=c++11` and
`-std=c++17` on any compiler! So in this sense, to "resolve _foo_ as a DR" connotes "to apply the same fix
uniformly across all language modes."

## EB

"Erroneous Behavior." This is a new term of art [introduced](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2795r5.html) into the C++26 draft.
The idea is that some things that were full-on ["UB"](#ub) before, such as reading from
uninitialized variables, are now merely "erroneous." Erroneous behavior is defined as
["well-defined behavior that the implementation is recommended to diagnose."](https://eel.is/c++draft/defns.erroneous)

You can think of the real feature here as _erroneous values_, which are a standardese-ization of
LLVM's [`poison` values](https://llvm.org/docs/LangRef.html#poison-values).
<a href="https://eel.is/c++draft/basic.indet">[basic.indet]</a> says that reading from an uninitialized
byte is not itself EB; rather, it yields an _erroneous value_. Actually _doing_ anything
with an erroneous (poison) value, however, will produce EB. Consider:

    unsigned char g;
    void f() { unsigned char c; g = c; }

Inside `f`, loading from the uninitialized `c` produces an erroneous value. But storing into
`g` cannot _store_ an erroneous value, because of course it must store a value between 0 and 255 inclusive,
and all of those are real values. So (per [[basic.indet]/2](https://eel.is/c++draft/basic.indet#2)) when we
do this store
"the behavior is erroneous and the result of the evaluation is [...] not erroneous." If the implementation
diagnoses erroneous behavior, it will diagnose the assignment `g = c` at runtime. But if it _doesn't_
diagnose erroneous behavior, then it will just make sure to initialize `c` to zero. (Or some other
value "determined by the implementation independently of the state of the program" — but in particular
[it is _forbidden_](https://stackoverflow.com/questions/78792583/in-c26-are-implementations-required-to-initialize-uninitialized-variables-t)
in C++26 to do what C and C++ have always done and just "initialize" `c` to whatever garbage
was already on the stack, because that wouldn't be "independent of the state of the program.")

In practice, you should treat "EB" the same as "UB"; don't think it's "safer" just because the acronym
has changed! For example, look at what GCC does with this function ([Godbolt](https://godbolt.org/z/Y41cxsjK8)):

    int a[10] = {};
    int h(bool b) {
      if (b) {
        unsigned char c; // uninitialized
        return a[c]; // EB followed by UB; thus inferred to be unreachable
      } else {
        return 42; // this is the only relevant branch
      }
    }

If `b` were true, then the `if` branch would have EB (when the erroneous value loaded from `c` is promoted to `int`).
But the "erroneous value's" actual _value_ is chosen by the optimizer; and it will certainly prefer to
choose a large value such as 42, such that `a[c]` indexes off the end of the array and causes UB.
Since GCC doesn't diagnose EB at runtime, it is free to consider the EB branch unreachable and
optimize `h` down to a simple `return 42`, exactly as it could in the bad old days when loading from `c` was UB.

Who _does_ diagnose EB? Well, C++26 isn't out the door yet, so right now of course the answer is "nobody";
but in the long run you should expect this to be the domain of "sanitizers" such as [MSan and UBSan](#asan-msan-tsan-ubsan).
Naturally it is _not_ the domain of UBSan, because EB isn't UB! It's possible that we'll eventually see
some sort of "EBSan" sanitizer whose job is to diagnose EB at runtime.

## EBO, EBCO

"[Empty Base (Class) Optimization](https://en.cppreference.com/w/cpp/language/ebo)."
This is the thing that, given the code

    struct A {};
    struct B { int i; };
    struct C : public A { int i; };

makes `B` and `C` have the same size
(and in fact [the same layout](https://www.youtube.com/watch?v=yTb6xz_FSkY)).

## EDG

[Edison Design Group](https://www.edg.com/company), a small company that makes compiler front-ends.
Its C++ front-end was first released in 1992. Essentially all of its employees are extremely core
members of the C++ standardization committee (WG21).

## EH, TDEH

"Exception Handling." See also: [SEH](#seh).

When talking about standard C++ exception handling, you may see references to "`setjmp`/`longjmp`
exception handling" versus "table-driven exception handling" (TDEH). The former is the old-school
implementation used in the code generators for compilers like Cfront ([source](https://pdfs.semanticscholar.org/6e3a/dc7855c03bac7458e41adb007557102bd52a.pdf)):
opening a new scope essentially calls `setjmp` to build a dynamic stack of "things that need to be
unwound when we throw," and then `throw` essentially calls `longjmp` as many times as it needs in
order to unwind the stack back to the appropriate handler.

TDEH has pretty much taken over the world in the past two decades. Opening a new scope in TDEH is
a free operation; for this reason it's also been colloquially called "zero-cost exceptions."
`throw` essentially consults a static data table of "things that need to be unwound when we throw
from this particular stack frame," and then unwinds one stack frame, and repeats, as many times as
it needs in order to unwind the stack back to the appropriate handler. TDEH pays a relatively larger
up-front cost in _data size_; `setjmp`/`longjmp` exception handling pays a relatively larger runtime cost
and also a larger cost in _code size_.

## EoP, EOP

Another overloaded acronym with at least two obscure meanings.

The original meaning of "_EoP_" is
[_Elements of Programming_](http://elementsofprogramming.com/) (2009),
a book by Alexander Stepanov (of [STL](#stl) fame) and Paul McJones.

In C++23, I've seen "EOP" used as an abbreviation for "explicit object parameter."
This is the new-in-C++23 feature that lets you write

    struct S {
        int x;
        int f(this S self) { return self.x; }
    };

We might say that `self` is an _explicit object parameter_, or that `S::f`
is an _explicit object parameter function_. (MSVC calls it an "explicit object
member function," without the word "parameter.") Personally, I don't recommend
abbreviating this expression, yet; but we'll see how usage evolves.

For more on explicit object parameters, see the paper that introduced them
into C++23: [P0847 "Deducing `this`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p0847r7.html)
(Gašper Ažman et al., July 2021).

## FAM

"[Flexible array member](https://en.wikipedia.org/wiki/Flexible_array_member)."
This is the C99 feature that lets you write

    struct S {
        int x, y, z;
        char extra_space[];
    };
    struct S *ps = malloc(sizeof(S) + 10);
    strcpy(ps->extra_space, "some data");

The "flexible" member must have no array bound, and must appear as the last member of the struct.

Flexible array members are not part of C++, and likely never will be, officially.
Accessing off the end of an object will always technically be undefined behavior.
Nevertheless, C++20's [destroying `delete`](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0722r1.html)
facility was designed specifically to support FAM-like techniques.

## GCC

Originally the "GNU C Compiler" (where "[GNU](https://en.wikipedia.org/wiki/GNU)"
famously stands for "GNU's Not Unix"). Since 1999 ([source](https://gcc.gnu.org/wiki/History)), the
acronym has stood for "GNU Compiler Collection." One of the big three C++ compiler vendors, besides
Clang and [MSVC](#msvc).

## GMF, PMF

"Global module fragment" and "private module fragment." In C++20 Modules syntax, the global module
fragment is the portion of a module unit (that is, a [TU](#tu) containing a _module-declaration_
such as `module hello;`) which appears at the very top of the source code, preceded by the line
`module;` and followed by the _module-declaration_.

The private module fragment appears at the very bottom, preceded by the line
`module :private;`. Things in the PMF are not `export`ed; you can modify things in module
`hello`'s PMF without needing to recompile other TUs that `import hello`.

    module;
    #include <unistd.h>   // this is part of the GMF
    extern int gmf();     // provided by some non-modules third-party code

    module hello;
    export void pmf();
    export void foo() { gmf(); pmf(); }

    module :private;
    void pmf() { }    // this is part of the PMF


I don't fully understand why the PMF is needed; it seems like we could have put the definition of
`void pmf()` up in the module, and simply refrained from marking that _definition_ as `export`.
But perhaps I'm missing something. Anyway, for more on module fragments, see `vector<bool>`'s blog post
["Understanding C++ Modules, Part 3"](https://vector-of-bool.github.io/2019/10/07/modules-3.html)
(October 2019).

"PMF" also happens to be an acronym for "pointer to member function," as in the
expression `&Foo::setValue` or the type `void (Foo::*)(int)`.

## HALO

"Heap Allocation eLision Optimization." This is the optimization on C++20 coroutines
referred to in [Gor Nishanov's talk](https://www.youtube.com/watch?v=8C8NnE1Dg4A&t=6m00s)
on the "disappearing coroutine" (CppCon 2016).
See ["Announcing `Quuxplusone/coro`"](/blog/2019/07/03/announcing-coro-examples/) (2019-07-03),
specifically [this example](https://coro.godbolt.org/z/5vjlk8); see also
[P0981 "HALO: the joint response"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0981r0.html)
(Richard Smith & Gor Nishanov, March 2018).

Normally, each time you enter at the top of a C++20 coroutine and create its return object
(regardless of whether you're multi-threading)
you'll have to heap-allocate enough space to store the coroutine's stack frame. However, in
some specific cases where the compiler can statically determine the lifetime of the coroutine
frame — determine that it will never "escape" from a very localized region of the code — then
the compiler can find a place higher up on the stack to allocate space for it. In that case,
the heap-allocation becomes unnecessary and can be "elided." This can happen quite often in the
generator/`co_yield` use-case, if your generator type is carefully crafted.
My understanding is that HALO will basically never happen in the multi-threaded/`co_await` use-case.

Even when the heap-allocation cannot be elided, C++20 `std::coroutine_traits` provides
rudimentary hooks for the programmer to customize the heap allocation mechanism. See
["C++ Coroutines: Understanding the promise type"](https://lewissbaker.github.io/2018/09/05/understanding-the-promise-type#customising-coroutine-frame-memory-allocation)
(Lewis Baker, September 2018).

## ICE

"Internal compiler error." A compiler (for C++ or any other language) should always be able to compile
your code or else tell you what's wrong with it. If you give the compiler some input and it gets
so confused that _the compiler itself_ assert-fails, then you've discovered an internal compiler error.
Many compilers' assertion-failure messages actually contain the words "internal compiler error"
(for example, [GCC](https://github.com/gcc-mirror/gcc/search?q=DK_ICE)). I don't know the original
inventor of the term.

If the compiler segfaults or dies from an unhandled exception, you could reasonably call that an
"internal compiler error" too. Some compilers will install signal handlers or exception handlers
to turn such bugs into assertion failures that actually print "internal compiler error"; some won't.

Sadly for clarity of communication, "ICE" is also the initialism for "integral constant expression."

> An _integral constant expression_ is an expression of integral or unscoped enumeration type,
> implicitly converted to a prvalue, where the converted expression is a core constant expression.
> [Note: Such expressions may be used as bit-field lengths, as enumerator initializers if the
> underlying type is not fixed, and as alignments. —end note]
> —[N4810 [expr.const]/5](http://eel.is/c++draft/expr.const#7)

## IFNDR

"Ill-formed, no diagnostic required." To a first approximation, this means the exact
same thing as "undefined behavior" (UB). Specifically,

> If a program contains a violation of a rule for which no diagnostic is required,
> this document places no requirement on implementations with respect to that program.
> —N4810 [[intro.compliance]/2.3](http://eel.is/c++draft/intro.compliance#2.3)

The standard sometimes uses the phrase exactly
(e.g. [[dcl.attr.noreturn]/1](http://eel.is/c++draft/dcl.attr.noreturn#1)), and
sometimes uses variations (e.g. [[using.headers]/3](http://eel.is/c++draft/using.headers#3)):

> A translation unit shall include a header only outside of any declaration or definition,
> and shall include the header lexically before the first reference in that translation unit
> to any of the entities declared in that header. No diagnostic is required.

What this means, implicitly, is that *if the preceding "shall" statement
is violated by the user's program,* no diagnostic is required. So you'll have an ill-formed program
(which by definition is "not C++"). Because no diagnostic is required, your C++ compiler is not
required to tell you that its input was "not C++"; but, because the input wasn't C++, your C++ compiler
is not required to produce any particular output. It might produce
[nasal demons](http://www.catb.org/jargon/html/N/nasal-demons.html). It might even [ICE](#ice), although
that wouldn't be nice.

Frequently, the distinction between "IFNDR" and "[UB](#ub)" is that "IFNDR" connotes a static property
of the code, whereas "UB" connotes a runtime property. A division routine
can have conditional UB at runtime (if you pass it a divisor of zero), but code cannot be conditionally
IFNDR at runtime. Vice versa, if some mistake (such as an ODR violation) could conceivably result in
a linker error, then the mistake will typically be described in terms of "IFNDR" rather than "UB."
Recall that if a program is _ill-formed_, then the compiler doesn't have to generate code for it;
but if a program is _well-formed_, then the compiler must generate some sort of code for it,
even in the presence of undefined behavior.

## IILE

"Immediately invoked lambda expression." This is a relatively obscure idiom in C++, but we plucked
it from JavaScript, where it is called "[immediately invoked function expression](https://en.wikipedia.org/wiki/Immediately_invoked_function_expression)"
(IIFE). In JavaScript, IIFEs are typically used to avoid polluting the current scope with helper
variables — which is important because in JavaScript you're often working in the global scope.
In C++, the idiom is typically used to avoid mutation in the current scope. That is, rather than
write an initialization as

    void test(int *first, int *last, bool(*pred)()) {
        std::vector<int> v(first, last);
        std::sort(v.begin(), v.end());
        std::stable_partition(v.begin(), v.end(), pred);

        // Now do several things that don't involve modifying `v`
    }

you might move the initial sorting-and-partitioning into a helper function — which you make
a lambda so that it can use `first`, `last`, and `pred` without cumbersome argument-passing,
and so that you don't have to scroll around while reading the code. The end result:

    void test() {
        const std::vector<int> v = [&]() {
            std::vector<int> v(first, last);
            std::sort(v.begin(), v.end());
            std::stable_partition(v.begin(), v.end(), is_prime);
            return v;
        }();

        // Now do several things that don't involve modifying `v`
    }

For a dangerous example of using IILEs with C++20 coroutines, see
["C++2a Coroutines and dangling references"](/blog/2019/07/10/ways-to-get-dangling-references-with-coroutines/#exciting-new-way-to-dangle-a-reference) (2019-07-10).

The C++20 standard's [notion of "immediate invocation"](http://eel.is/c++draft/expr.const#def:immediate_invocation)
has absolutely nothing to do with IILEs; it has to do with the evaluation of C++20 `consteval` functions.

## IWYU

"Include What You Use." This is both a slogan _and_ the name of [a specific libclang-based tool](https://include-what-you-use.org/).
Basically, "Include What You Use" means that if you use an entity, you should explicitly `#include`
the header that is documented to provide that specific entity. Don't rely on its being provided transitively
by including any other header, because what's true today might be false tomorrow.
(See also: [Hyrum's Law](https://www.hyrumslaw.com/).)

    #include <vector>
    std::vector<std::unique_ptr<int>> v;  // IWYU violation

Since `std::vector` depends on `std::allocator`, which is defined in `<memory>`, you might expect that
`<vector>` needs to include `<memory>` — and indeed on libc++ 12.0 it does. But on libstdc++, MSVC, and
libc++-sometime-in-the-future (as of July 2021), this [TU](#tu) fails to compile.
To fix it, you should "Include What You Use":

    #include <memory>  // for unique_ptr
    #include <vector>  // for vector
    std::vector<std::unique_ptr<int>> v;

The `include-what-you-use` command-line tool (on OSX, simply `brew install include-what-you-use`) can detect
and even suggest how to correct IWYU violations. Its output is usually pretty self-explanatory:

    $ include-what-you-use test.cpp

    test.cpp should add these lines:
    #include <memory>  // for unique_ptr

    test.cpp should remove these lines:

    The full include-list for test.cpp:
    #include <memory>  // for unique_ptr
    #include <vector>  // for vector
    ---

## LTO

"Link-Time Optimization." Any kind of optimization that requires looking at the whole program —
thus also sometimes known as "whole-program optimization" (WPO) or "whole-program analysis" (WPA).
This is a special case of
"[interprocedural optimization](https://en.wikipedia.org/wiki/Interprocedural_optimization)" (IPO).

LLVM's docs have [a great example](https://llvm.org/docs/LinkTimeOptimization.html#example-of-link-time-optimization)
showing how LTO can iteratively remove dead (but non-static) functions, and then update global
invariants to cause even more code to go dead.

When I worked at Green Hills, their linker was known for its super aggressive link-time optimizations
such as [function outlining](https://jakewharton.com/r8-optimization-method-outlining/)
(i.e., the opposite of function inlining) and unused virtual function deletion (UVFD).

## MDT

"Most Derived Type." Also known as the "dynamic type," or
([in libsupc++](https://github.com/gcc-mirror/gcc/blob/873d395c2976a8321cec03f21d77e11f746da7c0/libstdc%2B%2B-v3/libsupc%2B%2B/dyncast.cc#L62))
the "whole type," this is the actual, real, true type of an [OOP](#oop) object, as opposed to
the static type of the pointer through which you're currently accessing it. For example, in

    Fruit *p = new Apple;

the static type of `*p` is `Fruit`, but the _dynamic_ type of that object — that object's _most derived type_ —
is `Apple`.

## MSVC

Microsoft Visual C++ — in C++ contexts, essentially a synonym for Microsoft Visual Studio (VS or MSVS).

## MVP

Another overloaded acronym. Since it's so overloaded, you probably won't hear the initialism
used without any context — and shouldn't use it that way, either.

All the top Google hits are for
"[Model–View–Presenter](https://en.wikipedia.org/wiki/Model%E2%80%93view%E2%80%93presenter),"
a variation on Model–View–Controller (MVC).

Microsoft identifies "MVPs" ("[Most Valuable Professionals](https://en.wikipedia.org/wiki/Microsoft_Most_Valuable_Professional)")
in the community; the [list](https://mvp.microsoft.com/en-us/MvpSearch?&kw=c%2B%2B&ex=Developer+Technologies&ps=48&pn=1)
as of 2021 includes such luminaries as Kate Gregory, Jon Kalb, and Jason Turner.

Of course in baseball "MVP" stands for "[Most Valuable Player](https://en.wikipedia.org/wiki/Most_valuable_player),"
and in Silicon Valley it often stands for "[Minimum Viable Product](https://en.wikipedia.org/wiki/Minimum_viable_product)."

But its most C++-specific meaning is "Most Vexing Parse." The Most Vexing Parse
affects old-school direct-initialization with parentheses, and goes like this:

    const char *s = "foo.txt";
    fs::path t(s);                // OK: t is a variable
    fs::path u = std::string(s);  // OK: u is a variable
    fs::path v(std::string(s));   // Yuck: v is a function!
    fs::path w(std::string());    // Yuck: w is a function too!

`v` and `w` are perfectly valid C++ code. In `v`, the compiler
ignores the "redundant" parentheses around `s` and parses it
as equivalent to

    fs::path v(std::string s);    // v is a function

In `w`, the compiler treats `std::string()` as a function type —
the type of an unnamed function parameter — and decays it to produce
the equivalent of

    fs::path w(std::string (*param)());  // w is a function

The simplest way to fix a Most Vexing Parse issue is to stop declaring things
with `T x(y)` syntax, and use `T x = y` or `auto x = T(y)` instead.
See ["The Knightmare of Initialization in C++"](/blog/2019/02/18/knightmare-of-initialization/) (2019-02-18).

    auto v = fs::path(std::string(s));  // OK and best
    auto w = fs::path(std::string());   // Also works for w

Sillier alternatives, which satisfy the parser without improving
readability for the programmer, are to replace the outer parentheses
with curly braces, or to double them up, or to use a cast operator:

    fs::path v{std::string(s)};   // OK but not best
    fs::path v((std::string(s)));
    fs::path v(static_cast<std::string>(s));

## NSDMI

"Non-static data member initializer." This is the C++11 feature that allows you to write
"initializers" on the member variables of a class.

    struct S {
        static int s = 1;  // an initialized static data member
        int x;             // a non-static data member, sans initializer
        int y = 2;         // NSDMI!
    };

This term gained currency in 2019 because of Corentin Jabot's proposal for
["`auto` NSDMIs"](https://cor3ntin.github.io/posts/auto_nsdmi/) — non-static data members
whose type is deduced (at class-definition time) from the type of their initializer.
`auto` NSDMIs are not (yet?) part of any draft standard.

## NTBS

"Null-Terminated Byte String." Not to be confused with the [NTSB](https://en.wikipedia.org/wiki/National_Transportation_Safety_Board).
Most C and C++ functions that take a parameter of type `const char *` expect that it points to
the first character of an NTBS. `"hello world"` is an example of an NTBS.

Here, "null-terminated" means it ends with a `'\0'` byte (ASCII NUL), and "byte string" just means that
we're [ignoring all encoding-related nonsense](/blog/2019/02/03/on-teaching-unicode/).
Cppreference distinguishes NTBS from [NTMBS](https://en.cppreference.com/w/c/string/multibyte)
("null-terminated multibyte string," e.g., UTF8-encoded) and even
[NTWS](https://en.cppreference.com/w/c/string/wide) ("null-terminated wide string",
i.e., a sequence of `wchar_t`s ending with `wchar_t(0)`).

## NTTP

"Non-type template parameter." This is a weird one, because you'd think by symmetry it ought to
be spelled "TNTP" — template type parameter, template template parameter, template non-type parameter,
right? But no: C++ has "template type parameters" and "non-type template parameters."

A template type parameter is like `template<class C>`.

A template template parameter is like `template<template<class> class TC>`.

A non-type template parameter is like `template<int V>` or `template<auto V>`.

Until C++20, NTTPs were allowed to be of only certain primitive types: integers, enums, pointers, and references.
Starting in C++20, NTTPs are allowed to be of floating-point types (as in `MyTemplate<3.14>`) or
even certain kinds of class types; see [CNTTP](#cnttp).

## NUA

`[[no_unique_address]]`, an attribute that is new in C++20. I would never abbreviate it myself,
but I'm starting to notice people using "NUA" in Slack, so, into the glossary it goes! Not to be
confused with [NUMA](https://en.wikipedia.org/wiki/Non-uniform_memory_access).

C++20 introduced `[[no_unique_address]]` basically to get rid of the need for the [EBO](#ebo-ebco).

## NVI

"Non-virtual interface." This rare acronym refers to the increasingly common (and, in my view, good)
practice of [separating the two pieces of the customization
point](/blog/2018/03/19/customization-points-for-functions/) even for plain
old classical polymorphism. The piece specialized by the derived class stays as a virtual
function (but becomes private); the piece invoked by the caller stays public (but becomes non-virtual).

    class AbstractWidget {
        virtual void do_frobnicate() = 0;
    public:
        void frobnicate() { this->do_frobnicate(); }
    };

    class DerivedWidget : public AbstractWidget {
        // note: implicitly "private"
        void do_frobnicate() override { ... }
    }

The Non-Virtual Interface pattern is used in `<iostream>`
([public non-virtual `sputn` and protected virtual `xsputn`](https://en.cppreference.com/w/cpp/io/basic_streambuf/sputn)),
and also in [PMR](#pmr)
([public non-virtual `allocate` and private virtual `do_allocate`](https://en.cppreference.com/w/cpp/memory/memory_resource)).

## ODR

The "One-Definition Rule." See [cppreference](https://en.cppreference.com/w/cpp/language/definition).
The ODR is quite long and involved — it takes up four printed pages in the paper standard! But
the important points are:

> Every program shall contain exactly one definition of every non-inline function or variable that
> is *odr-used* in that program outside of a discarded statement; no diagnostic required.
> —N4810 [[basic.def.odr]/10](http://eel.is/c++draft/basic.def.odr#10)

An *odr-use*, to a first approximation, is any use that requires the used entity to be defined somewhere.
(This excludes things like asking for the `sizeof` or `decltype` of a variable.)
By "discarded statement," they mean the untaken branch of an `if constexpr`.

> There can be more than one definition of a class type, enumeration type, inline function
> with external linkage, inline variable with external linkage, class template, non-static
> function template, concept, static data member of a class template, member function of a
> class template, or template specialization for which some template parameters are not specified
> in a program provided that [...] the definitions satisfy the following requirements. [...]
>
> - each definition of D shall consist of the same sequence of tokens
>
> —N4810 [[basic.def.odr]/12](http://eel.is/c++draft/basic.def.odr#12)

The first quote above disallows programs like

    // alpha.cpp
    int i = 0;

    // beta.cpp
    int i = 0;

The second quote disallows programs like

    // alpha.cpp
    inline int foo() { return 0; }

    // beta.cpp
    inline int foo() { return 1; }

(but if you changed that `1` to a `0`, then the program would be well-formed).

Both of these programs exhibit "ODR violations."
A program which violates the ODR is [IFNDR](#ifndr). In the first example above, you'll likely get
a linker error; in the second example, you'll likely get a compiled program with unexpected runtime
behavior.

## OOP

"Object-oriented programming," of course. Sometimes shortened even further to "OO,"
as in "OO principles."

## PCH

"[Pre-compiled header](https://gcc.gnu.org/onlinedocs/gcc/Precompiled-Headers.html)."
Normally, when you `#include "foo.h"`, you have to recursively open all
the files that it includes and parse all that C++ code (Include guards defend against doing this more
than once _per translation unit_, but if you have a hundred translation units, you'll still be opening all
those files a hundred times.) Therefore, most compilers support some way to pre-compile "foo.h"
so that when you say `#include "foo.h"`, the compiler actually just opens "foo.pch" — a preprocessed, parsed,
and compressed representation of _all_ the code recursively included by "foo.h".

Most compilers restrict the usage of PCH files — e.g. requiring that each translation unit can only
include one PCH, and it must be the very first non-comment line in the file. And notably, from
[GCC's docs](https://gcc.gnu.org/onlinedocs/gcc/Precompiled-Headers.html):

> The precompiled header file must have been produced by the same compiler binary
> as the current compilation is using.

That is, _PCH files are not a distribution format._ See also: [BMI](#bmi-cmi).

## PGO

"[Profile-Guided Optimization](https://en.wikipedia.org/wiki/Profile-guided_optimization),"
occasionally called "profile-driven optimization" (PDO).
You compile your program with profiling instrumentation;
then you run it through its paces to collect a profile; and then you feed that profile back
into a second invocation of the compiler. From the profile, the compiler can tell what loops
are hot, what functions are frequently called together, and so on; which can lead to better
codegen the second time around.

This is a topic that I wish I knew more about.

## PIMPL

"Pointer to IMPLementation." Variously capitalized "PIMPL," "PImpl," or "pImpl," this is a technique
for moving expensive implementation details out of your most commonly traveled .h files and
into separately compiled .cpp files. See [cppreference](https://en.cppreference.com/w/cpp/language/pimpl).

## PMR

"Polymorphic Memory Resources." C++17 added the PMR library, mostly in the header `<memory_resource>`,
and mostly in the nested namespace `std::pmr`. The most important components are
`std::pmr::memory_resource`, which is a traditional abstract base class; and
`std::pmr::polymorphic_allocator<T>`, which is an allocator (similar to `std::allocator<T>`) which
holds within itself a pointer to a `memory_resource` that it uses to fulfill requests for memory.

For more on PMR, see ["`<memory_resource>` for libc++"](/blog/2018/06/05/libcpp-memory-resource/) (2018-06-05)
and my talk ["An Allocator is a Handle to a Heap"](https://www.youtube.com/watch?v=0MdSJsCTRkY)
(C++Now 2018, CppCon 2018).

## POCCA, POCMA, POCS, SOCCC

"`propagate_on_container_copy_assignment`," "`propagate_on_container_move_assignment`,"
"`propagate_on_container_swap`," and "`select_on_container_copy_construction`," respectively.
When you have an STL container (such as `std::vector`) with a custom allocator type, you can write

    A a1("foo");
    A a2("bar");
    assert(a1 != a2);  // for the sake of argument
    std::vector<int, A> v1(a1);
    std::vector<int, A> v2(a2);
    v1 = v2;                     // A
    v1 = std::move(v2);          // B
    std::swap(v1, v2);           // C
    std::vector<int, A> v3(v1);  // D

Before line A, we clearly have `v1.get_allocator() == a1`. After line A, does `v1.get_allocator()` equal
`a1` or `a2`? What about after line B? What about after line C? After line D, does `v3.get_allocator()`
equal `a1` or `A()` or something else?

The standard library's [`std::allocator_traits<A>`](https://en.cppreference.com/w/cpp/memory/allocator_traits)
exposes member typedefs named `propagate_on_container_copy_assignment`, `propagate_on_container_move_assignment`,
and `propagate_on_container_swap` that control these behaviors; they're inherited from the allocator type
`A` if possible, or else defaulted to `false_type`. If they're all `true_type`, then you have a traditional
allocator that fully enables "pilfering" the allocation from one vector into another (in cases B and C above).
If they're all `false_type`, then you have a "sticky" allocator that inhibits pilfering in cases where the
source and destination containers have different allocators. C++17's [PMR](#pmr) allocators are "sticky."
If some of POCCA/POCMA/POCS are `true_type` and some are `false_type` for the same allocator type,
then you probably have a bug.

Notice that `std::allocator_traits<A>::select_on_container_copy_construction(const A&)` is a static member function,
not a simple compile-time `true_type` or `false_type`. For example, [PMR](#pmr) makes it return the runtime result of
`std::pmr::get_default_resource()`. Also, there's no `select_on_container_move_construction`, because C++17 assumes
that move construction is always kind of "invisible" (indistinguishable from copy elision) as far as allocators are
concerned. (Read: PMR didn't need this specific customization point, so it wasn't included in the proposal.)

If your allocator type is stateless and/or sets `is_always_equal`, then the settings of
POCCA/POCMA/POCS don't really matter and might just as well be inconsistent.
For [historical reasons](https://stackoverflow.com/questions/42051917/why-does-stdallocator-require-propagate-on-container-move-assignment-to-be-tru),
`std::allocator` falls into that category.

For more on this topic, see my talk "[An Allocator is a Handle to a Heap](https://www.youtube.com/watch?v=0MdSJsCTRkY)"
(C++Now 2018, CppCon 2018). I also covered allocators in my training course
[_The STL From Scratch_](/blog/2019/06/21/stl-from-scratch-at-cppcon-2019/) (CppCon 2017, 2018, 2019).

## POD

"[Plain Old Data](https://en.cppreference.com/w/cpp/named_req/PODType)."
This term has been deprecated in C++20, along with the type trait `std::is_pod<T>`.

## POF

"Plain Old Function." This term was formerly used in the context of [`<csignal>`](https://en.cppreference.com/w/cpp/header/csignal)
to encompass the kinds of functions that were safe to install as [signal handlers](https://linux.die.net/man/7/signal).
A "Plain Old Function" was basically any function that you could have written in C, eschewing all the C++ stuff —
no `throw`, `new`/`delete`, lambdas, `std::vector`, etc. [csignal.syn] said:

> The behavior of any function other than a POF used as a signal handler in a C++ program is implementation-defined.

However, this notion of "POF" erroneously excluded things like lambdas, `std::array`, and `std::min`,
which were perfectly safe; erroneously included things like `malloc` which were not; and required the C++ implementation
to somehow "define" the result of installing a signal handler with undefined behavior. So the entire notion was
[rightly scrapped](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0270r2.html) in C++17.
Now we talk only of "signal-safe functions," with a definition reasonably similar to [the POSIX one](https://man7.org/linux/man-pages/man7/signal-safety.7.html),
and "POFs" are a thing of the past.

## PR

Most commonly, in software engineering, a "PR" is a "Pull Request" —
either literally a GitHub pull request, or in the more general sense of a patch
that's ready for code review.

Confusingly, the Clang/LLVM project has historically used "PR" to mean "Problem Report,"
so if you see a Clang or libc++ person talking about "PR12345," they probably mean Bugzilla
bug number 12345. (Clang/LLVM pull requests will be identified as "D123456,"
where the D stands for "[Differential](https://reviews.llvm.org/differential/)";
this in turn is not to be confused with WG21's use of "D1234" to identify a
pre-publication draft version of a P-numbered paper.)

In [CWG and LWG](#cwg-ewg-ewgi-lewg-lewgi-lwg), on the other hand, "P/R"
(with the slash) stands for the "Proposed Resolution" to an open issue — that is, what change
are we going to make to the standard's wording in order to close some loophole? This
meaning is not that far different from "Pull Request," but applies to standardese
rather than code.

So a libc++ maintainer might say — with a reasonable expectation of being understood —
"Implementing the P/R for LWG2357 will fix PR2468; I've done that in D123456.
The same PR includes an implementation of D2581 from next month's mailing."
Or: "I know you want this fixed, but I can't submit a PR until someone suggests a P/R!"

## QoI

"Quality of Implementation." This is the C/C++ committee wonk's version of
["That's a hardware problem."](http://www.topedge.com/home/people/gaman/jokes/b15.htm)
The C++ Standard mandates certain behaviors of a conforming C++ implementation; but
the Standard is generally silent on issues of usability, performance, debuggability,
cross-platform portability, and so on.

Oh, your compiler [ICEs](#ice) when you try to compile
`INT_MAX + 1`? That's not a conformance issue, it's a quality-of-implementation issue.
Bring it up with your compiler vendor.

If one compiler supports function types with up to 1024 parameters, and another
compiler supports only up to 16 parameters, which one is "right"? Neither. Both.
That's a QoI issue.

In [D&E](#de), Bjarne Stroustrup gives the following (non-exhaustive) examples of QoI issues:
vtable layout; name-mangling scheme; debuggability; the compiler's behavior
when confronted with an [ODR](#odr) violation; and the availability of a
[Boehm-style garbage collector](https://en.wikipedia.org/wiki/Boehm_garbage_collector).
For other QoI issues, consult the Standard's
[list of implementation-defined behaviors](http://eel.is/c++draft/impldefindex)
and [list of _suggested_ minimum implementation limits](http://eel.is/c++draft/implimits).

> Not to be confused with [the Klingon verb](https://mughom.fandom.com/wiki/Qol) "Qol."

## RAII

"Resource Acquisition Is Initialization." This is a brush capable of very broad strokes, but it
boils down to the idea that you should have destructors that free your resources, copy constructors
that duplicate your resources, and copy-assignment operators that do both. It's as broad and vague
a slogan as "move semantics" or "value semantics," though; different people might express its
fundamental precepts in slightly different ways.

Shameless plug: I'll be giving a talk on "RAII and the Rule of Zero" at CppCon this September!
[Come see it!](https://cppcon2019.sched.com)

> It should really have been called Resource Freeing Is Destruction, but
> [that acronym was taken](https://en.wikipedia.org/wiki/Radio-frequency_identification).

## RTTI

"Runtime Type Information." This is the metadata that's generated for each user-defined class type
for use by C++ runtime features such as `dynamic_cast`, `typeid`, and exception-handling.
Many compilers provide a command-line switch such as `-fno-rtti` to limit or eliminate this
information (which of course limits or eliminates the programmer's ability to use `dynamic_cast`
and `typeid`).

For background on `dynamic_cast`'s use of RTTI, see my talk
"[`dynamic_cast` from scratch](https://www.youtube.com/watch?v=QzJL-8WbpuU)" (CppCon 2017).

## RVO, NRVO, URVO

"Return Value Optimization," a.k.a. "[copy elision](https://en.cppreference.com/w/cpp/language/copy_elision)"
(or, in C++11 and later, "move elision" is also a thing).

There are two places where copy elision (a.k.a. RVO) typically kicks in. The place where C++17
_mandates_ that it happen (thanks to
"[deferred materialization of temporaries](https://blog.tartanllama.xyz/guaranteed-copy-elision/),"
a.k.a. "guaranteed copy elision") is when the object being returned is a prvalue:

    return x+1;
    return Widget();

Since the temporary object here has no name, this is colloquially known as "unnamed return value optimization"
(URVO).

The other place where copy elision might happen, but is _optional_, is when returning a local variable
by name:

    Widget x; [...]
    return x;

Since this return value has a name, this is colloquially known as "named return value optimization" (NRVO).

Notice that "URVO" can be seen as a special case of guaranteed copy elision, which would also kick in
if you wrote something like

    void foo(Widget);
    void test() {
        foo(Widget());
    }

Our `Widget` is constructed directly into the parameter slot. This is arguably not "RVO"
since it doesn't involve "return values" — it is not even "copy elision" according to the paper standard —
but it is definitely still "deferred materialization of temporaries."

I am not aware of any way to trigger "NRVO" other than via a `return` statement. It is
technically allowed in `throw` statements as well, but
[no compiler implements that feature](/blog/2018/04/09/elision-in-throw-statements/).
It is also technically allowed in C++20 `co_return` statements, but neither Clang nor MSVC
implement that feature. (MSVC doesn't even do implicit move!)

For more on this topic, see my talk
"[RVO Is Harder Than It Looks](https://www.youtube.com/watch?v=hA1WNtNyNbo)" (CppCon 2018).

## SBO, SOO, SSO

"Small Buffer Optimization," referring to a small buffer held within the memory footprint of a type
that in the general case would have to dynamically allocate memory to hold something or other.
The small buffer is used to avoid that memory allocation in "small" cases. If our buffer is being
used to store an object, then we might say that our type has a "Small Object Optimization" (SOO).
If our buffer is being used to store a string, then we have a "Small String Optimization" (SSO).
For slightly more on SBO/SOO/SSO, see
"[The space of design choices for `std::function`](/blog/2019/03/27/design-space-for-std-function/#sbo-affects-semantics)" (2019-03-27).

## SCARY iterators

This silly initialism was introduced in
[N2911 "Minimizing Dependencies within Generic Classes for Faster and Smaller Programs"](http://www.open-std.org/jtc1/sc22/WG21/docs/papers/2009/n2911.pdf)
(Tsafrir, Wisniewski, Bacon, Stroustrup; June 2009). It refers to the template-metaprogramming technique
of keeping "policy parameters" such as allocators at the outermost possible level and not letting them
pollute the lower levels of the system. It's the difference between

    template<class T, class A>
    class vector {
        struct iterator { ... };
    };

    // Many distinct iterator classes
    static_assert(not std::is_same_v<
        vector<int, A1>::iterator,
        vector<int, A2>::iterator
    >);

and

    template<class T>
    class vector_iterator { ... };

    template<class T, class A>
    class vector {
        using iterator = vector_iterator<T>;
    };

    // Fewer iterator classes: hotter code
    static_assert(std::is_same_v<
        vector<int, A1>::iterator,
        vector<int, A2>::iterator
    >);

To quote the paper:

> The acronym <b>SCARY</b> describes assignments and initializations that are <b>S</b>eemingly erroneous
> (appearing <b>C</b>onstrained by conflicting generic parameters), but <b>A</b>ctually work with
> the <b>R</b>ight implementation (unconstrained b<b>Y</b> the conflict due to minimized dependencies).

See also: "[SCARY metafunctions](/blog/2018/07/09/scary-metafunctions/)" (2018-07-09).

For another example of an initialism that doesn't come (entirely) from initials, see [HALO](#halo).

## SEH

Microsoft Windows has a feature called
[Structured Exception Handling](https://en.wikipedia.org/wiki/Microsoft-specific_exception_handling_mechanisms#SEH) (SEH);
I don't know much about it except that it somehow unifies C++ exception handling and things that
would be considered "signals" in POSIX-land, such as floating point exceptions. If you pass the
`/EHc` flag to MSVC, or simply omit `/EHsc`, then you can
[catch division-by-zero as a C++ exception](https://rextester.com/KVWO69475)! This works only because
of SEH. There's no equivalent on non-Windows operating systems as far as I know.

## SFINAE

"Substitution Failure Is Not An Error." This is the slogan that helps you remember why the compiler
doesn't complain about

    template<class T>
    void f(T, typename T::type) { puts("hello"); }

    template<class T>
    void f(T, long) { puts("world"); }

    void test() {
        f(1, 2);  // "world"
    }

Here, we've already done template argument _deduction_ on our first candidate `f` and figured out
that if we call this candidate, then we'll set `T=int`. The "failure" happens during template
argument _substitution_ — when we try to extract a member type `T::type` from `T=int`, and find
that there is no such member. If we'd been compiling ordinary code and seen

    using T = int;
    void f(T, typename T::type) { puts("error"); }

then that would have been a hard error. But in this case, we merely have a _substitution failure_,
which is Not An Error. We simply discard this candidate `f` and start looking at the next
candidate, which turns out to work fine. This is exactly the same thing we would have
done if we'd failed to _deduce_ `T` originally — we simply discard the template from further
consideration and move on.

For more on SFINAE, see several of my conference talks:

- [Template Normal Programming, Part 1](https://www.youtube.com/watch?v=vwrXHznaYLA) (CppCon 2016)
- [Template Normal Programming, Part 2](https://www.youtube.com/watch?v=VIz6xBvwYd8) (CppCon 2016)
- [A Soupçon of SFINAE](https://www.youtube.com/watch?v=ybaE9qlhHvw) (CppCon 2017)

## SIOF

The "[Static Initialization Order Fiasco](https://isocpp.org/wiki/faq/ctors#static-init-order)."

C++ guarantees ([[expr.const]/2](http://eel.is/c++draft/expr.const#2)) that certain kinds of global
initializations — like `int i = 42;` — will get baked into the data section. The C++20 standard even
adds a new keyword, `constinit`, so that you can write

    constinit int i = i_sure_hope_this_function_is_constexpr(6, 9);

which means that the compiler _must_ put it in the data section or else give you a compiler error.

But for dynamically initialized global variables —

    std::string s = "hello";
    std::string t = s + " world";

If `s` and `t` are defined in the same [TU](#tu), then C++ guarantees their initializers will run
in the order you'd expect. But if they're defined in two different TUs, the linker might decide to
order the initializer for `t` _before_ the initializer for `s`. So `t`'s initialization uses `s`
as a string before `s` has actually been constructed, leading to [UB](#ub) at runtime.
([Wandbox](https://wandbox.org/permlink/qycov99Bj0TUPxsP).)

## SMF

"Special member function." According to the C++20 standard ([special/1](http://eel.is/c++draft/special#1)),
the special member functions are "default constructors, copy constructors, move constructors,
copy assignment operators, move assignment operators, and prospective destructors."
(The phrase "prospective destructor" acknowledges that a C++20 class may have many constrained
member functions all named `~T`, all of which are special member functions, but only one of which
will ultimately be selected as _the_ destructor for the class.)

Notice that "specialness" is imperfectly correlated with the ability to `=default` the member function.
`Foo::Foo(int=0)` is a default constructor (and thus an SMF of `Foo`), but cannot be `=default`ed.
In C++20, `operator<=>` and `operator==` can both be defaulted, yet neither of them is an SMF.

## STL

The "Standard Template Library." This name originally referred specifically to the library developed
by Alexander Stepanov and others at Silicon Graphics (the "SGI STL";
[PDF documentation here](http://stepanovpapers.com/STL/DOC.PDF)) and proposed for standardization
in 1993. Ironically, the name "Standard Template Library" _far_ preceded any ISO standard for C++!

The SGI STL became the basis for a lot of the C++98 Standard Library. Since "SL"
doesn't make for a great acronym, and since most of the standard library is templated in some way,
it is common for C++ programmers to refer to the entire C++ standard library as "the STL."
Others might use the term more narrowly to refer to the parts of the library that deal with
containers, iterators, algorithms, allocators, adaptors, and function objects (such as `std::less`),
excluding for example `std::shared_ptr` and `std::valarray`.
Still others might include only those parts of the modern Standard Library inherited directly from SGI,
excluding for example `std::unordered_set` and `std::move_iterator`.

Notably, [iostreams were invented by Bjarne Stroustrup circa 1984](https://stackoverflow.com/a/2753094/1424877);
therefore they are part of the "STL" only in that phrase's most inclusive (and most common) sense.

In [_Effective STL_](https://amzn.to/2ZGM0Tu) (2008), Scott Meyers defines his use of the term "STL"
as "the parts of C++'s Standard Library that work with iterators." (He explicitly excludes the
container adaptors `stack`, `queue`, and `priority_queue`, even though those _were_ part of the SGI STL,
on the grounds that they are not iterable.)
As for [_Mastering the C++17 STL_](https://amzn.to/34rWnt9) (O'Dwyer, 2017), I freely admit that
I used "STL" purely as a short form of the phrase "Standard Library" optimized for book covers and
search engines.

The other meaning of "STL" in C++ contexts is as the initials and preferred nickname of
Stephan T. Lavavej ([pronounced "Steh-fin Lah-wah-wade"](https://twitter.com/StephanTLavavej)).
As of 2019, STL happens to work on [MSVC](#msvc)'s implementation of _the_ STL.

## TBAA

"Type-based alias analysis." This is what lets the compiler conclude that in the following
snippet ([Godbolt](https://godbolt.org/z/pVVlZM)), `*pi` and `*pf` cannot alias each other.

    int foo(int *pi, float *pf) {
        *pi = 42;
        *pf = 3.14;
        return *pi;
    }

Since it would be [UB](#ub) to write `3.14` into a memory location and then read an `int`
back from that same memory location, and UB never happens in a correct program, the compiler
infers that `pi` must point somewhere different from `pf`. The generated code "remembers" the
value that was stored into `*pi` and returns 42 directly from the function, instead of generating
a load from memory.

On the other hand, if you changed `pi` from `int*` to `float*`, you would see the load happening,
because loading a `float` from a memory location that contains a `float` is _not_ UB.
So in that case the compiler couldn't assume that `pi` and `pf` don't alias. Other types that
might alias `float` include [plain `char`](https://godbolt.org/z/flEMi9) and
[`struct ContainsFloat`](https://godbolt.org/z/rJA1gW).

Type-based alias analysis is also known as "strict aliasing," because the analysis
[can be disabled](https://godbolt.org/z/SK3DUP) with the compiler option `-fno-strict-aliasing`.

## TCO

"Tail call optimization." This is a compiler optimization that takes what appears in the source code
as a function call (which usually pushes an activation frame onto the _call stack_) and turns
it into a plain old jump (which does not push a frame on the call stack). This is possible only
when the function call appears at the very tail end of the function — something like `return bar()`.
The compiler is saying, "Look, I know `bar` is going to end by returning to its caller; that's just
me, and I have nothing left to do. So let's just trick `bar` into returning to _my_ caller!"

Tail call optimization is often possible for `return bar();` but not for, e.g., `return bar()+1;`
— because there I _do_ have something left to do: add 1 to the result. In some languages and/or
programming idioms, instead of doing that plus-1 myself, I can tell `bar` to do it and
take my own stack frame out of the picture; this is known as _continuation-passing style_ (CPS).

In the case that the function being tail-called is the same as the current function (recursively),
you might call it _tail recursion optimization_ (TRO).

See ["It's not always obvious when tail-call optimization is allowed"](/blog/2021/01/09/tail-call-optimization) (2021-01-09).

## TMP

"Template metaprogramming." Personally, I find that the acronym "TMP" has a vague whiff of C++98
which is not shared by the simple spelled-out word "metaprogramming."

If you have lots of structs with `::type` and `::value` members, you're probably doing TMP.

## TU

"Translation unit." This is what language-lawyers say instead of ".cpp file." When you invoke the compiler
to translate your C++ code into machine code, you give it just one _translation unit_ (TU) at a time.
[Formally](http://eel.is/c++draft/lex.separate), a "TU" is the sequence of tokens that you get
after preprocessing an input file so that all its `#include` directives have been expanded and all its
`#if` directives have been resolved.

If you `#include` a file, then the text of that file becomes part of your translation unit.
In C++20 Modules, if you `import` a module then that module's "module interface unit" (MIU)
does _not_ become part of your translation unit. Rather, a module interface unit is itself a
_kind_ of translation unit: it is translated in a separate step.

## UB

"Undefined behavior." C++ shares this notion with C, and it means the same thing to both languages:

> behavior for which this document imposes no requirements
> —N4810 [ [defns.undefined]](http://eel.is/c++draft/intro.defs#defns.undefined)

> This document imposes no requirements on the behavior of programs that contain undefined behavior.
> —N4810 [[intro.abstract]/4](http://eel.is/c++draft/intro.abstract#4)

Notably, in contrast to "[IFNDR](#ifndr)," the standard acknowledges that a program containing UB is still a
well-formed program (even if its runtime _behavior_ is undefined).

You might also see the occasional "IDB" ("[implementation-defined behavior](http://eel.is/c++draft/intro.defs#defns.impl.defined)");
and if someone wrote "USB" I might grok from context that it meant
"[unspecified behavior](http://eel.is/c++draft/intro.defs#defns.unspecified)."

## UDL

"[User-Defined Literal](https://en.cppreference.com/w/cpp/language/user_literal)," as in,
the C++11 feature where you declare an `operator ""_myudl` and then are able to use `12345_myudl`
and/or `"hello world"_myudl` as "literals" in your code. The Standard Library defines several
overload sets' worth of UDLs inside [`namespace std::literals`](https://en.cppreference.com/w/cpp/symbol_index/literals).

The paper standard [says](http://eel.is/c++draft/usrlit.suffix) that all programmer-defined UDLs
must have names starting with `_` (underscore). Of course, they _cannot_ start with two underscores
or an underscore followed by a capital letter, because those are [also verboten](http://eel.is/c++draft/lex.name#3.1).

## UFCS

"Universal Function Call Syntax," a name for some kind of feature that would let you write `x.f(y)`
and `f(x, y)` interchangeably. Nobody really knows how to get this into C++. For an excellent
rundown of all the different proposals and their differences and difficulties,
see Barry Revzin's blog post "[What is UFCS anyway?](https://brevzin.github.io/c++/2019/04/13/ufcs-history)"
(April 2019).

## VLA

"Variable-length array." This is the C99 feature that lets you write

    int main(int argc, char **argv) {
        int arg_values[argc - 1];
    }

VLAs are not part of standard C++ ([and never will be](https://stackoverflow.com/questions/1887097/why-arent-variable-length-arrays-part-of-the-c-standard)).
Furthermore, C11 made VLAs a "conditional feature" which even C compilers needn't support. C11-and-later
compilers which don't support VLAs are supposed to define `__STDC_NO_VLA__` to `1`.

## VTT

"Virtual table table." In the Itanium ABI, this data structure sits alongside
the more familiar "vtable" and is used during construction and destruction of classes with virtual bases.
See ["What is the virtual table table?"](/blog/2019/09/30/what-is-the-vtt/) (2019-09-30).
