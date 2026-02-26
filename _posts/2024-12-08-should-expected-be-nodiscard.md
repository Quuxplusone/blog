---
layout: post
title: "`std::expected` should be nodiscard"
date: 2024-12-08 00:01:00 +0000
tags:
  attributes
  exception-handling
  nodiscard
  proposal
  sum-types
excerpt: |
  A correspondent writes to me that he's switched from `throw/catch` exceptions to C++23's `std::expected`
  in all new code. That is, where a traditional C++ program would have:

      int f_or_throw();

      int g_or_throw() {
        f_or_throw();
          // arguably OK, discards f's result
          // but propagates any exceptional error
        return 1;
      }

  his programs always have:

      using Eint = std::expected<int, std::error_code>;
      [[nodiscard]] Eint f_or_error() noexcept;

      Eint g_or_error() {
        return f_or_error().transform([](int) {
          return 1;
        });
      }

  In the former case, we "discard" the result of `f_or_throw()` by simply discarding
  the value of the whole call-expression. That's safe, because errors are always
  signaled by exceptions, which will be propagated up the stack regardless of what
  we do with the (non-exceptional, non-error) return value. This ensures that the
  error is never swallowed except on purpose.
---

> This post has been updated several times since its original publication.
> See the section titled ["Adoption (since this post)"](#adoption-since-this-post)
> below: as more libraries implement this proposal, I've progressively replaced
> callouts with plaudits. Until December 2025, the title of this post was
> "Should `expected` be nodiscard?" By now I'm thoroughly comfortable with its new title.

A correspondent writes to me that he's switched from `throw/catch` exceptions to C++23's `std::expected`
in all new code. That is, where a traditional C++ program would have:

    int f_or_throw();

    int g_or_throw() {
      f_or_throw();
        // arguably OK, discards f's result
        // but propagates any exceptional error
      return 1;
    }

his programs always have:

    using Eint = std::expected<int, std::error_code>;
    [[nodiscard]] Eint f_or_error() noexcept;

    Eint g_or_error() {
      return f_or_error().transform([](int) {
        return 1;
      });
    }

In the former case, we "discard" the result of `f_or_throw()` by simply discarding
the value of the whole call-expression. That's safe, because errors are always
signaled by exceptions, which will be propagated up the stack regardless of what
we do with the (non-exceptional, non-error) return value. This ensures that the
error is never swallowed except on purpose.

In the latter case, we "discard" the result of `f_or_error()` by transforming it.
The code above is equivalent to:

    Eint g_or_error() {
      if (auto rc = f_or_error()) {
        return 1;
      } else {
        return rc;
      }
    }

> Notice that an `expected` is [truthy on success](https://en.cppreference.com/w/cpp/utility/expected/operator_bool);
> this is the opposite of the C convention for `errno`, exit codes, and so on, which are zero on success
> and non-zero on error. But it's consistent with `std::optional`, which is truthy when `.has_value()`.
> `expected` is also truthy when `.has_value()` — and falsey when `.has_error()`.

However, the one thing we definitely should _not_ do is discard the entire return value
of `f_or_error()` without checking for error first!

    Eint g_or_error_wrong() {
      f_or_error();
        // definitely a bug, discards the result
        // *and* swallows the error code from f
      return 1;
    }

So my correspondent marks his `f_or_error` function as `[[nodiscard]]`, which makes the compiler
diagnose the above as a mistake.

    warning: ignoring return value of function declared
    with 'nodiscard' attribute [-Wunused-result]
       12 | Eint g_or_error() { f_or_error(); return 1; } // 
          |                     ^~~~~~~~~~

Now, the same logic applies to every function that _ever_ returns an `expected`: nobody should ever
discard an `expected` value without checking it for error. So this ought to be the canonical case
of a class type marked `[[nodiscard]]` in the library!

    template<class T, class E>
    class [[nodiscard]] expected { ~~~~ };

Yet, for some reason, no STL vendor in 2024 marks `expected` as `[[nodiscard]]`.

## Patterns for error-handling

Basically, in traditional exception-based error handling, you have these idiomatic lines:

    return f();  // propagate both result and error
    f();  // propagate error, discard result
    try { f(); } catch (...) {}  // discard both

And in `expected`-based error handling, you have these:

    return f();  // propagate both result and error
    return f().and_then(~~~);  // propagate error, discard result
    (void)f();  // discard both

Notice that "discard both" has a sigil in both cases: `try { ~~ } catch(...){}`
in the former case and `(void)~~` in the latter. But that's just a style guideline
at the moment; as long as `expected` isn't marked `[[nodiscard]]` then it's also
legal to write just

    f();  // discard both

which confusingly _looks_ just like the "propagate error, discard result" line
from the former exception-based case — and dangerously is _possible_ to write
without noticing. That's the benefit of requiring the `(void)` sigil in order
to discard a return value: the sigil increases the edit distance
between the correct code and the incorrect code, while separating them with
a spacious "no man's land" of invalid code that will cause your build to fail.
See ["Two musings on the design of compiler warnings"](/blog/2020/09/02/wparentheses/#musing-suppression-mechanisms-are-about-edit-distance-and-about-signaling) (2020-09-02).

## Field experience (historical)

LLVM/Clang's own `llvm::Expected` and `llvm::Error` have been marked `[[nodiscard]]` [since October 2016](https://github.com/llvm/llvm-project/commit/8659d16631fdd1ff519a25d6867ddd9dbda8aea9).

Niall Douglas's `boost::outcome::result` has used `[[nodiscard]]` since at least May 2017.
This is briefly noted in [P0762 "Concerns about `expected<T,E>` from the Boost.Outcome peer review"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2017/p0762r0.pdf)
(October 2017), although I don't know if that paper was ever discussed by WG21.

Google Abseil's `absl::StatusOr<T>` has used `warn_unused_result` since at least 2020, and
`[[nodiscard]]` [since January 2022](https://github.com/abseil/abseil-cpp/blob/master/absl/status/statusor.h#L113-L121).

> Food for thought: Is it ever reasonable to have a function `f` that returns a nodiscard class type,
> where `f` wants to "opt out" of the class's nodiscard-ness? "This class is nodiscard except when used
> here"? I think the answer is no, but I'd be interested to see examples, both in the context of
> `expected` and in other contexts.

I added `[[nodiscard]]` to my own codebase's `Expected` type, and found everything still green.
But we make very little use of `Expected` in our code (only 34 hits in the entire codebase),
so this was not surprising.

## Adoption (since this post)

- Martin Moene added `[[nodiscard]]` to [martinmoene/expected-lite](https://github.com/martinmoene/expected-lite/commit/d426ef265a764359d16097c42d1357c19b01e218) on 2024-12-10.

- Stephan T. Lavavej [patched up](https://github.com/llvm/llvm-project/commit/cb4433b677a06ecbb3112f39d24b28f19b0d2626) the libc++ test suite
    by adding `(void)` casts on 2024-12-10. (But he did not go so far as to mark libc++'s own `expected` as `[[nodiscard]]` yet.)

- Stephan T. Lavavej added `[[nodiscard]]` to [Microsoft/STL](https://github.com/microsoft/STL/pull/5174) on 2024-12-13.

- Sy Brand added `[[nodiscard]]` to [tl/expected](https://github.com/TartanLlama/expected/pull/165) on 2025-07-12.

## Should STL vendors add `[[nodiscard]]` to `expected`?

Yes.

Microsoft STL proved themselves remarkably agile here! When I wrote this post
on 2024-12-08, Microsoft STL — who are the gold standard for "aggressively marking nodiscard"
in general — had not yet marked their `expected` as nodiscard. However, as a direct result of this post,
[Stephan T. Lavavej marked `expected` as nodiscard](https://github.com/microsoft/STL/commit/7643c270e5bfb1cfad62f8b5ff4045c662bdaf81)
on 2024-12-13! So `expected` will indeed be nodiscard in the next release of Visual Studio.

I encourage libstdc++ and libc++ to follow suit. For libstdc++,
[bug #109941](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=109941) seems related.

## What about other sum types, like `optional` and `variant`?

No. The same logic doesn't apply to arbitrary sum types like `variant`. Think of it this way:
We wouldn't put `[[nodiscard]]` on a simple value type like `string`, because it seems
plausible that you could call a function that happens to return a string but you don't care
about that result. Well, `optional<int>` is a value type just like `string` — you might call
a function that returns an optional result, and just not care about that result.

`expected<T,E>` has the same data layout as `variant<T,E>`, but its semantics are different;
that's why it has a different name from `variant`. It's still a value-semantic type, but
it doesn't hold just a "result": it holds a "result or error."
Discarding a result is usually OK. Discarding an _error_, on the other hand, can be very bad,
and should always be done explicitly!

## What about error types like `error_code`, then?

Probably yes! Given `std::error_code f()`, it is _probably_ a bug to call `f` and then
drop its return value on the floor.

Prior to this post, gold-standard Microsoft failed to mark their `error_code` as nodiscard;
likewise their exception types, such as `runtime_error` and `system_error`. But when
would you ever want to implicitly discard an exception object instead of, say, `throw`’ing it?
Thus, a few days after this post was written, Microsoft STL went and
[marked all their exception types](https://github.com/microsoft/STL/pull/5174).
However, they have not (yet?) marked their `error_code`.

It would be interesting to see a major STL vendor (libc++, libstdc++, or Microsoft)
mark their `error_code` as nodiscard. Would it in fact cause false positives, or would it
simply catch a lot of bugs? Or neither? My bet is on "neither."

But marking `expected` as nodiscard, I suspect, is just a plain good idea and _will_
catch a lot of bugs in the long run.

---

See also:

* ["`map::operator[]` should be nodiscard"](/blog/2025/12/18/nodiscard-operator-bracket/) (2025-12-18)
