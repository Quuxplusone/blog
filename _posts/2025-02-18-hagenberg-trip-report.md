---
layout: post
title: "How my papers did at Hagenberg"
date: 2025-02-18 00:01:00 +0000
tags:
  allocators
  exception-handling
  language-design
  library-design
  proposal
  rant
  relocatability
  sg14
  triviality
  wg21
---

_"Not great, Bob!"_

Last week's Hagenberg WG21 meeting was the last "design" meeting before C++26. In three months will be
the last "wording" meeting before C++26. This means that Hagenberg was the week where everybody races
to catch the train right as it's pulling out of the station.

The awful thing about C++'s three-year release cycle is that this rush happens every three years.
Everyone is _almost constantly_ pushing: we need this now, we need that now. There's rarely as much
as a year to catch your breath before someone's pushing for the train doors again.

## Trivial relocation

First some good news: Louis Dionne (libc++) and Giuseppe D'Angelo (KDAB/Qt) brought
[P3516 "Uninitialized algorithms for relocation,"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3516r0.html)
which proposes almost exactly P1144's library API: `std::uninitialized_relocate`,
`uninitialized_relocate_backward`, `uninitialized_relocate_n`. Their first revision offered
that perhaps `std::relocate_at` should be replaced with an exposition-only _`relocate-at`_
helper; but LEWG liked it as a real API, so we'll be getting that too. P3516 was forwarded to LWG
in Hagenberg.

Now the bad news: [P2786R13 "Trivial relocatability for C++26"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2786r11.html)
was voted into the standard. Users of trivial relocation should pay attention, because what was just voted into C++26
differs from your current use of P1144 trivial relocatability in at least the following ways:

<b>1.</b>
P1144's `is_trivially_relocatable_v<T>` (cf. `absl::is_trivially_relocatable_v`, `amc::is_trivially_relocatable_v`,
`folly::IsRelocatable`) looks holistically at the whole type: if a type is trivially relocatable, then you can
optimize not just vector reallocation but also `vector::insert`, `swap_ranges`, etc. What was just standardized
in C++26 doesn't care about assignment or swap. Instead, there is a _second trait_, `std::is_replaceable_v<T>`,
which you _also_ have to query in order to find out if those value-semantic optimizations are safe. I brought
[P3559R0 "One trait or two?"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3559r0.html)
begging LEWG not to make this mistake, but LEWG voted it down.

<b>2.</b>
You should just use the P3516 entrypoints; but you might be tempted by a new low-level entrypoint
`std::trivially_relocate(first, last, result)`.
The name is potentially misleading, as _on some platforms_ (so far, just Apple's ARM64e with
[vptr signing](https://developer.apple.com/documentation/security/preparing-your-app-to-work-with-pointer-authentication))
this function's behavior is not trivial! That is, C++26 considers a Rule-of-Zero _polymorphic_ type to
be `is_trivially_relocatable` even when relocating it is _not_ tantamount to a memcpy. Even if you're
comfortable treating non-"replaceable" types as trivially relocatable, you'll need to avoid using `memcpy`
or `memmove` on a type just because it claims it's "trivially relocatable." The compiler's idea of how to codegen
that "trivial" relocation might not match yours. (To be fair, this is a variation on an age-old problem; see
["Trivially-constructible-from"](/blog/2018/07/03/trivially-constructible-from/) (2018-07-03).)

As for how P3516's `uninitialized_relocate` will deal with non-replaceable and/or "trivially relocatable
but not memcpyable" types, I think that remains to be seen.

<b>3.</b>
Instead of a single attribute, C++26 has two contextual keywords,
placed after the class-name instead of before. P1144:

    template<class T>
    struct [[trivially_relocatable]] UniquePtr {
      ~~~
    };

Current C++26 draft:

    template<class T>
    struct UniquePtr trivially_relocatable_if_eligible replaceable_if_eligible {
      ~~~
    };

<b>4.</b>
Instead of P1144 "sharp-knife" semantics, the contextual keywords have P2786 "dull-knife" semantics,
which means they are "viral downwards": every base class and every data member must be recursively tagged
with the keywords or else they have no effect on the top level. P1144:

    template<class T, int I>
    struct TupleElement { ~~~ };

    template<class, class...> struct TupleImpl;

    template<int... Is, class... Ts>
    struct TupleImpl<index_sequence<Is...>, Ts...>
        : TupleElement<Ts, Is>... {
      ~~~
    };

    template<class... Ts>
    struct [[trivially_relocatable(std::is_trivially_relocatable_v<Ts> && ...)]]
        Tuple : TupleImpl<make_index_sequence<sizeof...(Ts)>, Ts...> {
      ~~~
    };

Current C++26 draft:

    template<class T, int I>
    struct TupleElement
        trivially_relocatable_if_eligible replaceable_if_eligible { ~~~ };

    template<class, class...> struct TupleImpl;

    template<int... Is, class... Ts>
    struct TupleImpl<index_sequence<Is...>, Ts...>
        trivially_relocatable_if_eligible replaceable_if_eligible
        : TupleElement<Ts, Is>... {
      ~~~
    };

    template<class... Ts>
    struct Tuple trivially_relocatable_if_eligible replaceable_if_eligible
        : TupleImpl<make_index_sequence<sizeof...(Ts)>, Ts...> {
      ~~~
    };

This also means that a class containing a `boost::movelib::unique_ptr` or a `boost::interprocess::offset_ptr`
cannot be warranted `trivially_relocatable` at all.

<b>5.</b>
The keywords are not conditional (and probably cannot be made conditional later, because that
would cause grammatical ambiguities). So if you want your type to be conditionally trivially relocatable
based on some arbitrary condition, you must use base-class metaprogramming. P1144:

    template<class T>
    class [[trivially_relocatable(
      sizeof(T) <= 8 && is_trivially_relocatable_v<T>
    )]] SmallFunction {
      union {
        char payload_[8];
        T *t_;
      };
      SmallFunction(SmallFunction&&) noexcept { ~~~ }
      SmallFunction& operator=(SmallFunction&&) { ~~~ }
      ~SmallFunction() { ~~~ }
    };

Current C++26 draft:

    struct EmptyTR {};
    struct EmptyNonTR { ~EmptyNonTR() {} };

    template<class T>
    class SmallFunction
      trivially_relocatable_if_eligible replaceable_if_eligible
      : conditional_t<
          sizeof(T) <= 8 && is_trivially_relocatable_v<T>,
          EmptyTR,
          EmptyNonTR
        > {
      union {
        char payload_[8];
        T *t_;
      };
      SmallFunction(SmallFunction&&) noexcept { ~~~ }
      SmallFunction& operator=(SmallFunction&&) { ~~~ }
      ~SmallFunction() { ~~~ }
    };

I hope that someone will bring a paper complaining about any or all of these deficiencies to
the Sofia meeting on [June 16th, 2025](https://isocpp.org/std/meetings-and-participation/upcoming-meetings).
If you need help getting a paper into the system, please don't hesitate to contact me!

If you currently use trivial relocation in your library, and you are _not_ writing your own paper
for Sofia in June, then I strongly recommend you "comply in advance": Start rewriting your codebase to
use P2786 trivial relocation, as I asked you to experiment with [last April](/blog/2024/04/10/p1144-your-codebase/).
See how it goes. Get that implementation experience that we currently have only for P1144.

Moving on to some other papers I'm involved with...

## P2952 `auto& operator=(X&&) = default`

[P2952R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2952r2.html) (Taylor & O'Dwyer 2023)
was successfully forwarded by EWG, so you should probably expect that it'll be in C++26.
Thanks to my coauthor Matthew Taylor (whose idea it was in the first place) for keeping the
helm steady on this one!

This idea here is that we want to be able to say not just:

    SmallFunction& operator=(SmallFunction&&) { ~~~ }
    SmallFunction& operator=(SmallFunction&&) = default;
    auto& operator=(SmallFunction&&) { ~~~ }

but also:

    auto& operator=(SmallFunction&&) = default;

There is a potentially confusing corner case with the return type of:

    auto&& operator=(this Contrived&& self, Contrived&& rhs)
      = default;                         // deduces Contrived&
    auto&& operator=(this Contrived&& self, Contrived&& rhs)
      { self.m_ = rhs.m_; return self; } // deduces Contrived&&

(Because `self` here is [eligible for implicit move](https://eel.is/c++draft/expr.prim.id.unqual#4).)
I propose in [P2953](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2953r1.html)
that we should just forbid defaulting an assignment operator with such a contrived signature.
P2953's proposed wording uses more red than green. I like that kind of paper.

## P3160 Allocator-awareness for `inplace_vector`

LEWG rejected [P3160R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3160r2.html) (Halpern & O'Dwyer 2024).
I expect it to come back as a national body comment, since I cannot imagine us shipping a standard
container without the ability to hold allocator-aware elements. That is, `inplace_vector::insert` needs to call
the element type's allocator-extended constructor if it exists; but without knowledge of the allocator, it can't.
See ["Boost.Interprocess and `sg14::inplace_vector`"](/blog/2024/08/23/boost-interprocess-tutorial/) (2024-08-23).

This rejection was a surprise to me, because my impression in St Louis had been that we had all agreed to forward
[P0843](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p0843r14.html)'s non-allocator-aware container
_only because_ we knew allocator-awareness was coming right behind — all we lacked
was the wording, and we collegially "didn't want to hold up" P0843 on that account. Coming back in Hagenberg
with wording, only to find out it had been a bait-and-switch, was a rude (if perfectly legal) awakening.

## P3016 `initializer_list.data()`

LEWG had forwarded P3016R4 a while back, but LWG saw that `std::begin(il)` was losing its guarantee of
noexceptness and asked LEWG for a fix. At first, LEWG just wanted to add conditional noexcept to
`begin`, `end`, `data`, and `empty`, thus increasing the library's inconsistency. I smartly
(contrast my aforementioned naïveté in St Louis) brought a quick rider to add conditional noexcept to
_all ten_ [iterator.range] free functions, which LEWG approved, and with those additions forwarded
[P3016R6](https://quuxplusone.github.io/draft/d3016-valarray.html) back to LWG for another try.

You should expect that in C++26 you'll be able to say not only ([Godbolt](https://godbolt.org/z/9dWYWcWzW)):

    extern "C" void api(const int*, size_t);
    void f(std::initializer_list<int> il) {
      api(il.data(), il.size()); // well-formed tomorrow
    }

but also ([Godbolt](https://godbolt.org/z/Exv4oz4nd)):

    std::vector<int> v;
    static_assert(noexcept(v.begin())); // portable today
    static_assert(noexcept(begin(v))); // portable tomorrow

## P2927 `try_cast`

[P2927 "Inspecting `exception_ptr`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2927r2.html)
was voted in last time, with the new name `std::exception_ptr_cast`. In Hagenberg LEWG further voted
to give it a `=delete`'d overload for rvalues, so that C++26 will forbid you to write:

    struct Future {
      bool has_value() const;
      int value() const;
      std::exception_ptr error() const;
    };

    void lippincott(const std::exception *ex);
    Future f = ~~~;
    if (!f.has_value()) {
      lippincott(
        std::exception_ptr_cast<std::exception>(f.error())
          // error: f.error() is an rvalue
      );
    }

You'll have to write:

    if (!f.has_value()) {
      auto eptr = f.error();
      lippincott(
        std::exception_ptr_cast<std::exception>(eptr)
          // OK: eptr is an lvalue
      );
    }

See ["Value category is not lifetime"](/blog/2019/03/11/value-category-is-not-lifetime/) (2019-03-11)
and ["`try_cast` and `(const&&)=delete`"](/blog/2024/07/03/dont-delete-const-refref/) (2024-07-03).

Now, this likely isn't a big deal. `std::exception_ptr_cast`, like `std::from_chars`, has slowly morphed
from a convenience requested by end-users into a scary and cumbersome camel you'll probably hide under
a layer of indirection, e.g.

    namespace my {
      template<class T>
      const T *try_cast(const std::exception_ptr& eptr) {
        return std::exception_ptr_cast<T>(eptr);
          // OK: eptr is an lvalue, regardless of my caller
      }
    } // namespace my

So everyone's happy. Right?!

## Other papers I'm aware of

The big plenary news was that [P2900 Contracts](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p2900r13.pdf)
made the train — although it met some opposition, and my understanding is that support
for virtual functions was left on the platform like a stray briefcase.
But one can spin that omission as a positive: It will simply encourage
more disciplined use of the Non-Virtual Interface Idiom. A non-virtual public interface gives
you a single entry point on which to hang instrumentation, breakpoints, preconditions, postconditions,
and [Template Method](https://en.wikipedia.org/wiki/Template_method_pattern) code.
P2900 contracts certainly belong to that category, too. Thus, this non-NVI approach is made
actually ill-formed:

    struct Animal {
      virtual void speak(const char *prefix)
        pre(prefix != nullptr) = 0; // error, contract on a virtual function
      virtual ~Animal() = default;
    };

while this NVI approach, with the contract placed squarely on the interface where it belongs,
remains well-formed:

    struct Animal {
      void speak(const char *prefix)
        pre(prefix != nullptr) // OK
      {
        do_speak(prefix);
      }
      virtual ~Animal() = default;
    private:
      virtual do_speak(const char *) = 0;
    };

Also voted into the draft: [P3471 "Standard Library Hardening"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3471r2.html)
(Varlamov & Dionne 2024). This is the first "user" of Contracts in the standard library.
It replaces some _Preconditions_ elements in the paper standard with _Hardened Preconditions_,
which are just like preconditions except that now we'll require STL vendors to check them with contracts.
(Actually this checking is required only for "hardened implementations." GCC 14 supports an `-fhardened`
command-line switch; as of this writing Clang does not, but [it's coming](https://github.com/llvm/llvm-project/pull/78763).)

My impression is that P3471's main contribution (besides "locking in" P2900 Contracts)
is a laundry list, gleaned from libc++'s deployment experience, as to which preconditions
we consider to be hardenable versus not. That list is legitimately impressive, but I'd say it belongs
in vendor documentation or perhaps a Standing Document, rather than in the
paper standard where every minor addition or subtraction must go through the whole WG21 process.
We [just recently succeeded](https://isocpp.org/std/standing-documents/sd-9-library-evolution-policies)
in eliminating from the paper standard the ad-hoc laundry list associated with `[[nodiscard]]`; I think
eventually _Hardened Preconditions_ will go the same way.

The final paragraph of [P3471's Motivation section](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3471r2.html#motivation)
says, "Leaving security of the library to be a pure vendor extension fails to _position_ ISO C++ as
providing a _credible_ solution for code bases with formal security requirements" (emphasis added).
I read that concern with positioning as "We need these words _in the actual ISO document_ because
'C++ safety' [is a meme](https://stackoverflow.blog/2024/12/30/in-rust-we-trust-white-house-office-urges-memory-safety/)
right now," but in a couple years it'll probably be time to move the words back out
to a more appropriate vessel for their long-term maintenance.

[P1967 `#embed`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1967r13.html) (Meneide 2019) was finally voted
into the draft at this meeting, after two years in CWG.
(See ["P1967 `#embed` and D2752 static storage for `initializer_list`"](/blog/2023/01/13/embed-and-initializer-lists/) (2023-01-13).)

[P0447 `std::hive`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p0447r28.html) (Bentley 2019), the
"deque with holes" data structure formerly known as `plf::colony`, was also voted in, over some opposition.
I'm interested to see how long it takes library vendors to either implement it or admit they're not going to.
It's trying to hit the sweet spot between "complicated like MSVC's `deque`" and "constrained to a
single Best Implementation Strategy like `unordered_map`" — a sweet spot _I'm_ not sure exists. But we'll see.
