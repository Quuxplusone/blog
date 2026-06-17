---
layout: post
title: "Who uses P2786 and P1144 for trivial relocation?"
date: 2024-06-15 00:01:00 +0000
tags:
  proposal
  relocatability
  st-louis-2024
  triviality
---

On Friday, June 28th, there will be a discussion at the C++ Committee meeting
about the state of "trivial relocation" in C++.
(The meeting is physically in St Louis, but also streamed remotely via Zoom and open to
<i>visiting experts</i> per WG21's [Meetings and Participation Policy](https://isocpp.org/std/meetings-and-participation/).)
If you're an expert (read: a library maintainer or developer) with opinions or experience
on "trivial relocation" — especially if you have implemented it in your own codebase — I
encourage you to read the participation policy and try (virtually) to show up for the discussion!

There are two active proposals for trivial relocation in C++: my own P1144
(based on pre-existing practice in Folly, BSL, and Qt)
and Bloomberg's P2786 (a reaction to P1144). The latter — unfortunately in my view —
has raced past P1144 in the committee and shows a real danger of being included in C++26.

> Why "danger"? Because P2786 is not fit for any present-day library's needs.
> See [section 2](#what-present-day-libraries-use-p2786-andor-p1144).
>
> And how "raced"? See [section 3](#history-of-p1144-and-p2786-in-wg21).

For a good (non-Arthur-written) summary of "what is trivial relocation anyway," see Giuseppe D'Angelo's
ongoing series of blog posts for KDAB, collectively titled "Qt and Trivial Relocation":

* [Part 1: "What is relocation?"](https://www.kdab.com/qt-and-trivial-relocation-part-1/)
* [Part 2: "Relocation and erasure"](https://www.kdab.com/qt-and-trivial-relocation-part-2/)
* [Part 3: "Trivial relocability for vector erasure, and types with write-through reference semantics"](https://www.kdab.com/qt-and-trivial-relocation-part-3/)
* [Part 4: "On trivial relocation and move assignments"](https://www.kdab.com/qt-and-trivial-relocation-part-4/)
* [Part 5: "Trivial Relocation and Standard C++"](https://www.kdab.com/qt-and-trivial-relocation-part-5/)

## What present-day libraries use P2786 and/or P1144

There are several aspects to "trivial relocation" as a proposed C++ feature:

* The simple user-facing type-trait `is_trivially_relocatable_v<T>`. P2786 and P1144 both propose this,
    but with significantly different meanings. P2786 treats "relocatable" as a verb analogous to "move-constructible,"
    whereas P1144 treats "trivially relocatable" as a holistic property analogous to "trivially copyable."
    A P2786-friendly codebase would say that `tuple<int&>` is trivially relocatable (because it's trivially
    move-constructible and trivially destructible), whereas a P1144-friendly codebase would say that `tuple<int&>` is
    not trivially relocatable (because it permits a sequence of value-semantic operations preserving
    the number of live objects of the type, which cannot be emulated by operating solely on the participating
    objects' bit-patterns).

* New library algorithms. P1144 proposes the gamut you'd expect by treating the verb `relocate` analogously to `construct`:
    `uninitialized_relocate`, `uninitialized_relocate_n`, `uninitialized_relocate_backward`, `relocate_at`,
    and a prvalue-producing [`relocate`](/blog/2022/05/18/std-relocate/).
    P2786 proposes only `std::trivially_relocate`. These, again, are significantly different approaches.

* Invisible-to-the-user library optimizations that are compatible with either P1144 or P2786; e.g. vector reallocation,
    type-erased `function` move-construction and move-assignment, `inplace_vector` move-construction.

* Invisible-to-the-user library optimizations that are compatible *only* with P1144; e.g. vector insert and erase,
    swap, rotate, `inplace_vector` move-assignment.

* "Warrant" syntax. P1144 proposes a `[[trivially_relocatable]]` attribute; P2786 proposes a contextual keyword
    `trivially_relocatable`. Most libraries can be expected not to use these, because their effect would be
    limited to compilers that support the attribute (in the former case) or to compilers speculatively supporting
    a merely proposed C++26 keyword (in the latter case).

For any codebase that concerns itself at all with "trivial relocation," we can look at how it deals with
each of these four dimensions: Does it define a type-trait matching P1144's holistic semantics or P2786's single-operation semantics (or neither)?
Does it provide some or all of P1144's proposed library algorithms, or P2786's single algorithm (or neither)?
Does it implement optimizations compatible only with P1144, optimizations compatible with both proposals, or
no optimizations at all?

Further, it's only fair to look at four other relevant properties of a library:

* Has it ever taken relevant input from Arthur O'Dwyer; from Mungo and Alisdair; both; or neither?
    (And if so: Was the input in the form of code-review feedback, or actual code?)

* Does the library rely on P1144's proposed feature-test macros `__cpp_impl_trivially_relocatable` and/or `__cpp_lib_trivially_relocatable`,
    i.e., can you compile it with Arthur's fork of Clang/libc++ and get the optimizations today?
    Alternatively, does it rely on P2786's proposed feature-test macro `__cpp_trivial_relocatability`?

* Did any of the library's maintainers sign [P3236R1 "Please reject P2786 and adopt P1144"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3236r1.html)
    (May 2024)?

* Did any of the library's maintainers comment (pro or con) on [Clang #84621](https://github.com/llvm/llvm-project/pull/84621),
    which asks Clang to implement P1144 semantics for `__is_trivially_relocatable(T)`?


I'll try to update this list of libraries as I find out about new ones.


### [Abseil](https://github.com/abseil/abseil-cpp/)

Google Abseil implements [`absl::is_trivially_relocatable`](https://github.com/abseil/abseil-cpp/blob/f04e489056d9be93072bb633d9818b1e2add6316/absl/meta/type_traits.h#L461-L525)
with P1144 semantics: "its object representation doesn't depend on its address,
and also none of its special member functions do anything strange."
Abseil goes out of its way to avoid using Clang's `__is_trivially_relocatable` builtin,
because of [Clang #69394](https://github.com/llvm/llvm-project/issues/69394), but also because:

    // Clang on all platforms fails to detect that a type with a user-provided
    // move-assignment operator is not trivially relocatable. So in fact we
    // opt out of Clang altogether, for now.
    //
    // TODO(b/325479096): Remove the opt-out once Clang's behavior is fixed.

Abseil does use the builtin if P1144's feature-test macro `__cpp_impl_trivially_relocatable` is defined.
This means that we can use Godbolt to see Abseil getting better codegen on a compiler that supports P1144:
[this Godbolt](https://godbolt.org/z/4orv6ETM3) shows `inlined_vector::erase` generating a loop over the
assignment operator with Clang trunk, but a straight-line `memmmove` with P1144 Clang.

Abseil's Swiss tables (e.g. `absl::flat_hash_set`) optimize rehashing (compatible with both P2786 and P1144);
[this Godbolt](https://godbolt.org/z/6Wc57EY86) shows the benefit.
`absl::InlinedVector` optimizes `erase` and `swap` (compatible only with P1144), but not `insert` or `reserve`.
The `erase` and `swap` optimizations were implemented by Arthur O'Dwyer ([#1625](https://github.com/abseil/abseil-cpp/pull/1625),
[#1618](https://github.com/abseil/abseil-cpp/pull/1618), [#1632](https://github.com/abseil/abseil-cpp/pull/1632)).

One Abseil maintainer [thumbs-upped](https://api.github.com/repos/llvm/llvm-project/issues/84621/reactions) Clang #84621,
but did not leave a comment.


### [AMC](https://github.com/AmadeusITGroup/amc/)

Amadeus AMC implements `amc::is_trivially_relocatable` with P1144 semantics.
It implements the P1144 library algorithms `uninitialized_relocate_n` and `relocate_at` (but not the other three,
and not P2786's `trivially_relocate`).

`amc::SmallVector` and `amc::Vector` both optimize `reserve` (compatible with both P2786 and P1144),
as well as `swap` (delegating some of the work to `std::swap_ranges` so as to remain compatible with both P2786 and P1144).
They both optimize `insert` and `erase` (compatible only with P1144).

AMC's sole contributor, Stéphane Janel, signed P3236R1. He also commented
[in favor](https://github.com/llvm/llvm-project/pull/84621#discussion_r1554201161) on Clang #84621.


### [binutils-gdb](https://gnu.googlesource.com/binutils-gdb)

GNU binutils-gdb uses the adjective [`IsRelocatable`](https://gnu.googlesource.com/binutils-gdb/+/refs/heads/master/gdbsupport/poison.h#59),
but uses it specifically to mean the same thing as `is_trivially_copyable`, and only in order to `=delete`
memcpy and memmove for non-trivially-copyable types. I don't know why they didn't name their trait `IsTriviallyCopyable`.
binutils-gdb isn't evidence for any particular semantics.


### [Blender](https://projects.blender.org/blender/)

Blender implements P1144's [`uninitialized_relocate_n`](https://projects.blender.org/blender/blender/src/commit/6c9a0e8c15aa7f2b0c8a96a9c8adab1bb87666b3/source/blender/blenlib/BLI_memory_utils.hh#L82-L86),
but without any optimization for, or attempt to identify, trivially relocatable types.

Blender has never taken commits or code review from Arthur, Mungo, or Alisdair.

Two Blender maintainers signed P3236R1.


### [BSL](https://github.com/bloomberg/bde)

Bloomberg BSL's implementation is very old — older than any standards proposal in this area.

BSL implements `bslmf::IsBitwiseMoveable` with P1144 semantics, i.e. as a synonym for the holistic property
[`is_trivially_copyable`](https://github.com/bloomberg/bde/blob/0098f67d4bf58197b82dff79be636fcb3780791a/groups/bsl/bslmf/bslmf_isbitwisemoveable.h#L617-L665),
albeit with a (non-P1144-compatible, non-P2786-compatible) special case that assumes all one-byte types must be
empty and therefore trivial. Notably, BSL does *not* set `IsBitwiseMoveable` for trivially move-constructible, trivially destructible
types (such as `tuple<int&>`).

BSL provides the generic algorithms `bslma::ConstructionUtil::destructiveMove(destp, alloc, srcp)` (analogous but certainly
not identical to P1144's generic `relocate_at(srcp, destp)`) and `ArrayPrimitives::destructiveMove(d_first, first, last, alloc)`
(ditto, P1144's generic `uninitialized_relocate(first, last, d_first)`).

`bsl::vector` optimizes [`reserve`](https://github.com/bloomberg/bde/blob/0098f67d4bf58197b82dff79be636fcb3780791a/groups/bsl/bslstl/bslstl_vector.h#L3607-L3631)
(compatible with both P2786 and P1144) by delegating to the generic `ArrayPrimitives::destructiveMove`.
It optimizes [`insert`](https://github.com/bloomberg/bde/blob/0098f67d4bf58197b82dff79be636fcb3780791a/groups/bsl/bslalg/bslalg_arrayprimitives.h#L3965-L4053)
and `erase` (compatible only with P1144) likewise.

`bsl::deque` optimizes `insert` and `erase` (compatible only with P1144).

`bsl::vector::insert(pos, first, last)`, when `first` is an input iterator (so that `last - first` cannot be computed), will
append elements to the end of the vector and then delegate to the generic `ArrayPrimitives::rotate`, which optimizes
(compatible only with P1144). BSL does not provide its own implementations of `bsl::rotate`, `bsl::swap_ranges`, etc.; those
algorithms are just `using`'ed from namespace `std`.

BSL has taken commits from Alisdair Meredith and Mungo Gill, but if any of them were in this particular area,
it's not immediately obvious. It's never taken commits from Arthur O'Dwyer.


### [`fast_io`](https://github.com/cppfastio/fast_io)

The `cppfastio/fast_io` library implements
[`fast_io::freestanding::is_trivially_relocatable`](https://github.com/cppfastio/fast_io/blob/0ef858811e97a41007ae33ca757cf44f7c41ab20/include/fast_io_core_impl/freestanding/relocatable.h#L6-L13)
with P1144 semantics.

In eleven places, it tests `__has_cpp_attribute(clang::trivially_relocatable)` and uses the attribute to warrant types
as trivially relocatable if the attribute is supported. This attribute is supported only in Arthur's P1144 reference implementation.
It appears to me that none of these eleven instances strongly depend on P1144's "sharp-knife" semantics; I think they are equally
compatible with P2786's "dull-knife" semantics.

`fast_io::containers::vector<T>` optimizes reallocation (compatible with both P2786 and P1144) and
[`erase`](https://github.com/cppfastio/fast_io/blob/master/include/fast_io_dsal/impl/vector.h#L970-L990) (compatible only with P1144).
However, as of this writing, `erase` is certainly buggy for most `T`, regardless of the relocation optimization;
so this library is fundamentally *not an example* of usage experience.

`fast_io` has taken code review from Arthur O'Dwyer.


### [Folly](https://github.com/facebook/folly)

Facebook Folly's implementation is very old — older than any standards proposal in this area.

Folly implements `folly::IsRelocatable` with P1144 semantics. As of [#2216](https://github.com/facebook/folly/pull/2216) (June 2024),
Folly will use P1144's `std::is_trivially_relocatable<T>` when the feature-test macro
`__cpp_lib_trivially_relocatable` is set. (This code was contributed by Arthur O'Dwyer.)

It does not implement either proposal's library algorithms.

`folly::fbvector` optimizes `reserve` (compatible with both P2786 and P1144) and also `insert` and `erase` (compatible only with P1144).

`folly::small_vector` optimizes move-construction (compatible with both P2786 and P1144; contributed in [#1934](https://github.com/facebook/folly/pull/1934)
by Arthur O'Dwyer) and move-assignment (compatible only with P1144; contributed
[separately](https://github.com/facebook/folly/commit/2459e172518a7b5d55292b0d8b741ff14def87ba) by Giuseppe Ottaviano).
`small_vector` does not yet optimize `insert` or `erase`.

Folly maintainer Giuseppe Ottaviano signed P3236R1.


### [HPX](https://github.com/STEllAR-GROUP/hpx)

Stellar HPX [asked](https://github.com/STEllAR-GROUP/hpx/wiki/Google-Summer-of-Code-(GSoC)-2023#implement-the-relocate-algorithms)
for an implementation of P1144 and/or P2786 as part of Google Summer of Code 2023. P1144 was implemented; Arthur was asked to code-review,
and did so. The feature was shipped in [HPX 1.10](https://hpx-docs.stellar-group.org/latest/html/releases/whats_new_1_10_0.html) (May 2024).
IIUC, it's all in the namespace `hpx::experimental` — although a lot of the documentation currently implies it's in namespace `hpx`?

HPX 1.10 implements `is_trivially_relocatable_v` with P1144 semantics.
It implements the P1144 library algorithms `uninitialized_relocate{,_n,_backward}` and `relocate_at`; it
[comments on, but doesn't implement](https://github.com/STEllAR-GROUP/hpx/blob/d08354defb0a5c37bf5ad5da77191f10db9c3067/libs/core/type_support/include/hpx/type_support/relocate_at.hpp#L96-L121)
P1144 `relocate`; and it doesn't implement P2786 `trivially_relocate`.

It does not implement any library optimizations.

It does not use anyone's feature-test macros; it uses a config macro `HPX_HAVE_P1144_RELOCATE_AT` instead. If that macro
is set, it will fall back to P1144's `std::is_trivially_relocatable`, `std::uninitialized_relocate`, etc.

Three HPX contributors signed P3236R1. One (the primary implementor of HPX's P1144 implementation) commented
[in favor](https://github.com/llvm/llvm-project/pull/84621#issuecomment-2010442722) on Clang #84621.


### [Iros](https://github.com/ColeTrammer/iros)

Cole Trammer's Iros defines [`di::concepts::TriviallyRelocatable<T>`](https://github.com/ColeTrammer/iros/blob/21801c1cb2275320f4bd8a632197b19d7bd55773/libs/di/include/di/meta/trivial.h#L45-L61)
as a synonym for `is_trivially_copyable`.

The trait is used in [`erasure_cast`](https://github.com/ColeTrammer/iros/blob/21801c1cb2275320f4bd8a632197b19d7bd55773/libs/di/include/di/vocab/error/erasure_cast.h#L8-L11)
in a way that indicates the important part for Iros is `is_trivially_copy_constructible`, i.e. nothing to do with relocation.
This is incompatible with both P1144 and P2786.

Iros provides [`uninitialized_relocate(first, last, dfirst, dlast)`](https://github.com/ColeTrammer/iros/blob/21801c1cb2275320f4bd8a632197b19d7bd55773/libs/di/include/di/container/algorithm/uninitialized_relocate.h#L16-L41)
and [`uninitialized_relocate_backwards(first, last, dfirst, dlast)`](https://github.com/ColeTrammer/iros/blob/21801c1cb2275320f4bd8a632197b19d7bd55773/libs/di/include/di/container/algorithm/uninitialized_relocate_backwards.h)
in the `di::container` namespace. These take more arguments than the P1144 versions, and are not optimized.


### [libc++](https://github.com/llvm/llvm-project/tree/main/libcxx)

libc++ introduced [`std::__libcpp_is_trivially_relocatable<T>`](https://github.com/llvm/llvm-project/blob/d6cc35f7f67575f2d3534ea385c2f36f48f49aea/libcxx/include/__type_traits/is_trivially_relocatable.h#L24-L38)
in [February 2024](https://github.com/llvm/llvm-project/commit/4e112e5c1c8511056030294af3264da35f95d93c).
Unusually for this survey, libc++ uses Clang's builtin `__is_trivially_relocatable`
even in the absence of any feature-test macro. This means that it suffers from [Clang #69394](https://github.com/llvm/llvm-project/issues/69394);
but it's also evidence that libc++ is satisfied for now to look only at "move construct + destroy" (compatible with P2786).
For example, `__libcpp_is_trivially_relocatable<tuple<int&>>` is true (compatible only with P2786).

libc++ optimizes `vector` reallocation (compatible with both P2786 and P1144), and nothing else.

libc++ doesn't implement any library API from either P1144 or P2786 (not even for internal use).

libc++ has taken commits and code review from Arthur, but not in this area.


### [libstdc++](https://github.com/gcc-mirror/gcc/tree/master/libstdc%2B%2B-v3)

libstdc++ introduced [`std::__is_bitwise_relocatable`](https://github.com/gcc-mirror/gcc/blob/471fb09260179fd59d98de9f3d54b29c17232fb6/libstdc%2B%2B-v3/include/bits/stl_uninitialized.h#L1079-L1083)
in October 2018; author Marc Glisse originally called it `std::__is_trivially_relocatable` but renamed it in February 2019
at Arthur's suggestion (thus keeping the name `__is_trivially_relocatable` available for the core-language builtin,
which was added to Clang by Devin Jeanpierre in [February 2022](https://github.com/llvm/llvm-project/commit/19aa2db023c0128913da223d4fb02c474541ee22).

`std::__is_bitwise_relocatable` is true for trivial types and for `deque<T>` specifically, because libstdc++'s `deque`
has a throwing move-constructor (therefore before 2018 it was getting the [vector pessimization](/blog/2022/08/26/vector-pessimization/)).

libstdc++ optimizes `vector` reallocation (compatible with both P2786 and P1144), and nothing else.

libstdc++ defines a generic algorithm [`__relocate_a(first, last, d_first, alloc)`](https://github.com/gcc-mirror/gcc/blob/471fb09260179fd59d98de9f3d54b29c17232fb6/libstdc%2B%2B-v3/include/bits/stl_uninitialized.h#L1135-L1148)
which is roughly analogous to P1144's `uninitialized_relocate(first, last, d_first)`.
It also defines a helper
[`__relocate_object_a(dest, src, alloc)`](https://github.com/gcc-mirror/gcc/blob/471fb09260179fd59d98de9f3d54b29c17232fb6/libstdc%2B%2B-v3/include/bits/stl_uninitialized.h#L1064-L1077)
but only for non-trivially-relocatable types (and notice that this parameter order is the opposite of P1144's `std::relocate_at`).

libstdc++ has never taken commits from Arthur, Mungo, or Alisdair.


### [OE-Lib](https://github.com/OleErikPeistorpet/OE-Lib)

Ole Erik Peistorpet's OE-Lib defines [`oel::is_trivially_relocatable`](https://github.com/OleErikPeistorpet/OE-Lib/blob/b6ae29dd7a1be33a83f419176645a0fde9d7cb9e/fwd.h#L76-L106)
as ["trivially move-constructible and trivially destructible"](https://github.com/OleErikPeistorpet/OE-Lib/blob/b6ae29dd7a1be33a83f419176645a0fde9d7cb9e/auxi/core_util.h#L70-L73)
(compatible with P2786 but not with P1144). Likewise, [`oel::is_trivially_relocatable<std::pmr::string>`](https://github.com/OleErikPeistorpet/OE-Lib/blob/b6ae29dd7a1be33a83f419176645a0fde9d7cb9e/optimize_ext/default.h#L89-L91)
is true (compatible with P2786 but not with P1144).

`oel::dynarray` (a `vector`-alike type) optimizes `insert` and `erase` (compatible with P1144 but not with P2786) — but with an
explicit [code comment](https://github.com/OleErikPeistorpet/OE-Lib/blob/b6ae29dd7a1be33a83f419176645a0fde9d7cb9e/dynarray.h#L46-L59)
that "the allocator model is not quite standard: `destroy` is never used, `construct` [...] is not called when relocating elements."
So while it's not legal for _`std::vector::erase`_ to destroy-and-reconstruct arbitrary (P2786-relocatable-but-not-P1144-relocatable) elements
instead using their non-trivial assignment operator, _`oel::dynarray::erase`_ might be perfectly within its contract
to destroy-and-reconstruct arbitrary elements.

`oel::dynarray` also optimizes the novel operation [`da.unordered_erase(it)`](https://github.com/OleErikPeistorpet/OE-Lib/blob/b6ae29dd7a1be33a83f419176645a0fde9d7cb9e/dynarray.h#L847-L866);
i.e., destroy `*it`, trivially relocate `da.back()` into the hole, and decrement `da.size()`. Again this wouldn't be legal for `std::vector`
under P2786, but might be expected behavior for `oel::dynarray`.


### [ParlayLib](https://github.com/cmuparlay/parlaylib)

Carnegie Mellon ParlayLib has supported relocation with P1144 semantics since [October 2020](https://github.com/cmuparlay/parlaylib/commit/b8d5f3aedac79cf7203defe986c457518cd6cb78);
originally the code was based on a draft of P1144R0. In November 2023 the project's primary maintainer, Daniel Liam Anderson,
rewrote the code to match P1144R10;
this update was code-reviewed by Arthur O'Dwyer ([#67](https://github.com/cmuparlay/parlaylib/pull/67), [#1](https://github.com/Quuxplusone/parlaylib/pull/1))
and merged in February 2024.

ParlayLib implements [`parlay::is_trivially_relocatable`](https://github.com/cmuparlay/parlaylib/blob/36459f4/include/parlay/type_traits.h#L250-L304)
per P1144. If the P1144 feature-test macro `__cpp_lib_trivially_relocatable` is defined, then it will fall back to
`std::is_trivially_relocatable`. Unusually for this survey, ParlayLib will use Clang's builtin `__is_trivially_relocatable`
even in the absence of any feature-test macro.

ParlayLib supports the P1144 library algorithms `uninitialized_relocate{,_n}` and `relocate_at`.
It doesn't implement P2786 `trivially_relocate`.

ParlayLib [uses](https://github.com/cmuparlay/parlaylib/blob/36459f4/include/parlay/type_traits.h#L309-L313)
the `[[trivially_relocatable]]` attribute if the feature-test macro `__cpp_impl_trivially_relocatable` is set.
This allows it [to warrant to the compiler](https://github.com/cmuparlay/parlaylib/blob/36459f42a84207330eae706c47e6fab712e6a149/include/parlay/sequence.h#L69-L70)
that `parlay::sequence<T>` (a kind of multithreading-aware `vector`) is trivially relocatable.

ParlayLib supports several parallel `sort_copy`-style algorithms, each of which takes a template policy parameter controlling
the way in which data is copied from the input range to the output range. One such policy is `uninitialized_relocate_tag`,
which *relocates* elements from the input range to the output range. This is used to implement an
[in-place `sort`](https://github.com/cmuparlay/parlaylib/blob/36459f42a84207330eae706c47e6fab712e6a149/include/parlay/internal/counting_sort.h#L318-L325) as:

    auto Tmp = uninitialized_sequence<value_type>(In.size());
    auto a = count_sort<uninitialized_relocate_tag>(In, make_slice(Tmp), make_slice(Keys), num_buckets);
    parlay::uninitialized_relocate(Tmp.begin(), Tmp.end(), In.begin());

ParlayLib's primary maintainer signed P3236R1, and commented [in favor](https://github.com/llvm/llvm-project/pull/84621#issuecomment-2004173918)
on Clang #84621.


## [PocketPy](https://github.com/pocketpy/pocketpy)

PocketPy in [#208](https://github.com/pocketpy/pocketpy/pull/208) (2024) implemented a `small_vector` that uses relocation in
its move-constructor.

PocketPy implements [`pkpy::is_trivially_relocatable_v`](https://github.com/pocketpy/pocketpy/blob/0406dfab787b86cbd63f486209902965c78d78ef/include/pocketpy/vector.h#L182-L201)
with P1144 semantics, essentially as an implementation detail of `pkpy::small_vector`. This was introduced in
[#208](https://github.com/pocketpy/pocketpy/pull/208) (February 2024).

`pkpy::small_vector::reserve` [uses `realloc`](https://github.com/pocketpy/pocketpy/blob/0406dfab787b86cbd63f486209902965c78d78ef/include/pocketpy/vector.h#L353-L363)
for trivially relocatable value types (compatible with both P2786 and P1144). `pkpy::small_vector` doesn't
support `insert`, `erase`, or `swap` at all.

PocketPy has never taken commits or code review from Arthur, Mungo, or Alisdair.


### [Qt](https://github.com/qt/qtbase)

Qt's implementation is very old — older than any standards proposal in this area.

It implements `Q_IS_RELOCATABLE` (née `Q_IS_MOVABLE`) according to the P1144 definition,
and has implemented the P1144-alike library algorithm `q_uninitialized_relocate` since [June 2020](https://github.com/qt/qtbase/commit/ffb73175e6c5b35e6367c88479cc0bf160482016).

Like BSL, Qt lacks a public `rotate` algorithm, but internally it uses a
[`q_rotate`](https://github.com/qt/qtbase/blob/ad922bbac1d65ff044c63160c73324fa27f44793/src/corelib/tools/qcontainertools_impl.h#L91-L109)
optimized for trivially relocatable types (compatible only with P1144).

`QVector` and `QList` optimize `insert` and [`erase`](https://github.com/qt/qtbase/blob/ad922bbac1d65ff044c63160c73324fa27f44793/src/corelib/tools/qarraydataops.h#L183-L202)
(both compatible only with P1144, not P2786). Qt's incompatibility with P2786 is one of the main takeaways of the blog series
"Qt and Trivial Relocatability" linked [above](#for-a-good-non-arthur-written-su).

Qt has never taken commits or code-review from Arthur, Mungo, or Alisdair.

Qt contributor Giuseppe D'Angelo authored [P3233R0 "Issues with P2786"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3233r0.html)
in April 2024.

Two Qt maintainers signed P3236R1. They commented [in favor](https://github.com/llvm/llvm-project/pull/84621#discussion_r1527150062)
and ["weak 'leans-to'"](https://github.com/llvm/llvm-project/pull/84621#discussion_r1522177263), respectively, on Clang #84621.


## [Skia](https://github.com/google/skia)

Google Skia defines [`sk_is_trivially_relocatable`](https://github.com/google/skia/blob/e20c8b0bac0caab082d6ed25022cb956147e98c0/include/private/base/SkTypeTraits.h#L20-L28)
with P1144 semantics (`is_trivially_copyable`), but doesn't seem to use it for any optimizations.


## [`small_vectors`](https://github.com/arturbac/small_vectors/)

Artur Bać's C++23 `small_vectors` library provides
[`small_vectors::is_relocatable_v`](https://github.com/arturbac/small_vectors/blob/846441b/include/small_vectors/detail/uninitialized_constexpr.h#L11-L15)
as a synonym for `is_trivially_copyable`; but what it actually uses is
[`small_vectors::concepts::relocatable`](https://github.com/arturbac/small_vectors/blob/846441b/include/small_vectors/concepts/concepts.h#L67-L75),
whose semantics match Niall Douglas's `[[move_relocates]]`. That is, it says nothing about what happens during a relocate,
but what it does say is that move-construction is tantamount to relocation: a moved-from object can be _destroyed_ trivially.
For example: `unique_ptr` is both move-relocates and trivially relocatable;
libc++'s dummy-node `list` is move-relocates but not trivially relocatable;
MSVC's sentinel-node `list` is trivially relocatable but not move-relocates.
In short, `small_vectors`' use of the term "relocatable" is incompatible with either P1144 or P2786.
See ["Trivially destructible after move"](/blog/2025/01/10/trivially-destructible-after-move/) (2025-01-10).

It implements (as an implementation detail) a P1144-alike `detail::uninitialized_relocate_n(first, n, dest)`
with no memcpy optimization. Besides that algorithm, it also provides three novel algorithms:

- `uninitialized_relocate_with_copy_n(first, n, dest)` which does copy-and-destroy instead of move-and-destroy

- `uninitialized_relocate_if_noexcept_n(first, n, dest)` which does move-and-destroy if move is nothrow, otherwise copy-and-destroy

- `uninitialized_uneven_range_swap(first1, n1, first2, n2)` which swaps the first `min(n1, n2)` elements and then
    relocates the rest from the end of the longer range to the end of the shorter range. This is the building
    block of a small-vector `swap`.

`small_vectors`' almost-sole contributor commented [in favor](https://github.com/llvm/llvm-project/pull/84621#issuecomment-2023964183)
on Clang #84621.


### [`stdx::error`](https://github.com/charles-salvia/std_error)

Charles Salvia's `stdx::error` library defines
[`stdx::is_trivially_relocatable`](https://github.com/charles-salvia/std_error/blob/25a2263152d4fe1b923c8daa568c9f61157f3939/all_in_one.hpp#L180-L196)
with P1144 semantics; if the P1144 feature-test macro `__cpp_lib_trivially_relocatable` is defined,
it will fall back to `std::is_trivially_relocatable`. Unusually for this survey, `stdx::error` will use Clang's builtin `__is_trivially_relocatable`
even in the absence of any feature-test macro.

`stdx::error` uses P1144's `[[trivially_relocatable]]` attribute to mark `error` as trivially relocatable,
if the feature-test macro `__cpp_impl_trivially_relocatable` is defined.

Its use of trivial relocation is compatible with both P2786 and P1144.

All of this code was contributed by Arthur O'Dwyer ([#1](https://github.com/charles-salvia/std_error/pull/1),
[#2](https://github.com/charles-salvia/std_error/pull/2)) during the November 2023 Kona meeting.
`stdx::error` has never taken commits or code review from Mungo or Alisdair.


### [Subspace](https://github.com/chromium/subspace)

Chromium Subspace defines [`concept sus::mem::TriviallyRelocatable`](https://github.com/chromium/subspace/blob/f9c481a241961a7be827d31fadb01badac6ee86a/sus/mem/relocate.h#L47-L129)
with P1144 semantics: the internal documentation talks about "non-trivial move *operations* and destructors,"
and the concept [tests](https://github.com/chromium/subspace/blob/f9c481a241961a7be827d31fadb01badac6ee86a/sus/mem/relocate.h#L58C19-L58C49) `is_trivially_move_assignable`.

Alone in this survey, Subspace [tests](https://github.com/chromium/subspace/blob/f9c481a241961a7be827d31fadb01badac6ee86a/sus/mem/relocate.h#L60C7-L60C45)
`__has_extension(trivially_relocatable)` when deciding whether to trust Clang's `__is_trivially_relocatable` builtin. This `__has_extension` flag
is defined in Arthur's P1144 reference implementation but not in Clang trunk, nor in Corentin Jabot's P2786 reference implementation.

`sus::collections::Vec<T>` optimizes `reserve` (compatible with both P2786 and P1144).
Trivial relocation is also used as a building block in `Vec::drain` (compatible with both P2786 and P1144).
`Vec` doesn't support arbitrary `insert` or `erase`.

Subspace's almost-sole-contributor Dana Jansens wrote a blog post describing Subspace's use of trivial relocation
in [`sus::mem::swap(T&, T&)`](https://github.com/chromium/subspace/blob/f9c481a241961a7be827d31fadb01badac6ee86a/sus/mem/swap.h#L38-L56),
which is compatible only with P1144, not P2786:

* ["Trivially Relocatable Types in C++/Subspace"](https://orodu.net/2023/01/15/trivially-relocatable.html) (Dana Jansens, January 2023)

Dana also commented [in favor](https://github.com/llvm/llvm-project/pull/84621#issuecomment-2015642051) on Clang #84621.


### [Thermadiag/seq](https://github.com/Thermadiag/seq)

The `seq` library ("a collection of original C++14 STL-like containers and related tools") implements
[`seq::is_relocatable`](https://github.com/Thermadiag/seq/blob/22c72e0bf02260fc0aa3f8cc68902320f5d91a4f/seq/type_traits.hpp#L179-L196)
as a synonym for `is_trivially_copyable` (compatible with P1144), but with code comments suggesting that it
is only used to replace move-construction and destruction (compatible with P2786 but not P1144).

However, [`seq::detail::CircularBuffer`](https://github.com/Thermadiag/seq/blob/22c72e0bf02260fc0aa3f8cc68902320f5d91a4f/seq/tiered_vector.hpp)
does optimize `insert` and `erase` (compatible only with P1144). It also provides the novel operation
[`push_back_pop_front_relocatable(v)`](https://github.com/Thermadiag/seq/blob/22c72e0bf02260fc0aa3f8cc68902320f5d91a4f/seq/tiered_vector.hpp#L2002-L2005),
which is a three-element rotate — `v` goes into the back of the queue while the front of the queue goes into `v` — optimized
into three memcpys. If we understand this as destroying and reconstructing `v`, it's compatible with both P2786 and P1144;
if we understand it as an assignment to `v`, it's compatible only with P1144.

`seq::any` has a policy parameter ([with good documentation](https://github.com/Thermadiag/seq/blob/main/docs/any.md#type-information-and-various-optimizations))
controlling whether it's allowed to hold non-trivially-relocatable types in its SBO buffer. This is compatible
with both P2786 and P1144.

`seq::radix_map` optimizes reallocation (compatible with both P2786 and P1144).


### [Thrust](https://github.com/nvidia/cccl)

NVIDIA Thrust implements
[`thrust::is_trivially_relocatable`](https://github.com/NVIDIA/cccl/blob/701b50d4a299e24daf5d945cde28e8df079c6937/thrust/thrust/type_traits/is_trivially_relocatable.h#L215-L223)
with P1144 semantics and documentation that refers to P1144R0.

It uses the trait in `async_copy_n` to relocate data from one place to another (compatible with both P1144 and P2786);
however, `async_copy_n` itself is then used as a building block of the in-place
[`async_stable_sort_n`](https://github.com/NVIDIA/cccl/blob/701b50d4a299e24daf5d945cde28e8df079c6937/thrust/thrust/system/cuda/detail/async/sort.h#L93-L120)
analogous to ParlayLib's code quoted [above](#parlaylib-supports-several-paral).
Since this uses relocation to permute elements during their lifetime ("swap by relocating"),
it is compatible with P1144 but not P2786.

Thrust has never taken commits or code-review from Arthur, Mungo, or Alisdair.


## History of P1144 and P2786 in WG21

<i>Links in this section tend to go to the committee minutes on the WG21 wiki, which is private and accessible to committee members only.</i>

* [P1144 "`std::is_trivially_relocatable`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p1144r10.html) (Arthur O'Dwyer, 2018–present)
    remains stuck in EWGI. It has been discussed three times in the six years since 2018:

  * At the February 2019 Kona meeting, EWGI saw P1144R3 and gave feedback.

  * At the February 2020 Prague meeting, EWGI [saw](https://wiki.edg.com/bin/view/Wg21prague/P1144R4SG17) P1144R4 (in my absence)
        and voted to forward to EWG (1–3–4–1–0, votes ordered "Strongly in Favor" to "Strongly Against").
        No changes were requested (since everyone seems to have assumed it was being forwarded).
        P1144R5 was published in March 2020, but was never scheduled again in WG21 for the next three years.

  * At the February 2023 Issaquah meeting, EWGI saw both P2786R0 and P1144R6. An author of P2786 asked that P2786 and P1144 be forwarded together to EWG,
        or not at all, so that EWG could have a design discussion seeing both designs at once. However, two separate forwarding polls were taken.
        P1144R6 (with one author present remotely) [polled](https://wiki.edg.com/bin/view/Wg21issaquah2023/EWGIP1144R6) 0–7–4–3–1;
        P2786R0 (with two) polled 1–8–3–3–1. The former was judged "not consensus" and the latter was judged "consensus."

  * At the November 2023 Kona meeting, trivial relocatability was discussed in a Friday informational-only session of EWG.
        No quorum was present and no minutes were taken.

* [P2786 "Trivial Relocatability for C++26"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2786r6.pdf) (Mungo Gill and Alisdair Meredith, 2023–present)
    has passed once through EWG and CWG, although it enters St Louis on LEWG's plate and also (luckily, although I don't quite understand
    how we got here procedurally) back on EWG's plate for further discussion.

  * P2786R0 was first published in February 2023, [seen](https://wiki.edg.com/bin/view/Wg21issaquah2023/EWGIP1144R6) by EWGI (as above),
      forwarded 1–8–3–3–1.

  * At the November 2023 Kona meeting, trivial relocatability was discussed in a Friday informational-only session of EWG.
        No quorum was present and no minutes were taken.

  * At the February 2024 Tokyo meeting, EWG [saw](https://wiki.edg.com/bin/view/Wg21tokyo2024/NotesEWGP2786) 
        P2786 alone (despite the author's request to discuss it alongside P1144).
        It was forwarded to CWG (7–9–6–0–2), seen by CWG, and approved. It was then sent to LEWG for consideration
        by that subgroup.

  * On 2024-04-09, LEWG [saw](https://wiki.edg.com/bin/view/Wg21telecons2024/P2786#Library_Evolution_Telecon_2024-04-09) P2786R4 in a Zoom telecon.
        It was pointed out by several participants that P2786 didn't match the semantics of any present-day library;
        even Bloomberg's own BSL uses the P1144 model.

* The past year (especially the months since the February 2024 Tokyo meeting) has seen several new papers in the space:

  * [P2967R1 "Relocation has a library interface"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p2967r1.pdf) (Alisdair Meredith and Mungo Gill, October 2023–present)

  * [P3233R0 "Issues with P2786"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3233r0.html) (Giuseppe D'Angelo, April 2024)

  * [P3236R1 "Please reject P2786 and adopt P1144"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3236r1.html) (several authors, April 2024–present)

  * [P3239R0 "A relocating `swap`"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3239r0.pdf) (Alisdair Meredith, May 2024)

  * [P3278R0 "Analysis of interaction between relocation, assignment, and swap"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3278r0.pdf) (Nina Ranns and Corentin Jabot, May 2024)

---

UPDATE, October 2024:

* At the June 2024 St Louis meeting, EWG [saw](https://wiki.edg.com/bin/view/Wg21stlouis2024/NotesEWGTrivialRelocatability)
    [P3233R0 "Issues with P2786"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3233r0.html) and
    [P3236R1 "Please reject P2786 and adopt P1144"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3236r1.html),
    and as a result voted (21–15–3–6–5) to "un-forward P2786 from CWG and bring it back to EWG." This is very good news;
    but at the same time, EWG still shuns <i>directly</i> discussing P1144.
    See ["How my papers did at St Louis"](/blog/2024/06/30/st-louis-trip-report/) (2024-06-30).
