---
layout: post
title: "`map::operator[]` should be nodiscard"
date: 2025-12-18 00:01:00 +0000
tags:
  attributes
  c++-style
  llvm
  nodiscard
  proposal
  stl-classic
---

Lately libc++ has been adding the C++17 `[[nodiscard]]` attribute aggressively to every header.
(I'm not sure why _this_ month, but my guess is that libc++ just dropped support
for some old compiler such that all their supported compilers now permit the attribute
even in C++11 mode.) libc++ is following the trail that Microsoft STL has blazed
[since VS 15.6 in late 2017](https://devblogs.microsoft.com/cppblog/c17-progress-in-vs-2017-15-5-and-15-6/).

Some functions, like `std::min`, always make sense to mark `[[nodiscard]]`.
Others, like [`unique_ptr::release`](https://en.cppreference.com/w/cpp/memory/unique_ptr/release),
are deliberately left unmarked because even though it is _usually_ a bug to write

    p.release();

_sometimes_ that's actually what you meant — you're calling it only for its side effect.
Back in 2022, Stephan T. Lavavej [estimated](https://www.youtube.com/watch?v=FwLRIvUV584&t=267s)
of `unique_ptr::release` that "90% of discards are a bug, but 10% are maybe valid...
Even though it would find bugs, the cost in false positives is too great."
However, I think it's worth pointing out that the fix for a false positive is trivial:
all you have to do is refactor that line of code into

    (void)p.release();

and the warning goes away. So personally I'd apply `[[nodiscard]]` even to `unique_ptr::release`.
But it would _clearly_ create noise to apply `[[nodiscard]]` to, for example, `printf` (which
returns `int`) or [`vector::erase`](https://en.cppreference.com/w/cpp/container/vector/erase)
(which returns an iterator).

----

An interesting borderline case came up this week. In [llvm-project#169971](https://github.com/llvm/llvm-project/pull/169971),
Hristo Hristov marked libc++'s `map::operator[]` as nodiscard. The assumption was that it is
usually a bug to write

    mymap[key];

Hans Wennborg quickly [reported](https://github.com/llvm/llvm-project/pull/169971#issuecomment-3656203466)
that actually Google's codebases do this a fair bit. Statements calling `map::operator[]` purely for its
side-effect are found
[in Chromium](https://source.chromium.org/chromium/_/swiftshader/SwiftShader/+/ff4435d3f92dabbf65e210033ea178359ba7db0e:third_party/SPIRV-Tools/source/opt/ir_context.cpp;l=774):

    // Map the result id to the empty set.
    combinator_ops_[extension->result_id()];

[in V8](https://github.com/v8/v8/blob/3cf7ba6f3659b1f59eac1253e053a7545200280a/src/compiler/turboshaft/late-load-elimination-reducer.cc#L350):

    // We need to insert the load into the truncation mapping as a key, because
    // all loads need to be revisited during processing.
    int32_truncated_loads_[op_idx];

and even [in flatbuffers](https://github.com/google/flatbuffers/blob/a86afae9399bbe631d1ea0783f8816e780e236cc/src/idl_parser.cpp#L3991):

    if (attributes.Add(kv->key()->str(), value)) {
      delete value;
      return false;
    }
    parser.known_attributes_[kv->key()->str()];

This last example doesn't even come with a comment to explain it. If I owned this code, I'd absolutely
worry that some coworker would come along and refactor it to

    parser.known_attributes_[kv->key()->str()] = false;

But that would be incorrect! Assignment-of-`false` is an unconditional assignment; but what we have
above is a _conditional_ assignment: "If this key isn't yet in the map, then assign it `false`; if it
is already in the map, leave its value alone." That is, it's the verb that the STL calls
[`.try_emplace`](https://en.cppreference.com/w/cpp/container/map/try_emplace), and as a maintainer
I'd insist that that line be rewritten for clarity into

    parser.known_attributes_.try_emplace(kv->key()->str(), false);

However, if that's not possible (maybe because the codebase needs to keep working pre-C++17),
then we can at least add the cast-to-void:

    // Map to "false" only if the attribute isn't already mapped
    (void)parser.known_attributes_[kv->key()->str()];

Actually, the GitHub history records that the code originally had a very clear
(albeit slightly inefficient) `if` statement:

    if (known_attributes_.find(attribute->key()->str()) == known_attributes_.end())
      known_attributes_[attribute->key()->str()] = false;

which [was replaced](https://github.com/google/flatbuffers/pull/5077/commits/ce032fe253af2095dd56c782003674f951aa946f#diff-3e4e0d4b8ff36f318f58a28c083e2395f035dfadb7b0a636cd49f5643e8b00e2L2884-L2885)
with the current inscrutable version during review.

----

Anyway, libc++ responded by removing `[[nodiscard]]` from `map::operator[]` again in
[#172444](https://github.com/llvm/llvm-project/pull/172444).
I think they should have stuck to their guns: `mymap[key];` is a horrible way to write
`mymap.try_emplace(key)` and STL vendors shouldn't condone it.

Microsoft STL doesn't mark `map::operator[]` as nodiscard, either, presumably because they
encountered this "idiom" in the wild too. Microsoft (and now libc++) _do_ mark
the side-effect-less `operator[]`s of `array`, `deque`, and `vector`.

LLVM internally doesn't mark the `operator[]`s of
[`llvm::DenseMap`](https://github.com/llvm/llvm-project/blob/03d044971eccac47ed8518684ea18ba413cd5748/llvm/include/llvm/ADT/DenseMap.h#L341-L347),
[`llvm::MapVector`](https://github.com/llvm/llvm-project/blob/03d044971eccac47ed8518684ea18ba413cd5748/llvm/include/llvm/ADT/MapVector.h#L98-L100), or
[`llvm::StringMap`](https://github.com/llvm/llvm-project/blob/03d044971eccac47ed8518684ea18ba413cd5748/llvm/include/llvm/ADT/StringMap.h#L275-L277),
although my experimentation shows that they could do so with only a handful of minor
fixups ([here](https://github.com/llvm/llvm-project/blob/03d044971eccac47ed8518684ea18ba413cd5748/llvm/lib/Transforms/IPO/LowerTypeTests.cpp#L1153),
[here](https://github.com/llvm/llvm-project/blob/03d044971eccac47ed8518684ea18ba413cd5748/llvm/lib/Analysis/ModuleSummaryAnalysis.cpp#L492),
[here](https://github.com/llvm/llvm-project/blob/03d044971eccac47ed8518684ea18ba413cd5748/llvm/lib/LTO/ThinLTOCodeGenerator.cpp#L1101-L1104),
[here](https://github.com/llvm/llvm-project/blob/03d044971eccac47ed8518684ea18ba413cd5748/llvm/lib/Target/AArch64/AArch64TargetTransformInfo.cpp#L3970-L3972)),
in many cases by using the more expressive and efficient `tryEmplace` they've already
implemented and used as a building block for their `operator[]`.

> PSA: If your codebase contains any instances of the `m[k];` "idiom,"
> please change them to `m.try_emplace(k);` or to `(void)m[k];` with a code comment.
> Your code reviewers will thank you, and so will your library vendors!

See also:

* ["Should `std::expected` be nodiscard?"](/blog/2024/12/08/should-expected-be-nodiscard/) (2024-12-08) — Yes, it should.
