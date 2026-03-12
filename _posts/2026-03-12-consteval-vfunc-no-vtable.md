---
layout: post
title: "MSVC's `/experimental:constevalVfuncNoVtable` is non-conforming"
date: 2026-03-12 00:01:00 +0000
tags:
  abi
  constexpr
  classical-polymorphism
  cpplang-slack
  msvc
  today-i-learned
---

[P1064 "Allowing Virtual Function Calls in Constant Expressions,"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p1064r0.html#_proposed_changes)
adopted for C++20, permits you to mark virtual functions as `constexpr` and
then call them at compile-time. In fact, you can even mark them `consteval`
(also a C++20 feature), which means you can call them _only_ at compile-time.
Thus ([Godbolt](https://godbolt.org/z/1Eo4Edde4)):

    struct Base {
      consteval virtual int f() const { return 1; }
    };
    struct Derived : Base {
      consteval int f() const override { return 2; }
    };

    constexpr const Base *m() { static constexpr Derived d; return &d; }

    static_assert(std::is_polymorphic_v<Base>);
    static_assert(std::is_polymorphic_v<Derived>);
    static_assert(sizeof(Base) == 8);
    static_assert(sizeof(Derived) == 8);
    static_assert(typeid(*m()) == typeid(Derived));
    static_assert(m()->f() == 2);

None of the `static_assert`s above should be surprising. `Base` is certainly
[polymorphic](https://eel.is/c++draft/class.virtual#def:class,polymorphic)
(it has virtual methods); the result of `typeid` and the behavior of `m()->f()` are mandated
by the Standard; and `sizeof(Base)` is 8 because of its 8-byte vptr. Now,
`Base`’s vptr is really useful only at compile time,
because `f`, the only virtual function in `Base`’s vtable, is callable only at
compile time. So you might think we could somehow save some space by giving
`Base` a "`consteval`-only vptr" — omitting the vptr from its struct layout at runtime.
However, there are several problems with that idea.

First: Can we give `Base` two different layouts — include the vptr in the constexpr-time layout
and omit it from the runtime layout? No, because that would make `sizeof` give two
different values depending on whether the expression `sizeof(Base)` was seen in a
[manifestly constant-evaluated](https://stackoverflow.com/questions/65429853/how-to-understand-the-definition-of-manifestly-constant-evaluated)
context. That would be a nightmare.

Second: If we omit `Base`'s vptr at compile time _and_ at runtime, (A) how would that affect
its type traits? and (B) where would we hang its runtime type information?

Today I learned that MSVC 16.11 (way back in 2021) added an experimental switch
to see what happens if you answer those questions the "wrong" way. The switch is named
[`/experimental:constevalVfuncNoVtable`](https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#iso-c++20-continuing-work-defect-reports-and-clarifications).
If you turn it on, you'll observe the following surprising behavior
([Godbolt](https://godbolt.org/z/96dr8xh41)):

    static_assert(!std::is_polymorphic_v<Base>);
    static_assert(!std::is_polymorphic_v<Derived>);
    static_assert(sizeof(Base) == 1);
    static_assert(sizeof(Derived) == 1);
    static_assert(typeid(*m()) == typeid(Base));
    static_assert(m()->f() == 2);

In this experimental mode, MSVC omits `Base`’s vptr — changing its `sizeof` at both compile time and
runtime. Yet `m()->f()` continues to perform virtual dispatch and call `Derived::f()`! (The compiler's
built-in constexpr interpreter knows the true dynamic type of every object created at constexpr time,
so this is easy for it. And you can't call `f` at runtime.)

In this mode, MSVC decides that `Base` is actually non-polymorphic (despite the Standard's clearly stating
that it is: it has virtual methods). Having discarded polymorphic-ness, MSVC also classifies `Base` as
[empty](https://eel.is/c++draft/tab:meta.unary.prop) and trivially copyable; furthermore,
`typeid(*m())` can skip the evaluation of `*m()` (because such evaluation
is required only for glvalue expressions of _polymorphic_ type) and invariably return `typeid(Base)`.

Could MSVC make this mode's behavior more conforming by patching their type-traits to report that
`Base` _is_ polymorphic (and non-empty, and non-trivially copyable), and making `typeid` report the correct
dynamic type via the same compiler magic that powers the dynamic dispatch in `m()->f()`?
Almost, but not quite. They could use compiler magic to make `typeid` work polymorphically at constexpr time;
but without a vtable to hang the RTTI on, `typeid` would certainly _have_ to act non-polymorphically at runtime,
[as seen here](https://godbolt.org/z/184vM8K5s).

Conclusion: MSVC's five-year-old `/experimental:constevalVfuncNoVtable` gives you a non-conforming
experience, in which types with all-`consteval` virtual functions are treated as non-polymorphic.
Its opposite, `/experimental:constevalVfuncVtable`, gives conforming behavior that matches GCC and Clang
(as far as I know).
