---
layout: post
title: "Branchless sorting of trivially relocatable types"
date: 2026-06-05 00:01:00 +0000
tags:
  algorithms
  benchmarks
  llvm
  relocatability
  triviality
  value-semantics
---

A few days ago Christof Kaser posted a very impressive blog post on
["Fast Branchless Quicksort using Sorting-Networks"](https://tiki.li/blog/blqsort)
([chkas/blqsort](https://github.com/chkas/blqsort)). A "branchless" algorithm is
one designed to exploit modern processors' conditional-move instructions. So for
example the `blqs::sort2` primitive, which looks like this:

    template<class T, class Compare>
    void sort2(T& a, T& b, Compare comp) {
      T x = a;
      T y = b;
      bool m = comp(x, y);
      a = m ? x : y;
      b = m ? y : x;
    }

when instantiated for `int` compiles down to a couple of `cmov` instructions on x86-64
and a couple of `csel` instructions on ARM64. ([Godbolt.](https://godbolt.org/z/v34v5oor1))

But at the higher generic-programming level, count all the copy operations in `sort2`!
It copies `a` into `x`; then copy-assigns `x` back into `a`. If `T` were an expensive-to-copy
type like `std::string`, this would be slow code; and if `T` were `unique_ptr`, it wouldn't
compile at all. Therefore, `blqsort` enables its entire branchless "fast path" only for
types that are trivially copyable and roughly register-sized.

> As of this writing the gating condition is `std::is_trivially_copyable<T>::value && sizeof(T) <= 16`,
> but I've pointed out to Christof that his `heap_sort` also depends on `T` to be trivially
> default-constructible. It's also possible (if pathological) for `T` to be trivially copyable
> yet not copy-constructible or (more commonly) not copy-assignable. But this blog post isn't
> really about narrowing the gate; we're going to broaden it instead!

"Trivially copyable" is what I call a "holistic" trait: it means something about the behavior
of the entire type, rather than about just one special member function or just one kind of expression.
And specifically _what_ it means is that you can do any value-semantic operation — bringing new
copies into existence, poofing them out of existence, overwriting one's value with another's,
swapping or permuting or copying — as if the objects were just bags of bits. As long as you
never try to invent a _new_ value out of whole cloth, you can shift copies of your _given_ values
around as much as you like, even into completely new areas of memory, simply by memcpying them.
In the following diagram, each box represents a C++ _object_, and the color of the box represents
the object's _value_ (for example 42, or 3.14, or "hello," or Tuesday).

![](/blog/images/2026-06-05-holistic-tc.png)

We see that in the "After" picture, the green object has become blue. Was that by copy-assignment
from the original blue object? or move-assignment? or copying from the yellow, destroying, and
then copy-constructing from a blue object? With trivial copyability, we needn't say! Each of those
possible operation-sequences is guaranteed to be _physically_ tantamount to simply memcpying the
"blue" bytes into their final location.

"Trivially relocatable" (the widely deployed P1144 idiom, I mean, not the unusable version that
was briefly merged into the C++26 draft in 2025) is another "holistic" trait. Specifically
what _trivially relocatable_ means is that you can do any _affine_ value-semantic operation —
swapping, permuting, relocating from one place to another — as if the objects were just bags of
bits. As long as you preserve the _number of copies of each given value,_ you can shift that
particular set of values around as much as you like, even into completely new areas of memory,
simply by memcpying them. (But unlike two paragraphs ago, with _trivially relocatable_ you're
not allowed to turn one value into two, or poof a value completely out of existence: each and
every input value must be represented the same number of times in the final output.)

![](/blog/images/2026-06-05-holistic-tr.png)

As long as whatever highest-level algorithm we're doing preserves this "affine," one-to-one
property, every possible operation-sequence is guaranteed to be _physically_ tantamount to
simply memcpying the bytes around.

> The above images come from a ten-slide presentation on holistic traits I wrote in mid-2024.
> See the whole slide deck [here](/blog/code/2026-06-05-holistic-traits.pdf).

Algorithms that have this "affine" one-to-one property include `swap`, `rotate`, `partition`, and... `sort`!
Imagine rewriting `blqs::sort2` like this ([Godbolt](https://godbolt.org/z/czfaKWrcc)):

    template<class T, class Compare>
    void sort2(T& a, T& b, Compare comp) {
      union U { T t; U() {} ~U() {} };
      U x, y;
      std::relocate_at(&a, &x.t);
      std::relocate_at(&b, &y.t);
      bool m = comp(x.t, y.t);
      std::relocate_at(m ? &x.t : &y.t, &a);
      std::relocate_at(m ? &y.t : &x.t, &b);
    }

This is conceptually closer to what our `sort2` algorithm actually "needs" to do.
It doesn't really care that it's making copies of `T` objects; conceptually it's just
bringing the values "closer to hand" (which could be a relocate), comparing its close-up
copies, and then putting the values back "in memory" (which could be a relocate). For
trivially copyable `T`, it's totally fine to replace the first relocate with a copy-construct
and the second relocate with a copy-assign: the end result is guaranteed to be the same,
assuming it compiles at all. The relocation-based version merely extends that guarantee
from "trivially copyable `T`” to "trivially relocatable `T`."

But the above code both depends on P1144/P3516 `relocate_at` and is super ugly.
Imagine patching every helper in `blqsort.h` with this pedantry! We can actually
achieve the same effect much more easily by leaning on the holistic guarantee: We know
that the end result of `sort2` — in fact the end result of `blqsort` — will be an affine,
one-to-one, permutation of the input values. So we can actually run the whole algorithm
working entirely with object representations! The patch for this is only a few lines long:

      constexpr bool copy_is_cheap =
        std::is_trivially_copyable<T>::value && sizeof(T) <= 16;

    + constexpr bool relocate_is_cheap =
    +   std::is_trivially_relocatable<T>::value && sizeof(T) <= 16;

      if constexpr (copy_is_cheap) {
        blqsort(first, last - 1, comp);
    + } else if constexpr (relocate_is_cheap) {
    +   struct Rep {
    +     alignas(T) char data_[sizeof(T)];
    +   };
    +   blqsort((Rep*)first, (Rep*)(last - 1),
    +     [&](const Rep& a, const Rep& b) { return comp(*(const T*)&a, *(const T*)&b); });
      } else {
        block_qsort(first, last - 1, comp);
      }

This says, "If we know that `T` is trivially relocatable, then let's just pretend we're sorting
anonymous blocks of bytes instead. Make sure to treat them as `T` objects for the purposes
of `comp`, but don't worry about the value-semantic bookkeeping; I promise it will all come out
correctly in the end."

Christof's `blqs::sort` uses the branchless fast path, `blqsort`, only for trivially copyable
types; types that don't make it through that gate will use the slow path, `block_qsort`, instead.
This patch broadens the gate: now we can use the fast path for all trivially relocatable types,
including for example `unique_ptr` and `shared_ptr`. (But not `string`, because it fails the
`sizeof` check.)

## Benchmark results

This turns out to be a perfect testbed for the kinds of performance improvements you can get
from P1144-style trivial relocation on something as familiar as sorting a vector of `shared_ptr`.
I wrote a benchmark ([backup](/blog/code/2026-06-05-blqs-bench.cpp)) comparing the performance
of the high-level `blqs::sort` on four different kinds of `T`:

    struct TC {
      Int *p_;
      void *ctrl_;
      explicit TC() = default;
      explicit TC(Int& i) : p_(&i) {}
      friend auto operator<=>(const TC& a, const TC& b) { return *a.p_ <=> *b.p_; }
    };

    struct TR {
      std::shared_ptr<Int> p_;
      explicit TR() = default;
      explicit TR(Int& i) : p_(&i, [](Int*){}) {}
      friend auto operator<=>(const TR& a, const TR& b) { return *a.p_ <=> *b.p_; }
    };

    struct NTC : TC {
      using TC::TC;
      ~NTC() {}
    };

    struct NTR : TR {
      using TR::TR;
      NTR(NTR&&) = default;
      NTR(const NTR&) = default;
      NTR& operator=(NTR&&) = default;
      NTR& operator=(const NTR&) = default;
      ~NTR() {}
    };

`TR` is a Rule-of-Zero type that pays all the same costs as `shared_ptr`
for its value-semantic operations — yet it is trivially relocatable. Then `NTC` is
exactly the same as `TC` except that its no-op destructor makes it not-trivially-anything;
and `NTR` is exactly the same as `TR` (paying `shared_ptr` costs) except that its no-op
destructor makes it not-trivially-anything.

Sorting a vector of 50 million elements with Christof's (current) implementation of
`blqs::sort` out of the box shows `TC` taking the fast path and all three of
`TR`, `NTC`, `NTR` taking the slow path, as predicted. Here the “`std::sort`” column
refers to libc++'s current implementation.

| Type  | `std::sort` | `blqs::sort` | Savings |
|:------|------------:|-------------:|--------:|
| `TC`  |        5.9s |         3.5s |     41% |
| `TR`  |        6.5s |  <b>6.2s</b> | <b>4%</b> |
| `NTC` |        5.8s |         4.3s |     25% |
| `NTR` |        6.3s |         6.7s |     −6% |

Now we apply the patch above, to enable the fast path whenever `relocate_is_cheap`.
The numbers for `TC`, `NTC`, and `NTR` aren't expected to change; so you can see
roughly how noisy this microbenchmark is.

| Type  | `std::sort` | `blqs::sort` | Savings |
|:------|------------:|-------------:|--------:|
| `TC`  |        5.8s |         3.4s |     42% |
| `TR`  |        6.5s |  <b>4.0s</b> | <b>38%</b> |
| `NTC` |        5.8s |         4.4s |     24% |
| `NTR` |        6.2s |         6.3s |     −2% |

That is, by applying an eight-line patch to use the fast path for trivially relocatable
`shared_ptr`, we've turned `blqs::sort` from a 4% win into a 38% win, right in line
with its speedup for trivially copyable types.

## What gave us this speedup?

To get this speedup, we used one "dirty trick" and one compiler extension.
Our dirty trick was:

    struct Rep {
      alignas(T) char data_[sizeof(T)];
    };
    blqsort((Rep*)first, (Rep*)(last - 1),
      [&](const Rep& a, const Rep& b) { return comp(*(const T*)&a, *(const T*)&b); });

Those casts are `reinterpret_cast` in disguise. This is type-punning, and
technically it's undefined behavior. (It's certainly not constexpr-friendly.)
We can make it well-defined and constexpr-friendly by using the P1144/P3516
entrypoint `std::relocate_at` on `T` objects instead of copy-assignment on `Rep`
objects; but as we've seen, that's very ugly — and in fact very error-prone.
Changing a copy-assignment-based algorithm to use relocation takes a major
code audit to make sure you balanced out every construction and destruction, never
moved or relocated twice from the same object, never accessed an object outside
its lifetime, etc. Our "dirty trick" foolproofly accomplished the same codegen,
with just a little surgical patch at the top level. If we really needed constexpr-friendliness,
I'd add an `if consteval` at the top level sooner than I'd give up this trick.

But of course the thing we _must_ have, in order to apply this optimization,
is a reliable way to detect `is_trivially_relocatable<T>`. My code makes it look
easy, because I use a compiler (and standard library) with full P1144 support.
Sadly neither Clang nor GCC is such a compiler.

Mainline Clang actually has two
different builtins in this area — `__is_trivially_relocatable` and
`__builtin_is_cpp_trivially_relocatable` — but both of them have too many false
positives to use safely in production: the latter by design (it intended to
implement the failed P2786 proposal) and the former merely by neglect. An attempt
was made to fix up the former in [Clang #84621](https://github.com/llvm/llvm-project/pull/84621),
but the maintainers resisted, and the patch was eventually abandoned.
Obviously I'd like to see it revived.
