---
layout: post
title: "Trivial relocation, `std::swap`, and a $2000 prize"
date: 2023-02-24 00:01:00 +0000
tags:
  help-wanted
  relocatability
  value-semantics
  wg21
excerpt: |
  This is the first in a series of at least three weekly blog posts. Each post will explain
  one of the problems facing [P1144 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)
  and [P2786R0 "Trivial relocatability options"](https://isocpp.org/files/papers/P2786R0.pdf) as
  we try to (1) resolve their technical differences and (2) convince the rest of the C++ Committee
  that these resolutions are actually okay to ship.

  Today's topic is trivial swapping.
---

This is the first in a series of at least three weekly blog posts. Each post
([I](/blog/2023/02/24/trivial-swap-x-prize/),
[II](/blog/2023/03/03/relocate-algorithm-design/),
[III](/blog/2023/03/10/sharp-knife-dull-knife/),
[IV](/blog/2023/06/03/p1144-pmr-koans/))
will explain
one of the problems facing [P1144 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html)
and [P2786R0 "Trivial relocatability options"](https://isocpp.org/files/papers/P2786R0.pdf) as
we try to (1) resolve their technical differences and (2) convince the rest of the C++ Committee
that these resolutions are actually okay to ship.

Today's topic is trivial swapping.

## Is trivial swap beneficial?

Trivial swappability first showed up on this blog [in June 2018](/blog/2018/06/29/trivially-swappable/),
even before P1144 was a thing.
Back then I said that optimizing `std::swap` into three memmoves had "zero benefit as far as I can see." But soon
after that, I implemented a trivial `swap` in my libc++ fork, and saw that the benefits were actually
significant. For example, `std::rotate` is based on `swap`, so any improvement in `swap` is picked up for free
by `std::rotate` ([Godbolt](https://godbolt.org/z/cGaTeE313)).
And by `std::partition` ([Godbolt](https://godbolt.org/z/e9P5fec6f)).
And by `std::swap_ranges`, `std::array::swap`, and so on.

The notion that some types are "trivially swappable" was also picked up by Nathan Myers in
[P2187 "`std::swap_if`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p2187r3.pdf) (July 2020),
although that particular paper ended up not going anywhere.

Microsoft's STL goes out of its way to recognize trivially copyable types in `swap`, `array::swap`,
`swap_ranges`, etc., and thus collects bug reports from people asking why it doesn't use trivial swap
in more places and for more types:

* ["Automatically using the vectorized std::swap_ranges and std::reverse"](https://developercommunity.visualstudio.com/t/automatically-using-the-vectorized-stdswap-ranges/340605) (July 2018)
* ["std::swap of arrays [...] specialization for trivial types"](https://github.com/microsoft/STL/issues/2683) (April–May 2022)

So, trivial swap does seem like it would be nice to have.

## Does trivial relocatability imply trivial swappability?

The Standard's [[utility.swap]/3](https://eel.is/c++draft/utility.swap#3) defines the semantics of `std::swap` as follows:

> Exchanges values stored in two locations.

It doesn't say any more than that about _how_ the values are swapped. So a reasonable naïve implementation would be

    alignas(T) char temp[sizeof(T)];
    std::relocate_at(&a, (T*)temp);
    std::relocate_at(&b, &a);
    std::relocate_at((T*)temp, &b);

That is: "Relocate `a`'s value into temporary housing; then relocate `b` into `a`'s old home; finally,
relocate `a`'s value from the temporary back into `b`." Compare this to the
present-day implementation

    T temp = std::move(a);
    a = std::move(b);
    b = std::move(temp);
    // temp.~T() at end of scope

which was in turn _significantly_ more efficient than C++98's

    T temp = a;
    a = b;
    b = temp;
    // temp.~T() at end of scope

The change from C++98 to C++11's move-enabled implementation was kind of a big deal:
nothing pre-C++11 had ever given `std::swap` permission to "move-from" `a` like that. Similarly, nothing
pre-P1144 has ever given `std::swap` permission to "relocate-from" `a`. This would be something new.

Relocating-from is a particularly big deal because it involves _destroying the original object._
If we swap `a` and `b` as above, we'll end the lifetime of the original `a` object
and then start a new object's lifetime (also at `a`) on the next line.
If I took a pointer to `a` before the swap, is it "dangling" after the swap because the object
it was pointing at has been destroyed? There's still an object at that address, of course; but
now it's a "different" object. (C++'s formal wording doesn't handle this kind of thing very well.
The relevant wording is in [[basic.life]/8](https://eel.is/c++draft/basic.memobj#basic.life-8),
around the term of art _transparently replaceable_; it's currently the subject of
[multiple](https://wg21.cmeerw.net/cwg/issue2676) [CWG](https://wg21.cmeerw.net/cwg/issue2677) [issues](https://wg21.cmeerw.net/cwg/issue2545),
which should make these lifetime shenanigans perfectly legal once the wording issues are resolved.)

Now, _in practice_, we wouldn't bother using `std::relocate_at` inside `std::swap`, because for
non-trivially-relocatable types `std::relocate_at` likely costs more than move-assignment.
So all we need to do is distinguish trivially swappable types (which we'll swap "by implementation magic"
without ending their lifetimes) from non-trivially swappable types (which we'll swap the
traditional C++11 way).

> [P2786R0](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2786r0.pdf) section 9.6,
> top of page 17, makes the intriguing claim that "all trivially swappable types are trivially relocatable."
> The idea is that if `T` is trivially swappable, then you can always relocate a single `T`
> simply by pretending that you have already constructed a `T` at the destination address,
> trivially swapping the real `T` with the imaginary `T`, and then quitting the pretense that
> any `T` exists at the source address anymore.
>
> My impression is that "trivially swappable" and "trivially relocatable" are, indeed,
> exactly synonymous. But P2786's argument feels like sophistry: It's not obvious how one
> could implement "trivial relocation in terms of trivial swapping" in practice, and of course
> it wouldn't be efficient to do so. Contrariwise, "trivial swapping in terms of trivial relocation"
> is easily implemented in practice.

## Uh-oh: `std::swap` can't unconditionally `memcpy`

Let's assume we've solved all of the formal wording issues [above](https://wg21.cmeerw.net/cwg/issue2677).
We still face a practical problem: sometimes a trivially swappable type can't be trivially swapped!
I've previously covered this issue in ["When is a trivially copyable type not trivially copyable?"](/blog/2018/07/13/trivially-copyable-corner-cases/) (2018-07-13).

Consider this code:

    struct Lesser {
      explicit Lesser(int, int);
      short s;
      char c;
    };
    void swapit(Lesser& a, Lesser& b) {
      std::swap(a, b);
    }

You might think that `swapit` could be implemented as three calls to `memmove`:
from `&a` to the stack, from `&b` to `&a`, and from the stack back to `&b`. So far
so good. But how many bytes should those `memmove`s copy? `sizeof(Lesser)`?
[Wrong!](https://godbolt.org/z/K3oq3fKKa)

    template<class Comp>
    struct Widget {
      [[no_unique_address]] Comp comp;
      char d;
    };

On the Itanium ABI used by GCC and Clang, `Widget<Lesser>::d` is stored in the tail-padding
byte of `Widget<Lesser>::comp`. This means that if you swap all 4 bytes of the `Lesser`
object, you're actually also swapping the byte stored in its tail padding.
So if you have implemented `Widget`'s member `swap` function in the obvious straightforward way...

    void swap(Widget& y) noexcept {
      using std::swap;
      swap(comp, y.comp);
      swap(d, y.d);
    }

...you'll find that it swaps the `d` values _twice!_ This is an unmitigated disaster.
We can't have this.

### But wait, doesn't Microsoft use `memcpy` for trivial swapping?

Yes, and they see this "disaster" in practice.
But, because Microsoft's ABI uses the tail-padding so much less often than
Itanium's ABI, the disaster is mitigated by happening only occasionally,
not constantly.

Here's a concrete example of MSVC getting it wrong ([Godbolt](https://godbolt.org/z/qrYz564aK)):

    struct __declspec(align(8)) TailPad {
      int a;
    };
    struct Evil : TailPad {
      int b;
    };

    int main() {
      Evil e1 = {1,2};
      Evil e2 = {3,4};
      TailPad& tp1 = e1;
      TailPad& tp2 = e2;

      std::swap(tp1, tp2);  // should swap 1 and 3, leave 2 and 4 alone
      assert(e1.b == 2); // fails at runtime
    }

MSVC's `std::swap` dispatches to the precompiled library routine `__std_swap_ranges_trivially_swappable_noalias`
with parameters that ask it to swap `sizeof(TailPad)` bytes — but that's too many,
and it ends up swapping `Evil::b` when we didn't mean to.

All three of libc++, libstdc++, and MSVC currently have similar misbehavior when copying trivially copyable
(yet tail-padded) objects. (This is now [libstdc++ bug #108846](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=108846).)

- `std::copy(&tp1, &tp1 + 1, &tp2)`
- `std::move(&tp1, &tp1 + 1, &tp2)`
- `std::copy_backward(&tp1, &tp1 + 1, &tp2 + 1)`
- `std::move_backward(&tp1, &tp1 + 1, &tp2 + 1)`
- `std::copy_n(&tp1, 1, &tp2)`
- `std::move_n(&tp1, 1, &tp2)`
- `std::rotate_copy`, `std::set_union`, etc., which dispatch to `std::copy`
- `std::swap_ranges(&tp1, &tp1 + 1, &tp2)` (MSVC only)

The simplest fix in these cases is to check the size of the input range. If it's a range of 1 element,
it might be a potentially overlapping subobject, and so we can't `memcpy`. If it's a contiguous range of
2 or more elements, then it's safe to `memcpy`: as far as I know, neither major ABI
ever stores things in the tail padding of an array element.

But `std::swap` itself doesn't know "the size of the input range." It only ever receives single
objects. So `swap` isn't as easy to fix as `copy`.

## If `std::swap` can't `memcpy`, then how _do_ we trivially swap?

[P1144R7](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p1144r7.html#overlap-concerns)
suggests five possible ways forward:

<b>#1.</b> Per [Dana Jansens](https://danakj.github.io/2023/01/15/trivially-relocatable.html),
bring the ABI notion of "data size" into standard C++. `Lesser`'s "data size" is less than its `sizeof`,
so it's simply _not_ trivially relocatable. (But it remains trivially copyable? This doesn't satisfy me,
for multiple reasons.)

<b>#2.</b> Bring it in behind the scenes: allow the compiler to consider any type non-trivially relocatable
for implementation-defined secret reasons. (For example, `Lesser` might be trivially relocatable on MSVC, because MSVC's ABI
promises never to store anything in its tail padding; but non-trivial on GCC, because GCC's ABI doesn't promise.)

<b>#3.</b> Bring it all the way into the core language, e.g. by adding `datasizeof(T)` alongside `sizeof(T)`.
`std::swap` then can use memcpy for trivially swappable types, as long as it swaps `datasizeof(T)` bytes
and not `sizeof(T)` bytes.

<b>#4.</b> Completely disallow the public generic algorithm `std::swap(T&, T&)` from using trivial relocation.
Behind the scenes, vendors can always create a private entrypoint `__swap_complete_objects(T&, T&)` that does
the exact same thing as `swap` but with a precondition that the objects are complete.
`std::rotate`, `std::partition`, and so on could be reimplemented in terms of `__swap_complete_objects`.
This solution is unsatisfying because it requires heroics from the vendor.
Remember, the benefit of trivial `swap` is that it speeds up `rotate` and `partition` _for free._ If the
vendor has to go reimplement `rotate` and `partition` in terms of some new private functionality, it's
not _free_ anymore.

<b>#5.</b> Bless what MSVC is doing today. Recharacterize that situation I called an "unmitigated disaster":
It's not a bug, it's a precondition. Does `std::swap` swap the wrong number of bytes when you use it on
overlapping subobjects? Don't do that, then!

[P1144R6](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1144r6.html) implicitly assumed solution #5;
but that was before we came up with examples like `Widget<Lesser>`, that seem pretty obviously not the
programmer's fault. I think we _have_ to make `Widget<Lesser>` work. Therefore I no longer think
solution #5 is acceptable.

Solution #4 is the only solution that is fully backward-compatible with C++23, so it's the most likely
path forward. However, those "heroics" to reimplement things in terms of `__swap_complete_objects` turn out
to be _extremely_ difficult. This week I gave it a quick try in libc++, and gave up in despair.

> The problem is all the layers of indirection: `rotate` is implemented in terms of `swap_ranges`,
> which is implemented in terms of `iter_swap`, which is a customization point implemented in terms of `swap`,
> which is a customization point. Only the very highest level knows whether it's safe to use `memmove`;
> only the very lowest level needs to know.

## "Implementation bounty" X-Prize

I found solution #4 above very difficult to implement; but maybe I'm missing something.
So I'm putting my money where my mouth is!

I hereby offer a $2000 (USD) cash prize to the first person who produces an efficient and conforming implementation
of `std::swap_ranges`, `std::rotate`, and `std::partition` — using `memmove`-based trivial swapping where possible,
but falling back to a conforming `std::swap` implementation when the objects involved might be
potentially overlapping subobjects.

UPDATE, 2023-03-04: My libc++ fork now passes all eight cases in the test suite! So the prize for
patches to libc++, specifically, is claimed. But as of this writing the prize remains claimable for
patches to libstdc++ or Microsoft STL. See ["Update on my trivial `swap` prize offer"](/blog/2023/03/04/trivial-swap-prize-update/) (2023-03-04).

[Here](https://godbolt.org/z/aa3aeqd3q) ([backup](/blog/code/2023-02-24-prize-test-suite.cpp))
is the test suite that the winning implementation will pass. It consists of seven mandatory test cases:

- `must_use_memmove_1`: `std::rotate` on iterators of type `TR*`
- `must_use_memmove_2`: `std::ranges::rotate` on iterators of type `TR*`
- `must_use_memmove_3`: `std::partition` on iterators of type `TR*`
- `must_use_memmove_4`: `std::ranges::partition` on iterators of type `TR*`
- `must_use_memmove_5`: `std::swap_ranges` on iterators of type `TR*`
- `must_not_fail_6`: `std::swap` on two potentially overlapping subobjects of type `TR`
- `must_not_fail_7`: `std::rotate` on a range of potentially overlapping subobjects of type `TR`

And one bonus test case that would be nice-to-have, but you'll win the prize even without it:

- `should_use_memmove_8`: `std::rotate` on iterators of type `reverse_iterator<TR*>`

"Passing" tests 1–5 and 8 is defined as "generating code with no calls to `operator delete`,"
i.e., generating zero calls to `TR::operator=(TR&&)`. "Passing" tests 6 and 7 simply means not
failing the assertions. And of course your patched library should remain conforming — for example,
it should pass its own upstream test suite.

As of 2023-02-22, [Clang trunk](https://godbolt.org/z/K8hqqjvhs) passes only tests 6 and 7;
my libc++ fork passes only tests 1, 3, 5, and 8.
(UPDATE: As of 2023-03-04, [my libc++ fork passes all eight tests.](https://godbolt.org/z/8x89ahxcP))

The easiest solutions for me to judge will be patches against libc++ (either against [trunk](https://github.com/llvm/llvm-project)
or against [my P1144 fork](https://github.com/Quuxplusone/llvm-project/tree/trivially-relocatable)),
because I can build them locally. Patches against [libstdc++](https://github.com/gcc-mirror/gcc/)
or [Microsoft STL](https://github.com/microsoft/STL/) are also acceptable, but will be harder for
me to evaluate without help. By the way, if you patch GCC to natively recognize trivially relocatable types,
I'd be thrilled to assist you in getting your patched version its own place in the dropdown on
Godbolt Compiler Explorer!

If you have a solution that passes all seven test cases, you can email me, or find me on
[the cpplang Slack](https://cppalliance.org/slack/).
