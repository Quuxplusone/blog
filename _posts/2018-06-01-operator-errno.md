---
layout: post
title: 'Setting errno in `operator T`'
date: 2018-06-01 00:01:00 +0000
tags:
  exception-handling
  thread-local-storage
---

Paul McKenney and JF Bastien [write in P0108 "Skeleton Proposal for Thread-Local Storage (TLS)"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0108r1.html) (April 2016):

> The preferred approach is to provide alternative wrappers for the functions [...]
> Code sensitive to `errno` could assign the return value to a `status_value`
> and then extract both the `errno` and the return value.

They propose essentially this code:

    template <typename T>
    class math_result {
        math_error e;
        T val;

    public:
        explicit operator math_error() const {
            return e;
        }
        operator T() const {
            return val;
        }
    };

    extern double tgamma(double, math_error *);

    math_result<double> tgamma(double x) {
        math_result<double> ret;
        ret.val = tgamma(x, &ret.e);
        return ret;
    }

    int main() {
        double x = tgamma(3.14);  // discards errno
        auto xe = tgamma(3.14);
        printf("%d\n", int(math_error(xe)));
    }

But you know what might be even better IMHO? This code:

    template <typename T>
    class math_result {
        math_error e;
        T val;

    public:
        math_error error() const { return e; }
        T value() const { return val; }
        operator T() const {
            if (e) errno = e;
            return val;
        }
    };

    extern double tgamma(double, math_error *);

    math_result<double> tgamma(double x) {
        math_result<double> ret;
        ret.val = tgamma(x, &ret.e);
        return ret;
    }

    int main() {
        double x = tgamma(3.14);  // sets errno in a POSIX-compatible manner
        double y = tgamma(3.14).value();  // discards errno
        auto xe = tgamma(3.14);
        printf("%d\n", int(xe.error()));
    }

This `math_result::operator T` sets `errno` if you try to retrieve the value implicitly from
a result, similarly to how `std::expected::value()` throws an exception
if you try to retrieve the value from a valueless `expected`. The theme is
the same: take nice value-semantic local behavior and, for historical reasons,
when misused, turn it into icky global-side-effecting behavior.
