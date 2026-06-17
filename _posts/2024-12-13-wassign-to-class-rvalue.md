---
layout: post
title: "Field-testing `-Wassign-to-class-rvalue`"
date: 2024-12-13 00:01:00 +0000
tags:
  compiler-diagnostics
  llvm
  nodiscard
  proposal
  proxy-references
excerpt: |
  Perennially it is noticed that C++ forbids assignment to scalar rvalues, while
  permitting assignment to class rvalues. That is:

      T f();
      f() = "xyz";

  is forbidden when `T=const char*`, but permitted when `T=string`.
---

Perennially it is noticed that C++ forbids assignment to scalar rvalues, while
permitting assignment to class rvalues. That is:

    T f();
    f() = "xyz";

is forbidden when `T=const char*`, but permitted when `T=string`.
This permissiveness is bad, because it makes it easy to write bogus code like:

    struct Book {
      std::string title_;
      std::string author_;
      auto& title() { return title_; }
      auto author() { return author_; }
    };

    Book b;
    b.title() = "Hamlet"; // OK
    b.author() = "Shakespeare"; // Oops!

The first line "correctly" assigns to `b.title_` through the returned
reference. The second line incorrectly assigns to the temporary string
returned from `b.author()`, whose value is thrown away at the end
of the line: `b.author_` itself is not modified, and no warning is given.

Going back to our `T f()` example: Notice that `f() = "xyz"` is also permitted
when `T` is a reference type, like `string&` or `const char*&`. C++'s
permissiveness in allowing assignment to arbitrary class types was, historically,
a good thing, because it allowed us to invent class types that behave like
native references — "proxy reference" types. Examples include
`vector<bool>::reference` and `pair<int&, int&>`.

    using T = std::tuple<int&>;

    int x;
    T f() { return std::tie(x); }

    f() = std::make_tuple(42);
      // OK, assigns through to x

Assigning to an rvalue `tuple<int>` is basically always a bug.
But assigning to an rvalue `tuple<int&>` is _not_ a bug.
Can we get the compiler to distinguish these two situations?

<b>Yes we can.</b>

The key observation is that when `T` is a reference type, `const T` is the
same type — so `const T` is just as assignable as `T`, for reference types.
In C++23, [P2321](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p2321r2.html)
applied this logic to all of the STL's "proxy reference" types, too:
`vector<bool>::reference`, `tuple<int&>`, etc., all now have a const-qualified `operator=`.
We say that such types are "const-assignable."

> What about `tuple<int&, int>`? Its `operator=` remains non-const-qualified,
> because part of its value is not a "proxy reference." Assigning to an rvalue
> of this class type is still almost certainly a bug, because you're throwing away
> the second half of the assigned value at the end of the expression.
>
> What about `std::ignore`? Yes, it has been const-assignable since C++11.

Thus, starting in C++23, the compiler has a fairly foolproof way to tell whether
an assignment-to-materialized-temporary is a bug or not: It's a bug if and only if
the left-hand object is not const-assignable.

## Implementing `-Wassign-to-const-rvalue`

I implemented a new diagnostic in [my Clang fork](https://github.com/Quuxplusone/llvm-project/),
available [on Godbolt](https://godbolt.org/z/WzK9KhPe9). The logic goes as follows:

- Whenever overload resolution for `=`, or any compound-assignment operator (e.g. `+=`, `<<=`),
    or postfix `++` or `--`, resolves to an implicit-object member function with no
    `const` qualifier and no ref-qualifier, and the actual left-hand operand is an rvalue...

- ...quietly redo the overload resolution as if the left-hand operand had been a const-qualified
    xvalue instead. If this second overload resolution succeeds, then there's no problem:
    the operand must be a const-assignable proxy-reference type. (We still use the result of
    the first resolution, of course.) But, if the second resolution fails:

- If we're in C++23-or-later, or the operator is not `=`, then emit the warning
    `-Wassign-to-class-rvalue`. (This warning is enabled by `-Wall`.)

- If we're in C++20-or-earlier and the operator is `=`, then emit the warning
    `-Wcxx23-compat-assign-to-class-rvalue`. (This warning is off by default.)
    This special case deals with the fact that the library's own `tuple` and `pair` aren't
    const-assignable until C++23; they would cause millions of warnings otherwise.

Of course, if STL vendors would backport P2321's const-assignability fixes to
all language modes, instead of limiting them to C++23-and-later, then we could
think about eliminating that special case.

## Implementing `-Wpreincrement-of-const-rvalue`

Prefix `++` and `--` are another difficulty. You might think that if `T` is not a
proxy reference type, then

    T f();
    ++f();

should always be a bug. But this turns out to be a common pattern for _iterators:_

    struct Container {
      It begin();
      It end();
      auto& back() {
        return *--end();
      }
    };

I observed that ASIO uses not only `--end()` but `++begin()`.
(Oddly enough, that line also happens to be the subject of a correctness bug report,
[boost #883](https://github.com/boostorg/boost/issues/883).)

    return address_v6_range(
        address_v6_iterator(address_v6(begin_bytes, address_.scope_id())),
        ++address_v6_iterator(address_v6(end_bytes, address_.scope_id())));

So, we duplicate all of the above logic for prefix `++`/`--` as well, but
under a different warning flag:

- If the operator is prefix `++` or `--`, then emit the warning
    `-Wpreincrement-of-class-rvalue`. (This warning is off by default.)

## Field experience

I compiled my employer's codebase with both of these warnings turned on; I also compiled
the LLVM/Clang codebase itself.

In my employer's codebase, we had to fix only two proxy-reference types. One was
this `LogFinisher` class from a really old version of Google Protobuf:

    class LIBPROTOBUF_EXPORT LogFinisher {
     public:
      void operator=(LogMessage& other) {
        other.Finish();
      }
    };

and the other was basically a pre-C++20 version of
[`std::atomic_ref<int>`](https://en.cppreference.com/w/cpp/atomic/atomic_ref/operator_arith2).
Notice that C++20's `atomic_ref::operator++` and `atomic_ref::operator+=`
arrived already const-qualified; P2321 didn't need to touch them.

In LLVM/Clang, I observed four patterns that triggered this diagnostic.
None of them were bugs, as far as I could tell.

> In fact, that might be the most surprising takeaway here:
>
> I conceived this diagnostic as a guard against accidental no-ops like
> `b.author() = "Shakespeare"`, but in fact it found exactly <b>zero</b>
> cases of that bug in the wild.

Here are the four patterns that triggered `-Wassign-to-class-rvalue` in LLVM/Clang:

### 1. Actual proxy reference types

LLVM contains three copies of basically this same code (`PackedVector`, `BitVector`, and `SmallBitVector`).

    reference& operator=(reference t) {
      *this = bool(t);
      return *this;
    }

    reference& operator=(bool t) {
      doSomethingWith(t, BitPos);
      return *this;
    }

We could simply slap a `const` on there, but it's strictly better to write:

    bool operator=(reference t) const {
      return *this = bool(t);
    }

    bool operator=(bool t) const {
      doSomethingWith(t, BitPos);
      return t;
    }

This breaks anyone trying to use the result of `(rx = y)` as an lvalue — they deserve to be broken —
while preserving the ability to write chained assignments like `(rx = ry = z)`. And yes, LLVM's codebase
does contain at least one chained assignment like that!

### 2. `APSInt` with value

`llvm::APSInt` represents a $$k$$-bit integer with value $$n$$. But it doesn't have a constructor
taking both $$k$$ and $$n$$ together. Instead, the constructor takes $$k$$ and the assignment operator
takes $$n$$. So the idiomatic way of creating an `APSInt` temporary with a given value
(seen [here](https://github.com/llvm/llvm-project/blob/6cfad635d5aaa01abb82edc386329d8ed25078e1/clang/include/clang/StaticAnalyzer/Core/PathSensitive/APSIntType.h#L70)
and [here](https://github.com/llvm/llvm-project/blob/6cfad635d5aaa01abb82edc386329d8ed25078e1/clang/lib/Sema/SemaChecking.cpp#L6311))
is to evaluate the expression `(llvm::APSInt(k, false) = n)`.

Now, `operator=` returns `APSInt&`, so the next thing you usually end up doing is making
a new copy of that `APSInt` anyway. It would be better if `class APSInt` provided a factory
method like `APSInt(k, false).withValue(n)`.

### 3. Redundant copying

[Here](https://github.com/llvm/llvm-project/blob/6cfad635d5aaa01abb82edc386329d8ed25078e1/llvm/include/llvm/Analysis/BlockFrequencyInfoImpl.h#L153-L164) we have
several functions of this form:

    inline BlockMass operator+(BlockMass L, BlockMass R) {
      return BlockMass(L) += R;
    }

which can of course be simplified to:

    inline BlockMass operator+(BlockMass L, BlockMass R) {
      return L += R;
    }

Both versions assume that `BlockMass` is cheap to copy (since again, `L += R` is an lvalue).
If moving from `L` might be cheaper than copying, then we should surely rewrite as:

    inline BlockMass operator+(BlockMass L, const BlockMass& R) {
      L += R;
      return L;
    }

Similarly, [here](https://github.com/llvm/llvm-project/blob/6cfad635d5aaa01abb82edc386329d8ed25078e1/llvm/include/llvm/Support/ScaledNumber.h#L718-L740)
we pass `L` by reference, unconditionally make a copy into a temporary,
and then copy _again_ in the return statement because `<<=` returns an lvalue, not an rvalue:

    ScaledNumber operator<<(const ScaledNumber &L,
                            int16_t Shift) {
      return ScaledNumber(L) <<= Shift;
    }

Better to rewrite in the same way as `BlockMass`'s `operator+` above.

### 4. Premature optimization

[Here](https://github.com/llvm/llvm-project/blob/6cfad635d5aaa01abb82edc386329d8ed25078e1/clang/lib/Basic/Warnings.cpp#L42-L43):

    StringRef Suggestion = DiagnosticIDs::getNearestOption(Flavor, Opt);
    Diags.Report(diag::warn_unknown_diag_option)
        << (Flavor == diag::Flavor::WarningOrError ? 0 : 1)
        << (Prefix.str() += std::string(Opt)) << !Suggestion.empty()
        << (Prefix.str() += std::string(Suggestion));

Here the programmer seems to be worried that `a + b` will allocate a whole new buffer distinct
from `a` or `b`, whereas `a += b` can simply append into `a`'s existing buffer. But this isn't
so! When `a` is an rvalue — as `Prefix.str()` is here — `std::string`'s `operator+` is perfectly
smart enough to reuse the expiring string's buffer for the result. (This is overload #9 on
[cppreference's list](https://en.cppreference.com/w/cpp/string/basic_string/operator%2B); notice
it's existed since move semantics were invented, in C++11.)

So we can safely replace `+=` with `+` here; this communicates our intent better.

In fact, this example is exactly the sort of thing we started out desiring to warn against!
It's quite plausible that a programmer might write

    Prefix.str() += std::string(Opt);
    Prefix.str() += std::string(Suggestion);

incorrectly thinking it would modify `Prefix`'s string value.

## Sounds like a job for `[[nodiscard]]`

Using `+=` in the case above was unnecessary, but it wasn't really a _bug,_ because we
went on to use the resulting string value. Recall that prefix `--end()` was troublesome
for the same reason:

    auto back = *--end(); // OK
    --end(); // certainly a bug

This suggests that we could refine our diagnostic criteria
to warn about mutations of non-const-assignable rvalues _only if the return value of the
mutating operator is itself discarded._ Basically, the compiler could apply the logic from the bullet
points above, but then instead of _directly_ diagnosing the call to `operator=`, it would simply act
as if that `operator=` were marked `[[nodiscard]]`, and emit any diagnostics resulting from that.
This would silence the warnings on cases 2, 3, and 4,
but still diagnose actually buggy cases such as `b.author() = "Shakespeare"`.

On the other hand, in each of the "false positive" cases 2, 3, and 4, we found ways to
refactor the confusing code to make it clearer and faster. So I think the diagnostic
would be pretty useful in its present state, even without any refinements.

## Whither from here?

I think this experiment shows that `-Wassign-to-class-rvalue` could really be a
useful diagnostic. I'd love to see three changes out of this:

- Compiler vendors should implement a warning like this (with or without the nodiscard
    refinement). Almost certainly it could be named better; suggestions welcome!

- STL vendors should backport P2321's changes to all language versions, thus clearing
    the way for users to enable this warning in their existing C++17 and C++20 codebases
    without tripping over millions of warnings from `pair` and `tuple`.

- WG21 can stop speculating about whether `operator=` ought to be ref-qualified by default
    in the core language, whether new types ought to have ref-qualified `operator=`, etc.,
    because the benefits of such a change would _already be achieved_ simply by turning
    on this warning. Language and library changes can have unpredictable effects, but
    compiler warnings are cheap and easy and can be incrementally adopted in a codebase.

You can play around with the warnings I've implemented by visiting [Godbolt](https://godbolt.org/z/WzK9KhPe9).
Those warnings, again, are `-Wassign-to-class-rvalue`, `-Wpreincrement-of-class-rvalue`, and `-Wcxx23-compat-assign-to-class-rvalue`.
