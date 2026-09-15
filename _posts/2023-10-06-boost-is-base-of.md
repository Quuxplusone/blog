---
layout: post
title: "How `boost::is_base_of` matches private and/or ambiguous bases"
date: 2023-10-06 00:01:00 +0000
tags:
  classical-polymorphism
  explicit-object-parameters
  metaprogramming
  type-traits
excerpt: |
  The other day I learned that [Boost.TypeTraits](https://www.boost.org/doc/libs/1_83_0/libs/type_traits/doc/html/index.html)
  has a portable C++ implementation of `std::is_base_of`! You might think that `is_base_of`
  can be implemented simply as

      template<class B> std::true_type f(B*);
      template<class B> std::false_type f(void*);

      template<class B, class D>
      struct is_base_of : decltype(f<B>((D*)nullptr)) {};

  (modulo some bells and whistles like stripping cv-qualifiers and verifying that both `B` and `D` are
  class types).
  However, this isn't a full implementation of `is_base_of`'s standard semantics, because this
  implementation cannot handle private or ambiguous bases...
---

The other day I learned that [Boost.TypeTraits](https://www.boost.org/doc/libs/1_83_0/libs/type_traits/doc/html/index.html)
has a portable C++ implementation of `std::is_base_of`! You might think that `is_base_of`
can be implemented simply as

    template<class B> std::true_type f(B*);
    template<class B> std::false_type f(void*);

    template<class B, class D>
    struct is_base_of : decltype(f<B>((D*)nullptr)) {};

(modulo some bells and whistles like stripping cv-qualifiers and verifying that both `B` and `D` are
class types).
However, this isn't a full implementation of `is_base_of`'s standard semantics, because this
implementation cannot handle private or ambiguous bases ([Godbolt](https://godbolt.org/z/r6z9Wz7TW)):

    struct B {};
    struct D1 : private B {};
    static_assert(is_base_of<B, D1>::value); // hard error

    template<class> struct Bx : B {};
    struct D2 : Bx<int>, Bx<char> {};
    static_assert(is_base_of<B, D2>::value); // hard error

Instead, Boost.TypeTraits' implementation of `is_base_of`, formerly (pre-C++11) known as `is_base_and_derived`,
works [like this](https://godbolt.org/z/oTEnTxn6r) (modulo bells and whistles):

    template<class B, class D>
    struct Helper {
      template<class T = void>
      static std::true_type f(D*);  // #T
      static std::false_type f(B*); // #F
    };

    template<class B, class D>
    struct Host {
      operator B*() const; // #B
      operator D*();       // #D
    };

    template<class B, class D>
    struct is_base_of : decltype(
      Helper<B,D>::f(Host<B,D>())
    ) {};

Some historical explanations of how this code works are found in
[the Boost code's comment block](https://www.boost.org/doc/libs/1_83_0/boost/type_traits/is_base_and_derived.hpp)
and in a few comp.lang.c++.moderated messages from the trick's originator Rani Sharoni.

> Sadly a lot of comp.lang.c++.moderated is now unarchived by Google, and the January/February 2003 period
> in which this technique was discussed predates [the one public archive](https://archive.org/details/FULL-USENET-BACKUP-2020-Oct-comp.lang.c.moderated.101654.mbox.7z)
> I've found, which covers only July 2003–October 2020.
> Rani Sharoni discussed a similar technique in a different context in
> [`<df893da6.0312250556.515613@posting.google.com>`](https://groups.google.com/g/comp.lang.c++.moderated/c/KK8JLYclDtU/m/QZ1OQjba6z4J)  
> ("Re: Is this valid -> part-2.", December 25, 2003).)

Here's my own attempt to explain what's going on in this snippet.

---

Basically, there are four possible cases to consider here:

* `B` and `D` are the same type.
* `B` and `D` are unrelated types.
* `B` is a (perhaps inaccessible and/or ambiguous) base class of `D`.
* `D` is a (perhaps inaccessible and/or ambiguous) base class of `B`.

In the first case, `B` and `D` are the same type;
so #T and #F have the same parameter list
(except that #T is a template and #F isn't).
<a href="https://eel.is/c++draft/over.match.best#general-2.4">[over.match.best.general]/2.4</a>
([C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.best#2.4)) says that we should
prefer the non-template over the template. So if `B` and `D` are the same type, then
our trait will yield `false`.
(This is the wrong answer; but fixing this is easy.
That's one of the bells-and-whistles we're omitting in this walkthrough.)

In the second case, `B` and `D` are unrelated types.
Now we have two candidate functions #T and #F with _different_ parameter lists: #T takes `D*` and #F takes `B*`.
<a href="https://eel.is/c++draft/over.match.best#general-2">[over.match.best.general]/2</a>
([C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.best#2))
tells us that to choose the better viable candidate
we should compare the implicit conversion sequences (ICSes)
that produce those parameter types out of the argument type `Host<B,D>`.
The ICS producing `D*` is #D preceded by the identity conversion;
the ICS producing `B*` is #B preceded by a conversion from `Host<B,D>` to `const Host<B,D>&`.
Both ICSes are "user-defined conversion sequences," which means neither is _better_ than the other —
user-defined conversion sequences are comparable only when their user-defined pieces are the same
(<a href="https://eel.is/c++draft/over.match.best#over.ics.rank-3.3">[over.ics.rank]/3.3</a>;
[C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.best#over.ics.rank-3.3)),
and here #B and #D are different user-defined functions.
So our two viable candidates (#T and #F) have equally good ICSes.
Again <a href="https://eel.is/c++draft/over.match.best#general-2.4">[over.match.best.general]/2.4</a>
([C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.best#2.4))
breaks the tie in favor of the non-template, which is #F.
Our trait yields `false`, which is the correct answer in this case.

In the third case, `B` is a base class of `D`.
Again #T takes `D*`, #F takes `B*`, and we need to find
the ICSes that produce those parameter types from `Host<B,D>`.
The ICS producing `D*` is still #D preceded by the identity conversion.
For the ICS producing `B*`, we have a choice!
We could use the same ICS as above — #B _preceded by a conversion from `Host<B,D>` to `const Host<B,D>&`_
(<a href="https://eel.is/c++draft/over.match.best#over.ics.ref-1">[over.ics.ref]/1</a>;
[C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.best#over.ics.ref-1))
— or we could use #D _followed by a conversion from `D*` to `B*`_
(<a href="https://eel.is/c++draft/conv.ptr#3">[conv.ptr]/3</a>;
[C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/conv.ptr#3)).
To choose between these two user-defined conversion functions,
<a href="https://eel.is/c++draft/over.match.funcs#over.match.conv-1">[over.match.conv]/1</a>
([C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.funcs#over.match.conv-1))
says that we _recursively_ do overload resolution between #B and #D.
To use #B, we'd have to apply an identity conversion from `Host<B,D>` to `const Host<B,D>&`.
To use #D, we'd have to apply a no-op identity conversion,
which is better because it's less qualified
(<a href="https://eel.is/c++draft/over.match.best#over.ics.rank-3.2.6">[over.ics.rank]/3.2.6</a>,
[C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.ics.rank#3.2.6)).
Therefore, we prefer #D.
Okay, to recap: We're doing overload resolution between #T and #F.
The ICS producing #T's `D*` (we now know) is just #D.
The best ICS producing #F's `B*` is #D followed by a standard conversion from `D*` to `B*`.
The former is a subsequence of the latter
(or, in the words of <a href="https://eel.is/c++draft/over.ics.rank#3.3">[over.ics.rank]/3.3</a>
([C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.best#over.ics.rank-3.3)),
their user-defined pieces are the same and the former's second standard conversion (the identity conversion)
is better than the latter's), so overload resolution will prefer the former and choose #T.
Our trait yields `true`, which is the correct answer in this case.

In the fourth case, `D` is a base class of `B`.
Again #T takes `D*`, #F takes `B*`, and we need to find
the ICSes that produce those parameter types from `Host<B,D>`.
The ICS producing `B*` is #B _preceded by a conversion from `Host<B,D>` to `const Host<B,D>&`_.
For the ICS producing `D*`, we have a choice!
We could use #B _preceded by a conversion from `Host<B,D>` to `const Host<B,D>&`
and followed by a standard conversion from `B*` to `D*`_
(remember, in this case `D` is a base of `B`, not vice versa), or we could just use #D directly.
Obviously we'll prefer #D directly.
So, the ICS producing #T's `D*` is #D;
the ICS producing #F's `B*` is #B preceded by a conversion from `Host<B,D>` to `const Host<B,D>&`.
As in our second case, their user-defined pieces are different,
therefore neither one is a better conversion sequence than the other
(<a href="https://eel.is/c++draft/over.match.best#over.ics.rank-3.3">[over.ics.rank]/3.3</a>;
[C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.best#over.ics.rank-3.3)).
Again <a href="https://eel.is/c++draft/over.match.best#general-2.4">[over.match.best.general]/2.4</a>
([C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.best#2.4))
breaks the tie in favor of the non-template #F. Our trait yields `false`, which is
the correct answer in this case.

Notice that in no case do we ever wind up actually _evaluating_ a pointer conversion
from `D*` to `B*`, so it never matters that base `B` might be inaccessible and/or
ambiguous. In each case where such a conversion is possible, by definition we end up
calling #T, whose parameter type is `D*`.

---

Boost's `is_base_of` relies on
<a href="https://eel.is/c++draft/over.match.best#general-2.4">[over.match.best.general]/2.4</a>
([C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.best#2.4))
to break ties in cases 2 and 4. We could substitute another tiebreaker, e.g. the new-in-C++20
<a href="https://eel.is/c++draft/over.match.best#general-2.9">[over.match.best]/2.9</a>
([C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.best#2.9)),
which says that a non-reversed rewritten candidate should be preferred over a
reversed one. ([Godbolt](https://godbolt.org/z/rd95ecGYq).)

    template<class B, class D>
    struct Helper {
      friend constexpr int operator<=>(Helper, D*) { return 1; }
      friend constexpr int operator<=>(B*, Helper) { return 0; }
    };

    template<class B, class D>
    struct Host {
      constexpr operator B*() const { return nullptr; }
      constexpr operator D*() { return nullptr; }
    };

    template<class B, class D>
    struct is_base_of : std::bool_constant<
      Host<B,D>() < Helper<B,D>()
    > {};

---

Boost's `is_base_of` uses a qualification conversion to make #B less preferred than #D
in cases 3 and 4. We can substitute another standard conversion from
[the table in [over.ics.scs]](https://timsong-cpp.github.io/cppwp/n4861/over.ics.scs#tab:over.ics.scs),
e.g. the derived-to-base conversion, if only we can figure out how to shoehorn it into
a user-defined conversion function like `operator B*`. I don't think that's possible
pre-C++23 because of
<a href="https://eel.is/c++draft/over.match.funcs#general-4.sentence-3">[over.match.funcs.general]/4.2</a>
([C++20 DIS](https://timsong-cpp.github.io/cppwp/n4861/over.match.funcs#4.sentence-3));
but in C++23, we can do it via an explicit object parameter
([Godbolt](https://godbolt.org/z/9E9x1jYKn)).

    template<class B, class D>
    struct Helper {
      friend constexpr int operator<=>(Helper, D*) { return 1; } // #T
      friend constexpr int operator<=>(B*, Helper) { return 0; } // #F
    };

    struct HostBase {};

    template<class B, class D>
    struct Host : HostBase {
      constexpr operator B*(this HostBase) { return nullptr; } // #B
      constexpr operator D*(this Host) { return nullptr; } // #D
    };

    template<class B, class D>
    struct is_base_of : std::bool_constant<
      Host<B,D>() < Helper<B,D>()
    > {};
