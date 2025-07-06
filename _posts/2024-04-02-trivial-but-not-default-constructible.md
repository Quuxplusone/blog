---
layout: post
title: 'Trivial, but not trivially default constructible'
date: 2024-04-02 00:01:00 +0000
tags:
  cppcon
  language-design
  triviality
  value-semantics
excerpt: |
  I just watched Jason Turner's CppCon 2023 talk ["Great C++ `is_trivial`."](https://www.youtube.com/watch?v=bpF1LKQBgBQ)
  Circa [41m00s](https://www.youtube.com/watch?v=bpF1LKQBgBQ&t=41m) he picks on
  the wording of [[class.prop]/2](https://eel.is/c++draft/class.prop#def:class,trivial):

  > A <b>trivial class</b> is a class that is trivially copyable and has one or more eligible
  > default constructors, all of which are trivial.

  "That phrasing's always bothered me. How can you have more than one default constructor?
  They could be constrained by `requires`-clauses... but then not more than one of them is
  [eligible](https://eel.is/c++draft/special#def:special_member_function,eligible)."
---

I just watched Jason Turner's CppCon 2023 talk ["Great C++ `is_trivial`."](https://www.youtube.com/watch?v=bpF1LKQBgBQ)
Circa [41m00s](https://www.youtube.com/watch?v=bpF1LKQBgBQ&t=41m) he picks on
the wording of [[class.prop]/2](https://eel.is/c++draft/class.prop#def:class,trivial):

> A <b>trivial class</b> is a class that is trivially copyable and has one or more eligible
> default constructors, all of which are trivial.

"That phrasing's always bothered me. How can you have more than one default constructor?
They could be constrained by `requires`-clauses... but then not more than one of them is
[eligible](https://eel.is/c++draft/special#def:special_member_function,eligible)."

Here's the trick: Jason, you were wrongly assuming that a trivial class must be default-constructible at all!
[Godbolt](https://godbolt.org/z/EGjb17of3):

    template<class T>
    struct S {
        S() requires (sizeof(T) > 3) = default;
        S() requires (sizeof(T) < 5) = default;
    };

    static_assert(std::is_trivial_v<S<int>>);
    static_assert(not std::is_default_constructible_v<S<int>>);

Here `S<int>` is trivial, but, because it has _two_ eligible default constructors,
any attempt to default-construct an `S<int>` will fail due to overload resolution ambiguity.
Thus `S<int>` is not default-constructible at all (and certainly not _trivially_
default-constructible); but it still satisfies all the requirements of a _trivial class_.

---

By the way, it seems to me that Jason buries the lede when it comes to actually explaining triviality.
The rule is: A type `T` is _trivially fooable_ if and only if
its _foo_ operation is (known to the compiler to be) equivalent to the same _foo_ operation on
`T`'s object representation (i.e., an array of `unsigned char`). `T` is trivially copy-constructible
if its copy constructor merely copies its bytes; it's trivially default-constructible
if its default constructor merely default-initializes its bytes (recall that default-initializing
an array of `unsigned char` is a no-op); it's trivially value-initializable
(see ["PSA: Value-initialization is not merely default-construction"](/blog/2023/06/22/psa-value-initialization/) (2023-06-22))
if its value initialization merely value-initializes its bytes (to zero); and so on. _That's_ the
big thing to remember about the adverb "trivially" — it means "as if on the object representation."
