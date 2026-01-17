---
layout: post
title: "Divergence in instantiating unused functions of unevaluated lambdas"
date: 2026-01-17 00:01:00 +0000
tags:
  constexpr
  cpplang-slack
  implementation-divergence
  lambdas
  llvm
---

Here's a weird vendor divergence recently spotted on the cpplang Slack.
Consider a function template `foo`, where instantiating the body of
`foo<int>` will give an error. For example:

    template<class T>
    int foo() {
      return *T();
    }

Now suppose we take this invalid-to-instantiate function and call it in the
capture-list of an unevaluated lambda:

    using L1 = decltype( [x=foo<int>()]{ return x; } );

Normally, calling a function requires that we instantiate its body. But here,
we need only to complete the lambda type, which means figuring out that
`foo<int>()` returns `int` (and therefore `x` is an `int` data member), and
probably instantiating the declarations of `L1`'s constructor and `operator()`,
but doesn't necessarily mean instantiating the _definition_ of `L1`'s constructor,
which is the only thing that would actually codegen a call to `foo<int>`.

Recall that a lambda is equivalent to a class type with an `operator()`:

    struct Unnamed {
      Unnamed() = default;
      int operator()() const { return x_; }
      int x_ = foo<int>();
    };

    using L2 = decltype(Unnamed());

Now here's the weird divergence. EDG and MSVC are both happy with both of these
snippets; they don't mind that you're referring to `foo<int>()` as long as it
doesn't actually need to be called anywhere in the generated code.
But Clang complains only about `L1` ([Godbolt](https://godbolt.org/z/zfdE5Md7P)):

    error: indirection requires pointer operand ('int' invalid)
        3 |     return *T();
          |            ^~~~
    note: in instantiation of function template specialization 'foo<int>' requested here
       10 |     [x=foo<int>()]{ return x; }
          |        ^

And GCC complains only about `L2`:

    In instantiation of 'int foo() [with T = int]':
    required from here
       19 |     int x_ = foo<int>();
          |              ~~~~~~~~^~
    error: invalid type argument of unary '*' (have 'int')
        3 |     return *T();
          |            ^~~~

These two snippets — with the lambda-expression, or with the unnamed struct type —
really ought to be equivalent. But Clang 16+ and GCC both treat them as non-equivalent;
and disagree as to which one they find error-worthy.

> Clang 15-and-older complains about both `L1` and `L2`. Something changed
> in Clang 16.x to make it like `L2` better than `L1`.

Presumably this has something to do with the point at which GCC and Clang decide to
instantiate the bodies of otherwise-unused constexpr functions;
see perhaps [Clang bug #59966](https://github.com/llvm/llvm-project/issues/59966).
You can annoy more compilers by adding the `constexpr` keyword in various places:
adding it (redundantly) to `L2`'s defaulted constructor will annoy MSVC, and adding
it to `foo` will cause EDG to reject `L1` and every compiler to reject `L2`.
Why GCC and MSVC remain happy with `L1` in that case, I couldn't tell you.

---

Speaking of templates the instantiation of which gives an error,
you should know that in 2023, [CWG2518](https://cplusplus.github.io/CWG/issues/2518.html)
finally made it legal to write

    template<class T>
    int foo() {
        static_assert(false);
        return 0;
    }

as long as `foo` is never instantiated. Before 2023, this was an error
because the condition of the `static_assert` wasn't dependent: it would be
evaluated in Phase 1, as soon as the template was seen, and immediately
diagnosed as an error. Now, it's legal to write such a template:
even a non-dependent `static_assert` is guaranteed not to be diagnosed
until Phase 2. This improvement was [DR'ed](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#dr)
back to all older versions of C++, so it's not a "C++23 feature"; it's
a "C++ feature, introduced in calendar year 2023."
