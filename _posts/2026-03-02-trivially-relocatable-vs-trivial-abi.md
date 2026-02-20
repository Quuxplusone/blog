---
layout: post
title: "A dialogue on trivial-abi and trivially relocatable"
date: 2026-03-02 00:01:00 +0000
tags:
  abi
  library-design
  relocatability
  socrates
  triviality
excerpt: |
  I wrote in ["`[[trivial_abi]]` 101"](/blog/2018/05/02/trivial-abi-101/) (May 2018),
  during the active development of the `[[clang::trivial_abi]]` attribute:

  > <b>Relation to trivial relocatability:</b> None... well, some?
  >
  > As you can see, there is no requirement that a `[[trivial_abi]]` class type
  > should have any particular semantics for its move constructor, its destructor,
  > or its default constructor. Any given class type will _likely_ be trivially
  > relocatable, simply because most class types are trivially relocatable by accident...

  A correspondent writes in with a query for Socrates
  (previously seen on this blog [in July 2023](/blog/2023/07/16/why-span-has-no-resize/)).

  _Epistolographos writes:_ Actually, I think trivial-abi-ness and trivial relocatability are
  almost the same thing.
---

I wrote in ["`[[trivial_abi]]` 101"](/blog/2018/05/02/trivial-abi-101/) (May 2018),
during the active development of the `[[clang::trivial_abi]]` attribute:

> <b>Relation to trivial relocatability:</b> None... well, some?
>
> As you can see, there is no requirement that a `[[trivial_abi]]` class type
> should have any particular semantics for its move constructor, its destructor,
> or its default constructor. Any given class type will _likely_ be trivially
> relocatable, simply because most class types are trivially relocatable by accident...

A correspondent writes in with a query for Socrates
(previously seen on this blog [in July 2023](/blog/2023/07/16/why-span-has-no-resize/)).

_Epistolographos writes:_ Actually, I think trivial-abi-ness and trivial relocatability are
almost the same thing. Let me quote [your own words](/blog/2018/05/02/trivial-abi-101/#the-intuition-here-is-that-when)
back to you:

> When a thing is trivial-abi, any time you expect _copy_ you might actually get _copy-plus-`memcpy`:_
> The "put it in a register and then take it back out" operation is essentially tantamount to `memcpy`.
> And similarly when you expect _move_ you might actually get _move-plus-`memcpy`._
>
> Contrariwise, when a thing is trivially relocatable, any time you expect _copy-plus-destroy_
> you might actually get _`memcpy`_. And similarly when you expect _move-plus-destroy_
> you might actually get _`memcpy`_. You actually _lose_ calls to special member functions
> when you’re talking about "trivial relocation"; whereas with trivial-abi you never lose
> calls — you just get (as if) `memcpy` in addition to the calls you expected.

I (said Epistolographos) want to argue that passing a trivial-abi parameter _also_ replaces
move-and-destroy with `memcpy`. The way I see it, passing a trivial-abi argument doesn't just
move that argument's destruction into the callee; it conceptually introduces additional moves and
destroys, which are then folded back out. Suppose I write this in my source code:

    struct [[clang::trivial_abi]] T {};

    T produce();
    void consume(T arg) {
      use(arg);
    }
    int main() {
      consume(produce());
    }

The compiler (said Epistolographos) will essentially rewrite this into:

    struct [[clang::trivial_abi]] T {};

    T produce();
    void consume(T inreg) {
      T arg = std::move(inreg); // and destroy inreg early
      use(arg);
    }
    int main() {
      T onstack = produce();
      consume(std::move(onstack)); // and destroy onstack early
    }

Each commented line does an extra move-and-destroy — that's a relocate. Now, the compiler doesn't
actually generate calls to the move-constructor and destructor there; instead, it codegens
those lines by copying the bits of the object into a register (to relocate from `onstack` into `inreg`)
and then copying the bits back to the stack (to relocate from `inreg` into `arg`). Since the
compiler is replacing a relocation with a bitwise copy, I deduce that `T` must be a
trivially relocatable type.

— I think I see your point (said Socrates). If the compiler rewrites move-and-destroy into
a copy of `T`’s object representation, we should indeed deduce that the compiler thinks `T`
is trivially relocatable. But surely your argument is a little weak at two points: First,
how can we be certain that there _is_ a move-and-destroy happening? And second, even if
the compiler thinks `T` is trivially relocatable, is that something it _knows_ or merely
something it _assumes?_

Usually, to see whether a move-and-destroy is really happening, we would
instrument our type's special member functions, like this ([Godbolt](https://godbolt.org/z/GjarPGPW1)):

    int count = 0;
    struct [[clang::trivial_abi]] Counter {
      explicit Counter() { ++count; }
      Counter(const Counter&) { ++count; }
      void operator=(const Counter&) {}
      ~Counter() { --count; }
    };
    Counter produce() {
      Counter c;
      assert(count == 1);
      return c;
    }
    void consume(Counter arg) {
      assert(count == 1);
    }
    int main() {
      assert(count == 0);
      consume(produce());
      assert(count == 0);
    }

This test passes, no matter whether we use the `[[clang::trivial_abi]]` attribute or not.
That's because `count` counts the number of extant objects: relocating an object
from one place to another doesn't change the number of extant objects. So it is correct
to describe this `Counter` type as _trivially_ relocatable: we can replace its relocation
operation with `memcpy` and the behavior of the program doesn't change.

— Exactly! (exclaimed Epistolographos). And look here: When I ask Clang whether `Counter`
is trivially relocatable, it gives me the right answer!

    static_assert(__is_trivially_relocatable(Counter));
      // passes when Counter is [[clang::trivial_abi]]

— Yes, Epistolographos; but the same `static_assert` fails when we remove `[[clang::trivial_abi]]`
from the class definition. Yet the behavior of `Counter` hasn't changed. How can this be?

— Well, Socrates, of course the non-trivial-abi `Counter` remains "Platonically" trivially
relocatable (to coin an anachronism). But without the attribute, the compiler doesn't _know_
`Counter` to be trivially relocatable. The `[[clang::trivial_abi]]` attribute gives the
compiler this special knowledge, in the same way that P1144's `[[trivially_relocatable]]`
attribute gives the compiler special knowledge it didn't have before.

— Knowledge is good, certainly. But how can we be sure that this _is_ knowledge, and not
merely opinion? For example, suppose I change `Counter` to tally the number of constructor
calls, instead of the number of extant objects. I'll call this version `UpCounter`
([Godbolt](https://godbolt.org/z/Y9Pfxsf3W)):

    int count = 0;
    struct UpCounter {
      explicit UpCounter(int) { ++count; }
      UpCounter(const UpCounter&) { ++count; }
      void operator=(const UpCounter&) {}
      ~UpCounter() {} // here is the difference
    };
    UpCounter produce() {
      auto c = UpCounter(42);
      assert(count == 1); // A
      return c;
    }
    void consume(UpCounter arg) {
      assert(count == 1); // B
    }
    int main() {
      assert(count == 0);
      consume(produce());
      assert(count == 1); // C
    }

You'd agree that this program's behavior is fully specified by the Standard?

— Yes, Socrates, except for the use of NRVO in `produce`. That line is permitted to move-construct
from `c`, even though no reasonable compiler would do so.

— And if that line did move-construct from `c`, we'd expect `count` to equal `2` on lines B and C?

— Yes, Socrates.

— Even though that additional move-construction would be balanced out by an additional destruction?

— Yes, Socrates, because `UpCounter` increments `count` in every constructor, and never decrements it.
So every move-and-destroy operation increments `count` by one.

— For that reason, is `UpCounter` trivially relocatable?

— No, Socrates, because `UpCounter`’s move-and-destroy, unlike our original `Counter`’s, is visibly
different from a simple `memcpy`. Clang knows this:

    static_assert(!__is_trivially_relocatable(UpCounter));
      // passes as long as UpCounter is not [[clang::trivial_abi]]

— But look here, Epistolographos: I apply the `[[clang::trivial_abi]]` attribute to `UpCounter`...

    struct [[clang::trivial_abi]] UpCounter {
      explicit UpCounter(int) { ++count; }
      UpCounter(const UpCounter&) { ++count; }
      void operator=(const UpCounter&) {}
      ~UpCounter() {}
    };
    static_assert(__is_trivially_relocatable(UpCounter));

...and suddenly Clang thinks that it _is_ trivially relocatable.
Yet the behavior of `UpCounter` hasn't changed! How can this be?

— This case gives me pause, Socrates. It seems to me that `UpCounter` is not "Platonically"
trivially relocatable. Yet applying the attribute causes Clang to opine that it _is_ trivially relocatable.

— Now, according to my theory, declaring a type trivial-abi causes the compiler to generate
many additional bitwise copies. According to _your_ theory, it causes the compiler to generate
many additional move-and-destroy cycles and then inerrantly replace all of them with bitwise copies.
Either way, we agree that the compiler generates many bitwise copies. So, if I take a class like
`offset_ptr` and mark it trivial-abi, [it will not work](/blog/2018/05/02/trivial-abi-101/#we-can-easily-design-an-offset_p).
Or if I take a libstdc++-style `string` implementation ([Godbolt](https://godbolt.org/z/dznnK3zY3),
[backup](/blog/code/2026-03-02-trivial-abi-string.cpp)) and mark it trivial-abi, it won't work
either.

— That's right. It simply makes no sense to mark a non-trivially-relocatable class
trivial-abi. Doing that would be wrong.

— Here we agree. Recall the theory of attributes as warrants: a warrant is a way for the programmer,
who may be assumed to know what he's doing, to communicate Platonic Truth to the compiler.
As my friend Ko-Ko once put it: The programmer says "Make this type trivial-abi," and this type is
told off to be bitwise-copied. Consequently that type is as good as trivially relocated — practically,
it _is_ trivially relocated — and if it _is_ trivially relocated... why not say so?

Now, when my friend said that, he said it rather weakly, with a plaintive look; but in this instance
the sentiment is defensible. Since the calling convention for trivial-abi types _requires_ that a type be immune
to bitwise relocations, it seems like a plain and simple error to mark any non-trivially-relocatable type
`[[clang::trivial_abi]]`. It is an old C++ custom to treat "plain and simple errors" as "UB" — to
pretend they never happen — and thus it is perfectly reasonable for Clang to opine that every type
marked with `[[clang::trivial_abi]]` is in fact trivially relocatable.

— That's what I'm saying!

— But, Epistolographos, this happens only because the Clang maintainers decided (in early 2022,
via [D114732](https://reviews.llvm.org/D114732)) to encode that opinion into Clang.
(See [this comment thread](https://reviews.llvm.org/D114732?id=390435#inline-1097618) in particular.)

— Still, it's exactly as I said: Every type marked with `[[clang::trivial_abi]]` is
trivially relocatable!

— Well, not quite. Consider this type `A<N>`:

    struct N { ~N(); };
    template<class T>
    struct [[clang::trivial_abi]] A { T t; };
    static_assert(!__is_trivially_relocatable(A<N>));

— All right, Socrates; that's because `[[clang::trivial_abi]]` behaves more like what you call
`[[maybe_trivially_relocatable]]`, with "dull-knife" rather than "sharp-knife" semantics.
[_"In [almost](https://github.com/Quuxplusone/llvm-project/issues/52) all cases," interjected Socrates._]
In this case, because `N` is non-trivial-abi, `A<N>` quietly discards the attribute.
Let me rephrase: Every type which is
[trivial for the purposes of calls](https://itanium-cxx-abi.github.io/cxx-abi/abi.html#:~:text=trivial%20for%20the%20purposes%20of%20calls)
is trivially relocatable.

— Well, not quite. Consider a type that is _not_ marked with the attribute, but which is
trivial for the purposes of calls because it has defaulted trivial copy/move constructors and
a defaulted trivial destructor. Now, give that type a non-defaulted `operator=`. Now it's
non-trivially relocatable; but it remains trivial for the purposes of calls, because the
calling convention doesn't care about `operator=`.

— Ah, right. When we say a type is "trivial-abi," we're talking about how it interacts with the calling convention.

— Well, not quite. Consider `std::array<int, 100>`. It's trivial for the purposes of calls,
and so I think I would say that it is "trivial-abi"; but it is certainly not passed or returned
any differently from, say, `std::string`.

But I agree that in general, "trivial-abi" pertains to the calling convention, while "trivially relocatable"
pertains to library behavior. For example, it's very easy to think of types that are trivially relocatable
but not trivial-abi; `unique_ptr` is such a type.

— libc++ has an extension to make `unique_ptr` trivial-abi, doesn't it?

— Yes, it does. ([Godbolt.](https://godbolt.org/z/vhMnzs89q)) The convention by which `unique_ptr`
is passed and returned is controlled by
<span style="white-space: nowrap;">`-D_LIBCPP_ABI_ENABLE_UNIQUE_PTR_TRIVIAL_ABI`.</span>
This is completely orthogonal to the mechanism by which libc++ recognizes `unique_ptr`
as one of those trivially relocatable types for which e.g. `vector` reallocation ought
to use `memcpy` instead of move-and-destroy. Notice, in that Godbolt, how the compilations
of `demonstrate_calling_convention` differ, but both compilations successfully use `memcpy`
in `reallocate_vector`.

— So, to sum up:
Not all trivial-abi types are passed in registers; but whenever a trivial-abi type _is_ passed in registers,
it's constructed in one place and destroyed in another: effectively, it is trivially relocated.
Putting the `[[clang::trivial_abi]]` attribute on a class doesn't necessarily mean Clang
_will_ make that class trivial-abi, let alone that it will be passed in registers; but the programmer
would be foolish to put the attribute on any class that couldn't survive trivial relocation.
Clang _opines_ that the programmer is not foolish; therefore, Clang opines that any class marked
`[[clang::trivial_abi]]` should be reported as `__is_trivially_relocatable`.

— Yes, Epistolographos, I find no fault with your summary. I would add two things:
First, Clang is quite pragmatic to make that assumption. Good for Clang. And second, you must remember
that the implication goes only forward, not backward. There are very many class types which are
trivially relocatable but not trivial-abi.

— How so? Isn't it true that any trivially relocatable type can be relocated from one place to another
as-if by copying its bits into a register and out again?

— Yes, that's true.

— So marking a trivially relocatable class type as `[[clang::trivial_abi]]` must always be safe!
I think I should start using `[[clang::trivial_abi]]` as a poor man's version of P1144 `[[trivially_relocatable]]`:
I'll just put it on every class type that I want Clang to recognize as `__is_trivially_relocatable`!

— No, Epistolographos, that'll bite you in practice, for at least two reasons. First, and most importantly, "being trivial-abi"
_has ABI implications._ Remember, the libc++ maintainers know that `unique_ptr` is trivially relocatable,
but they didn't just unilaterally mark it as `[[clang::trivial_abi]]`, because that changes the calling convention
for all of its users. If you try to link one TU compiled with
<span style="white-space: nowrap;">`-D_LIBCPP_ABI_ENABLE_UNIQUE_PTR_TRIVIAL_ABI`</span> against
another TU compiled without it, you'll get linker errors at best and runtime UB at worst: one side will pass
`unique_ptr` arguments on the stack when the other expects them in registers, and vice versa.
Contrariwise, annotating a type with the "real" `[[trivially_relocatable]]` attribute doesn't change its ABI at all.
Many real-world codebases require a way to warrant a type as trivially relocatable without changing its ABI,
so your idea of using `[[clang::trivial_abi]]` to mean "trivially-relocatable" isn't directly useful to them.

— What's the second reason?

— Secondly, as we mentioned above, `[[clang::trivial_abi]]` has "dull-knife" semantics.
You can't use that attribute to warrant that, say, the `boost::shared_ptr` you're using
is _in fact_ trivially relocatable. (You wouldn't want to, because of the ABI implications; but also, you couldn't.)
Most real-world codebases require a way to warrant a type as trivially relocatable without "virally" annotating
all its data members' types; in that sense `[[clang::trivial_abi]]` doesn't serve at all as a "poor man's `[[trivially_relocatable]]`."

And that's fine: `[[clang::trivial_abi]]` doesn't exist to be a poor man's version of anything else.
It exists to change a type's ABI — hence its name! You should use it only for that purpose. The fact that
it also happens (by deliberate convention) to imply trivial relocatability is a mere lagniappe.

— I see. Well, good day, Socrates.

— Good day, Epistolographos.
