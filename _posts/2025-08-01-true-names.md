---
layout: post
title: "True names matter in C++"
date: 2025-08-01 00:01:00 +0000
tags:
  cpplang-slack
  implementation-divergence
  name-lookup
excerpt: |
  In C++, there's often multiple ways to refer to the same entity: aliases,
  inherited names, injected names, names that differ in qualification...
  Usually, if an entity has multiple names, which one you use won't matter:
  you can use any of those synonymous names and the compiler won't care.

  But each entity always has a uniquely "truest" name, privileged above
  all the other ways to refer to it. In some situations, in order to do a thing
  with an entity, you do actually need to call it by its "true name." Here is a
  (non-exhaustive) list of such situations.
---

Here's a post I've been meaning to write for a few years now.

In C++, there's often multiple ways to refer to the same entity: aliases,
inherited names, injected names, names that differ in qualification...
Usually, if an entity has multiple names, which one you use won't matter:
you can use any of those synonymous names and the compiler won't care.
For example:

    struct A {};
    namespace N {
      using B = ::A;
    }
    template<class T> struct C { using D = T; };

    A f(N::B x);

    C<A>::D f(A x) {
      // OK, because all of A, N::B, and C<A>::D
      // are aliases for the same type
      return x;
    }

Here the compiler "looks through" aliases to find out that `C<A>::D` is simply
an alias for `A`, so the function definition of `f` above does indeed match
its declaration. Both declaration and definition are equivalent to `A f(A)`.
Or again:

    struct B {
      int mf();
    };

    struct S { using A = B; };
    int S::A::mf() { return 1; }

Here the compiler finds that `S::A` is simply `B`, so the member function definition
of `mf` above does indeed provide a definition of `B::mf`, even though it's using
a funky name to do it.

But each entity always has a uniquely "truest" name, privileged above
all the other ways to refer to it. In some situations, in order to do a thing
with an entity, you do actually need to call it by its "true name." Here is a
(non-exhaustive) list of such situations.

> Sometimes an entity, such as a lambda type, has a "true name" that is not accessible
> to you-the-programmer. You might reasonably say, then, that a lambda type doesn't have
> any true name at all. But I prefer to think that it _does_ have some true name "out there"
> in the Platonic realm, even though it is ineffable, and even though any name the programmer
> can pronounce (such as `decltype(lam)`) must be at best an un-true synonym for that true Platonic name.
> The advantage of this Platonic mode of thinking is that it lets us reason about a mapping
> from entities to true names that is both total and one-to-one — that is, identical entities have
> identical true names, different entities have different true names, and no third case need be considered.

## Argument-dependent lookup (ADL)

A type alias, `using`-declaration, or `using`-directive can pull a name from one namespace
into another. But if you make an unqualified function call on an argument of that type, ADL
will still look in the namespaces associated with the type's _original_ (true) namespace,
regardless of the namespace you wrote in your code. [Godbolt](https://godbolt.org/z/x4dMr9bYW):

    namespace N1 {
      struct A {};
      int f(A) { return 1; }
    }
    namespace N2 {
      using B = N1::A;
      int f(B) { return 2; }
    }
    int main() {
      N2::B b;
      return f(b);
    }

This program unambiguously returns 1, not 2, because `f(b)` does ADL on an argument of
actual type `N1::A` and finds `N1::f`. The existence of `N2::f` doesn't matter.

This is the gotcha that matters most in practice. It will particularly come into play
if you're thinking of replacing

    namespace my {
      class String { ~~~~ };
    }

with

    namespace my {
      using String = std::string;
    }

since that changes the "true name" of `my::String` from "`my::String`" to "`std::string`,"
which belongs to a completely different namespace.

See also ["How `hana::type<T>` disables ADL"](/blog/2019/04/09/adl-insanity-round-2/) (2019-04-09)
and ["What if `vector<T>::iterator` were just `T*`?"](/blog/2022/03/03/why-isnt-vector-iterator-just-t-star/) (2022-03-03).

## In the debugger (name mangling)

When the compiler name-mangles a signature like `int f(A)` into a linker symbol like `_Z1f1A`,
it is always `A`'s true name that goes into the mangling. After all, this is the only way
to ensure that there is a single unique mangled name for each unique function in the program,
no matter whether it's declared as `f(A)` or `f(B::C)`.

Therefore, anything that relies on "demangling" these symbol names back into human-readable form
will likely display only the true name of each type. In particular, when you step through a
program in the debugger, relying on its DWARF debug info, you should expect to see only true names.

In fact, even though a debugger like `gdb` or `lldb` might understand type aliases in some places,
it generally will require you to use true names when doing fundamental things like setting
breakpoints. For example ([Godbolt](https://godbolt.org/z/qqnx7nhxa)):

    struct A {
      static int f();
    };

    struct B { using C = A; };
    struct D : A {};
    namespace N2 { using ::A; }
    namespace N3 { using E = ::A; }

You can call `f` by any of the names `A::f`, `B::C::f`, `D::f`, `N2::A::f`, or `N3::E::f`
(and define it by any of those except `D::f`), but you'll find that neither `gdb` nor `lldb`
are willing to accept anything but `A::f` as the location of a breakpoint.

    (gdb) b N2::A::f
    Function "N2::A::f" not defined.
    Make breakpoint pending on future shared library load? (y or [n]) n
    (gdb) b A::f
    Breakpoint 1 at 0x1132: file test.cpp, line 11.

    (lldb) b B::C::f
    Breakpoint 1: no locations (pending).
    WARNING:  Unable to resolve breakpoint to any actual locations.
    (lldb) b A::f
    Breakpoint 2: where = a.out`A::f() at test.cpp:11:3, address = 0x0000000100000360

It is also perhaps noteworthy that neither `gdb` nor `lldb` understands C++11 inline namespaces:

    namespace Outer {
      inline namespace Inner {
        int f();
      }
    }

`f` can be called or defined as `Outer::Inner::f` or simply as `Outer::f`. But when you go to
set a breakpoint on it in the debugger, you must use its true name, `Outer::Inner::f`; the
debugger will not accept `Outer::f` as a synonym.

> Both `gdb` and `lldb` will conveniently accept any unambiguous proper suffix of the true name:
> If there's only one `f` in your program, you can just `b f`, and if there's only one
> `Inner`, you can `b Inner::f`. But it must be a proper suffix of the <b>true</b> name,
> not of a conventional name like `Outer::f`.

## Functions, classes, and enums must be defined in or above their true namespace

([Godbolt.](https://godbolt.org/z/51MjT7xxn)) When you declare a function inside a namespace,
there are two idiomatic ways to then define it out-of-line. Either:

    // in the .h file
    namespace N1 {
    namespace N2 {
    int f();
    struct A {
      int mf();
    };
    } // namespace N2
    } // namespace N1

    // in the .cpp file
    namespace N1 {        
    namespace N2 {
    int f() { return 1; }
    int A::mf() { return 2; }
    } // namespace N2
    } // namespace N1

Or:

    // in the .cpp file
    int N1::N2::f() { return 1; }
    int N1::N2::A::mf() { return 2; }

When you have nested namespaces like this, you can also split the difference:

    // in the .cpp file
    namespace N1 {
    int N2::f() { return 1; }
    int N2::A::mf() { return 2; }
    } // namespace N1

That is, the definition of any function in `N1::N2` can legally appear inside `N1::N2`,
or (with qualification) inside `N1`, or (with more qualification) at the global scope —
basically, as long as it's somewhere along the path to its fully qualified true name.

Meanwhile, wherever you do define `N1::N2::A::mf`, you don't have to name it with
its true name. You can, for example, write ([Godbolt](https://godbolt.org/z/v46qoaYGv)):

    namespace P3 { using N1::N2::A; }
    int P3::A::mf() { return 2; }

Basically this says, "I'd like to define the `mf` member function of some type. Which type?
The type that I can refer to here as `P3::A`." (Its true name is `N1::N2::A`, but we don't
have to use that name here.)

But even while you can define `mf` using the un-true name `P3::A::mf`, you still must define
it _in the proper place_ — somewhere along the path to its fully qualified _true_ name.
All vendors correctly reject an attempt like:

    namespace P3 {
      using N1::N2::A;
      int A::mf() { return 2; }  // error
    }

And all vendors correctly reject the non-member-function analogue:

    namespace P3 { using N1::N2::f; }
    int P3::f() { return 1; }  // error

The subtlety here (as far as I can tell) is that there's a difference between defining the `mf`
member of "the type that I can refer to here by various names, but I'm definitely talking about
_that specific type_ unambiguously" and defining a non-member function whose name, after all, _is_
`N1::N2::f`, and `P3` is _not_ just another name for `N1::N2`.

> So if `P4` were just another name for `N1::N2`, then we could do it?
> Yes ([Godbolt](https://godbolt.org/z/cx34KbfW7)):
>
>      namespace P4 = N1::N2;  // a namespace alias
>      int P4::f() { return 1; }  // OK

Also notice that while a lambda's true name is ineffable, we can use this template-specialization
trick to infer something about a lambda's true _namespace_ — on a particular implementation,
that is. ([Godbolt.](https://godbolt.org/z/PEcT995dj))

## Definitions and specializations via "member of derived"

Even when `Base::X` and `Derived::X` refer to the same member type or member function,
compilers often treat these two names differently in practice — and here be
vendor divergence galore.
Consider the following example ([Godbolt](https://godbolt.org/z/8qPe3Mq8e)).

    struct B {
      struct M1;
      struct M2;
    };

    using A = B;
    struct A::M1 {};  // OK

    struct D : B {};
    struct D::M2 {};  // error on Clang and MSVC

We're declaring that `B` has member types `M1` and `M2`. Then we define `B::M1`
via the synonymous name `A::M1` (all four compiler vendors are happy with that).
Then we try to define `B::M2` via the synonymous name `D::M2`: notice that `D::M2`
is indeed the same type as `B::M2`, since `D` doesn't define any name `M2` of
its own (which would have hidden the base class's `M2` from name lookup).
EDG and GCC remain happy, but Clang and MSVC reject.

Contrariwise, if we are not defining a member type but specializing a member template
([Godbolt](https://godbolt.org/z/YroGWrTje)):

    struct B {
      template<class> struct Temp {};
    };

    using A = B;
    template<> struct A::Temp<char> {};  // OK

    struct D : B {};
    template<> struct D::Temp<int> {};  // error on EDG

then GCC, Clang, and MSVC are happy with our attempt to refer to `B::Temp` as `D::Temp`,
and this time it's EDG that rejects.

> Notice that no matter what (un-true) name you use to specialize a template,
> every template specialization, just like every function and type definition,
> must be declared in a namespace that is somewhere along the path to the primary
> template's fully qualified true name. See
> ["Why can't I specialize `std::hash` inside my own namespace?"](/blog/2024/07/15/specialization-in-another-namespace/) (2024-07-15).

## More?

I'm surely missing some cases where knowing the true name of a type is important.
Probably at least one or two that will make me go "d'oh!" when they're pointed out to me.
So why not [drop me an email](mailto:arthur.j.odwyer@gmail.com) and point them out?
