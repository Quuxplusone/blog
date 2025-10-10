---
layout: post
title: "What breaks without implicit `T*`-to-`bool` conversion?"
date: 2025-10-05 00:01:00 +0000
tags:
  cpplang-slack
  language-design
  llvm
  overload-resolution
  proposal
excerpt: |
  C++20 took a small step by deciding
  (via [P1957](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1957r2.html))
  that a `T*`-to-`bool` conversion should be considered narrowing, and thus forbidden
  in list-initialization. But could we go further and make `T*`-to-`bool` conversion
  _non-implicit?_

  I patched my local copy of Clang to forbid implicit conversion from `T*` to `bool`.
  Here's all the things that broke when I compiled LLVM/Clang itself with my patched compiler.
---

Pop quiz: Does this program return `1` or `2`?

    int f(bool) { return 1; }
    int f(std::string_view) { return 2; }
    int main() { return f("hello"); }

It returns `1`. Overload resolution sees that we have a choice of converting `"hello"`
(effectively a `const char*`) to either `bool` or `string_view`. The former is a
built-in _standard conversion_, while the latter is a user-defined conversion (which
calls the constructor of the user-defined `string_view` class type), so overload resolution
considers the former to be a better match.

However, this is surprising and usually unwanted. C++20 took a small step by deciding
(via [P1957](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1957r2.html))
that a `T*`-to-`bool` conversion should be considered _narrowing_, and thus forbidden
in list-initialization. But could we go further and make `T*`-to-`bool` conversion
_non-implicit?_

The idea is to forbid implicit conversions—

    void take(bool);
    bool give(int *p) {
      bool b = p;  // OUTLAWED
      take(p);     // OUTLAWED
      return p;    // OUTLAWED
    }

while still permitting explicit and contextual conversions—

    bool okay(int *p) {
      bool b(p);            // OK
      if (p) blah();        // OK
      bool c = (p && p);    // OK
      bool d = (p || p);    // OK
      int e = (p ? 1 : 2);  // OK
    }

So, I patched my local copy of Clang to forbid implicit conversion from `T*` to `bool`.
See the patch [here](https://github.com/Quuxplusone/llvm-project/commit/2025-10-05-no-implicit-pointer-to-bool-conversion~).
Here's all the things that broke when I compiled LLVM/Clang itself with my patched compiler.

> Unless you're an LLVM maintainer, you might want to skip to the end. The majority of
> this post is a catalog of bugs and unfortunate code patterns, similar to what you'll find
> on the [PVS-Studio](https://pvs-studio.com/en/blog/posts/cpp/1188/) blog.

## The "hypocritical `operator bool`" pattern

As we know, you should always mark your `operator bool` as `explicit`, so that it will be available
for contextual conversions but not implicit ones. (That is, your class type will have the behavior
that `T*` today fails to have.) But it seems to be _very_ common that a programmer writing such
an `operator bool` will rely on implicit conversion in the `return` statement! "Rules for thee but
not for me," you know.

    struct MyPtr {
      int *ptr_;
      explicit operator bool() const { return ptr_; }
    };

The Clang/LLVM codebase itself contains 30 instances of this
"hypocritical `operator bool`" pattern. To eliminate the implicit `T*`-to-`bool` conversion here,
I recommend `return ptr_ != nullptr`, `return bool(ptr_)`, or `return !!ptr_`.

(This audit incidentally turned up incorrectly non-`explicit` `operator bool`s
[here](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/lib/Transforms/Scalar/NewGVN.cpp#L708)
[here](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/lib/Transforms/IPO/OpenMPOpt.cpp#L363),
[here](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/lib/Analysis/HashRecognize.cpp#L115),
[here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/lib/Sema/SemaTemplateDeduction.cpp#L231);
and incorrectly non-`const`-qualified `operator bool`s
[here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/lib/AST/ByteCode/InterpState.h#L39),
[here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/lib/Sema/SemaTemplateInstantiate.cpp#L116).
A good linter would flag all of these.)

## Classes convertible to `T*` but not `bool`

Consider:

    std::atomic<int*> a;
    if (a) {

My patched Clang complains: `no viable conversion from 'std::atomic<int *>' to 'bool'`.
That's because the conversion from `atomic<T*>` to `bool` happens in two steps: first
the user-defined conversion via `atomic<T*>::operator T*() const`, and then a standard conversion
from `T*` to `bool` (which we chose to outlaw). The workaround here is to write
explicitly `if (a.load())`.

> Operations on atomics (even loads and stores) should always be explicit.
> Limit yourself to one atomic operation per source line.

The Clang/LLVM codebase contains two instances of this pattern with `std::atomic`, and nine instances
involving user-defined "smart-pointer-like" types such as
[`class TypedTrackingMDRef`](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/include/llvm/IR/TrackingMDRef.h#L126-L129) and
[`class MDOperand`](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/include/llvm/IR/Metadata.h#L929-L932)
which provide a converting `operator T*` but no dedicated `operator bool`.

> Class types which provide `operator T*` should probably also provide `explicit operator bool`.
> However, your class type almost certainly should not provide `operator T*` to begin with.
> Prefer a named method such as `.get()`, `.value()`, or `.load()`.

Also, classes whose `operator T*` is never-null should probably _not_ provide an
`operator bool`, because that would just mask misuses; see "Probable bug #1" below.

## `assert`-like functions

The standard library's `assert` macro is [defined](https://eel.is/c++draft/assertions#assert-2.1)
to wrap its argument in a contextual conversion to `bool`.
But if you've written your own `assert`-like function which is _not_ a macro
and which takes `bool` as its first parameter...

    void my_assert(bool cond);
    int main(int, char **argv) {
      assert(argv);     // OK, no problem
      my_assert(argv);  // OUTLAWED
    }

The LLVM/Clang codebase has one such function, used twice.

## A surprising `T *isFoo()` antipattern

The usual convention in LLVM/Clang — and I hope in general — is to have paired accessors
like this:

    bool hasFoo() const;
    const Foo *getFoo() const;

But [in this one place](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/include/clang/AST/DeclCXX.h#L1552-L1564)
the programmer has "cleverly" combined both accessors into one:

    const FunctionDecl *isLocalClass() const;

Since pointers implicitly convert to bools, this lets the programmer write:

    if (RD->isLocalClass())
      const FunctionDecl *FD = RD->isLocalClass();

But it means that our patched compiler complains [here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/lib/Sema/SemaTemplateInstantiateDecl.cpp#L6886):

    error: assigning to 'bool' from incompatible type 'FunctionDecl *'
     6902 |       NeedInstantiate = RD->isLocalClass();
          |                         ~~~~^~~~~~~~~~~~~~

The same pattern recurs [here](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/include/llvm/CodeGen/LiveIntervals.h#L317-L319) in LLVM.

## Intentional use in `bool` initializers and assignments

LLVM/Clang relies on pointer-to-boolean conversion pretty heavily, for things like
[this](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/lib/IR/EHPersonalities.cpp#L109):

    bool EHa = M->getModuleFlag("eh-asynch");
    const bool HaveSpace = ::strchr(ArgV[I], ' ');
    bool IgnoreErrors = Errs;

This kind of thing comes up about 73 times in LLVM/Clang. Each instance is trivial (if tedious) to patch:

    bool EHa = (M->getModuleFlag("eh-asynch") != nullptr);
    bool HaveSpace = (::strchr(ArgV[I], ' ') != nullptr);
    bool IgnoreErrors = (Errs != nullptr);

A special case comes up at least 6 times, e.g.
[here](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/lib/Analysis/MemoryDependenceAnalysis.cpp#L909)
and [here](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/lib/CodeGen/SelectionDAG/SelectionDAGBuilder.cpp#L3756-L3757):

    isInvariantLoad = LI->getMetadata(LLVMContext::MD_invariant_load);

would be better written as

    isInvariantLoad = LI->hasMetadata(LLVMContext::MD_invariant_load);

This is why these boolean accessors exist, after all!

## Actual bug #1

Consider [this code](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/lib/CodeGen/GlobalISel/RegBankSelect.cpp#L848-L850):

    MachineBasicBlock &Src = *MI.getParent();
    for (auto &Succ : Src.successors())
      addInsertPoint(Src, Succ);

The patched compiler complains (in part):

    llvm/lib/CodeGen/GlobalISel/RegBankSelect.cpp:850:7:
    error: no matching member function for call to 'addInsertPoint'
      850 |       addInsertPoint(Src, Succ);
          |       ^~~~~~~~~~~~~~
    llvm/include/llvm/CodeGen/GlobalISel/RegBankSelect.h:375:10:
    note: candidate function not viable: no known conversion from 'llvm::MachineBasicBlock *'
    to 'MachineBasicBlock &' for 2nd argument; dereference the argument with *
      375 |     void addInsertPoint(MachineBasicBlock &Src, MachineBasicBlock &Dst);
          |          ^                                      ~~~~~~~~~~~~~~~~~~~~~~
    llvm/include/llvm/CodeGen/GlobalISel/RegBankSelect.h:371:10:
    note: candidate function not viable: no known conversion from 'llvm::MachineBasicBlock *'
    to 'bool' for 2nd argument
      371 |     void addInsertPoint(MachineBasicBlock &MBB, bool Beginning);
          |          ^                                      ~~~~~~~~~~~~~~

In vanilla C++, it's unambiguously that second overload of `addInsertPoint` that gets called.
But circumstantial evidence suggests that the programmer meant to write

    MachineBasicBlock &Src = *MI.getParent();
    for (auto *Succ : Src.successors())
      addInsertPoint(Src, *Succ);

to call the first overload instead.

## Actual bug #2

[Here](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/lib/Target/AArch64/AArch64InstrInfo.cpp#L125):

    error: no matching member function for call to 'shouldSignReturnAddress'
      125 |     if (!MFI->shouldSignReturnAddress(MF))
          |          ~~~~~^~~~~~~~~~~~~~~~~~~~~~~
    note: candidate function not viable: no known conversion from 'const MachineFunction *'
    to 'const MachineFunction' for 1st argument; dereference the argument with *
      551 |   bool shouldSignReturnAddress(const MachineFunction &MF) const;
          |        ^                       ~~~~~~~~~~~~~~~~~~~~~~~~~
    note: candidate function not viable: no known conversion from 'const MachineFunction *'
    to 'bool' for 1st argument
      552 |   bool shouldSignReturnAddress(bool SpillsLR) const;
          |        ^                       ~~~~~~~~~~~~~

Again, in vanilla C++ `MF` would unambiguously convert to `bool` and call the second overload.
The programmer wrote `MF` when he meant `*MF`.

> Don't gratuitously give two different functions the same name. These functions should have been named distinctly,
> e.g. `shouldSignReturnAddressOf(MF)` and `shouldSignReturnAddressGivenSpill(bool)`. But even within
> an unavoidable overload set, you should avoid overloading on a `bool` argument, because of how
> promiscuously things convert to `bool`. Prefer to take an enum or maybe even an `int`, rather than
> a `bool`.

## Actual bug #3

[Here](https://github.com/llvm/llvm-project/blob/63ca8483d0/llvm/lib/Transforms/Utils/LoopUnrollAndJam.cpp#L246):

    if (!UnrollRuntimeLoopRemainder(L, Count, /*AllowExpensiveTripCount*/ false,
            /*UseEpilogRemainder*/ true,
            UnrollRemainder, /*ForgetAllSCEV*/ false,
            LI, SE, DT, AC, TTI, true,
            SCEVCheapExpansionBudget, EpilogueLoop)) {

The patched compiler complains:

     error: no matching function for call to 'UnrollRuntimeLoopRemainder'
     note: candidate function not viable: no known conversion
     from 'Loop **' to 'bool' for 14th argument

When you see the compiler talking about a function's "14th argument," you know you're
in for a fun time. The intended callee's signature is:

    LLVM_ABI bool UnrollRuntimeLoopRemainder(
        Loop *L, unsigned Count, bool AllowExpensiveTripCount,
        bool UseEpilogRemainder, bool UnrollRemainder, bool ForgetAllSCEV,
        LoopInfo *LI, ScalarEvolution *SE, DominatorTree *DT, AssumptionCache *AC,
        const TargetTransformInfo *TTI, bool PreserveLCSSA,
        unsigned SCEVExpansionBudget, bool RuntimeUnrollMultiExit,
        Loop **ResultLoop = nullptr);

[Default function arguments are the devil.](/blog/2020/04/18/default-function-arguments-are-the-devil/)
In this case, it looks like the programmer forgot to pass a boolean value for
the 14th argument `RuntimeUnrollMultiExit`, and so his intended 15th argument,
`ResultLoop`, was quietly coerced to bool instead. This should be fixed by eliminating
the default function argument and passing all 15 arguments explicitly at each call-site.

## Probable bug #1: Testing never-null raw pointers

LLVM/Clang is full of places where a function takes a simple boolean, but the programmer apparently
thought something more structured was required. For example,
[here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/lib/CodeGen/CGStmt.cpp#L1680)
the programmer writes `Builder.getTrue()` when a simple `true` would suffice. `Builder.getTrue()`
returns a pointer to a compiler-internal data structure representing truthiness — which, since it is
a non-null pointer, implicitly converts to `true`! Note that `Builder.getFalse()` would also
evaluate to `true`.

    error: cannot initialize a parameter of type 'bool' with an rvalue of type 'ConstantInt *'
     1678 |       Builder.CreateFlagStore(Builder.getTrue(), NRVOFlag);
          |                               ^~~~~~~~~~~~~~~~~

Likewise [here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/lib/CodeGen/CGStmtOpenMP.cpp#L4625)
and [here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/lib/CodeGen/CGOpenMPRuntime.cpp#L2054):

    error: cannot initialize a parameter of type 'bool' with an lvalue of type 'llvm::IntegerType *'
     4581 |                                  ? EmitScalarExpr(Filter, CGM.Int32Ty)
          |                                                           ^~~~~~~~~~~

And [here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/lib/CodeGen/CodeGenModule.cpp#L7080-L7082)
(along a codepath where `Value` is invariably non-null):

    error: cannot initialize a parameter of type 'bool' with an lvalue of type 'APValue *'
     7061 |         MaterializedType.isConstantStorage(getContext(), /*ExcludeCtor*/ Value,
          |                                                                          ^~~~~

## Probable bug #2: Testing never-null smart pointers

[Here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/lib/Frontend/PrecompiledPreamble.cpp#L126),
[here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/lib/AST/ASTImporter.cpp#L10255),
and [here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/unittests/Basic/FileManagerTest.cpp#L548):

    error: no viable conversion from 'DirectoryEntryRef' to 'bool'
     10255 |       if (Cache->OrigEntry && Cache->OrigEntry->getDir()) {
           |                               ^~~~~~~~~~~~~~~~~~~~~~~~~~
    note: candidate function
       87 |   operator const DirectoryEntry *() const { return &getDirEntry(); }
          |   ^

This is the same pattern as "Classes convertible to `T*` but not `bool`" above; but in this case
the existing `operator T*` invariably returns non-null. So this `if` test was never testing anything.

This bug was detectable only because `DirectoryEntryRef` defined `operator T*` without `operator bool`.
It certainly should not define _either_ of those conversions (and my understanding is that that's the
maintainers' long-term plan, too).

## Surprising usage in `checkEnvVar`

[Here](https://github.com/llvm/llvm-project/blob/63ca8483d0/clang/tools/driver/driver.cpp#L124-L135)
the Clang programmer has done something very weird. Essentially, it's:

    template<class T>
    T checkEnvVar(const char *varname) {
        const char *p = ::getenv(varname);
        if (p == nullptr) return T();
        return value;
    }

The `checkEnvVar` template is instantiated in only two ways. `checkEnvVar<std::string>` returns `""` if the variable
is unset, or the contents of the variable if it's set. `checkEnvVar<bool>` returns `false` if the
variable is unset, or `true` if it's set (even if it's set to the empty string, or to `0`).
The same template code happens to implement both behaviors.
I was surprised to see this, but it really does seem to be intentional.
The workaround is to change `return value` to `return T(value)`.

## The final tally

In the LLVM/Clang codebase as of this writing — I very roughly estimate 3.8 million lines of code —
here's the total count of places that rely on implicit `T*`-to-`bool` conversion
in various ways:

| Pattern         | Count |
|-----------------|-------|
| `getMetadata` for `hasMetadata` | 6 |
| `checkEnvVar`   | 1 |
| Actual bug #1   | 1 |
| Actual bug #2   | 1 |
| Actual bug #3   | 1 |
| Probable bug #1 | 3 |
| Probable bug #2 | 3 |
| `bool x = p;`   | 48 |
| Other `x = p;`  | 19 |
| `atomic<T*>`                                | 2 |
| Other `operator T*` without `operator bool` | 9 |
| Hypocritical `operator bool`                | 30 |
| Other `return p;`                           | 117 |
| `Hash.AddBoolean(p)` | 21 |
| `Diag(Loc, diag::err_riscv_type_requires_extension, D)` | 8 |
| `assert_with_loc`    | 2 |
| Other `f(p)`         | 50 |
| **Total**            | 322 |

I also audited another, proprietary, C++17 codebase — very roughly 1.2 million lines of code.
Its base rate of `T*`-to-`bool` conversions per line was only 30% of LLVM/Clang's. The audit
turned up one non-`explicit` `operator bool`, and zero bugs.
(EDIT: Actually that `operator bool` was responsible for a bug, albeit not involving
`T*`-to-`bool` conversion. See
["Implicit `operator bool` participates in comparison"](/blog/2025/10/10/implicit-operator-bool-considered-harmful/)
(2025-10-10).)

| Pattern         | Count |
|-----------------|-------|
| `bool x = getenv(s);` | 2 |
| Other `bool x = p;`   |  3 |
| `atomic<T*>`          | 3 |
| Hypocritical `operator bool` | 2 |
| Other `return p;`     | 16 |
| `f(p)`                | 4 |
| **Total**             | 30 |
