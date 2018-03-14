---
layout: post
title: 'Using `future_error` for control flow'
date: 2018-03-13 00:03:00 +0000
tags:
  c++-style
  exception-handling
excerpt: |
  `logic_error` is itself a logic error... or is it?
---

[Niall Douglas writes:](https://ned14.github.io/outcome/tutorial/default-actions/#fn:1)

> I take exception to throwing logic error type exceptions ...
> If used as control flow, then there are **always** much superior alternatives.
> If used for spotting a true logic error, then you just detected bad logic by the programmer
> or memory corruption, in which case this situation is **not** recoverable and you really ought to fatal exit.

In a draft proposal, someone whom I respect wrote, along the same lines:

> A precondition (e.g., `[[expects...]]`) violation is never a reportable error;
> it is always a bug in the caller (the caller shouldn’t be making the call).
> — Corollary: `std::logic_error` (and derivatives) should not exist and should never be thrown.
> `logic_error`'s existence is itself a logic error; it should be removed and any use of it in
> the standard library should be replaced with a contract.

To which I wrote in response:

> I agree half-heartedly with your take on `logic_error`. I've certainly heard it before.
> I wonder, though; would you attack `future_error` quite as vehemently as `logic_error`?
> At what point does an exceptional codepath cross over from "the caller made a bug,
> they should never do this" to "the caller might legitimately rely on this codepath's
> functionality"?

Here is a possible implementation of `when_either` which relies on the library implementation
of [`std::promise::set_value`](http://en.cppreference.com/w/cpp/thread/promise/set_value) to
throw `future_error` as an indication of the error condition "promise was already set by some
other thread."

    std::future<void> when_either(std::future<void> a, std::future<void> b) {
        auto prom = std::make_shared<std::promise<void>>();
        auto set_value_or_pass = [prom](auto){ 
            try {
                prom->set_value();
            } catch (const std::logic_error&) { }
        });
        a.then(set_value_or_pass);
        b.then(set_value_or_pass);
        return prom->get_future();
    }

Consider how you'd implement `when_either` if the behavior of `std::promise::set_value`
had *not* been defined to use exceptions for control flow.

I don't know where I'm going with this, really.  Just some food for thought.
