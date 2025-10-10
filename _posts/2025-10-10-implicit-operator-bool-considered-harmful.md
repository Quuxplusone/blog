---
layout: post
title: "Implicit `operator bool` participates in comparison"
date: 2025-10-10 00:01:00 +0000
tags:
  c++-style
  compiler-diagnostics
  overload-resolution
  pitfalls
  stl-classic
  war-stories
excerpt: |
  In ["What breaks without implicit `T*`-to-`bool` conversion?"](/blog/2025/10/05/pointer-to-bool-conversion/) (2025-10-05)
  I wrote that in a proprietary C++17 codebase my audit turned up
  "one non-`explicit` `operator bool`, and zero bugs."
  On closer inspection we found that that non-explicit `operator bool` actually was
  causing a real bug! Consider this hypothetical type that behaves something like
  `unique_ptr<int>` ([Godbolt](https://godbolt.org/z/EMc96648a)):

      struct IntPtr {
        explicit IntPtr() = default;
        explicit IntPtr(int *p) : p_(p) {}
        int& operator*() const { return *p_; }
        int *operator->() const { return p_; }
        operator bool() const { return bool(p_); }
        friend bool operator==(IntPtr a, IntPtr b) { return a.p_ == b.p_; }

        int *get() const { return p_; }
      private:
        int *p_ = nullptr;
      };

      std::set<IntPtr> myset;
      int i, j;
      myset.emplace(&i);
      myset.emplace(&j);
      assert(myset.size() == 2); // fails!

  After emplacing two items, `myset.size()` is only 1!
---

In ["What breaks without implicit `T*`-to-`bool` conversion?"](/blog/2025/10/05/pointer-to-bool-conversion/) (2025-10-05)
I wrote that in a proprietary C++17 codebase my audit turned up
"one non-`explicit` `operator bool`, and zero bugs."
On closer inspection we found that that non-explicit `operator bool` actually was
causing a real bug! Consider this hypothetical type that behaves something like
`unique_ptr<int>` ([Godbolt](https://godbolt.org/z/EMc96648a)):

    struct IntPtr {
      explicit IntPtr() = default;
      explicit IntPtr(int *p) : p_(p) {}
      int& operator*() const { return *p_; }
      int *operator->() const { return p_; }
      operator bool() const { return bool(p_); }
      friend bool operator==(IntPtr a, IntPtr b) { return a.p_ == b.p_; }

      int *get() const { return p_; }
    private:
      int *p_ = nullptr;
    };

    std::set<IntPtr> myset;
    int i, j;
    myset.emplace(&i);
    myset.emplace(&j);
    assert(myset.size() == 2); // fails!

After emplacing two items, `myset.size()` is only 1!
Adding `explicit` to our `operator bool` highlights our problem:

    [...in the implementation of std::less<IntPtr>...]
    error: invalid operands to binary expression ('const IntPtr' and 'const IntPtr')
      405 |       { return __x < __y; }
          |                ~~~ ^ ~~~

In the old code, `__x < __y` had "worked" by implicitly converting both sides to `bool`
and then comparing the bools with `<`. This certainly isn't what we want!
It made all non-null `IntPtr`s compare equivalent to each other.

> I'm actually surprised that no mainstream compiler has a warning for relational comparison
> of `bool`; I bet that's invariably a bug. But even if that warning had existed, it wouldn't
> have helped us here. The relational comparison is hidden deep inside a system header, where
> warnings are suppressed by default.

This silent misbehavior was possible only because we forgot the `explicit` on `IntPtr::operator bool`.
With `explicit operator bool`, this mistake breaks the build instead.
Of course, to get the behavior we _want_ we have to do something more:
we could add an `operator<` to `IntPtr`, or switch from `set` to `unordered_set`
(specializing `std::hash<IntPtr>`), or define a custom comparator such as:

    struct IntPtrLess {
      bool operator()(IntPtr a, IntPtr b) const { return a.get() < b.get(); }
    };
    std::set<IntPtr, IntPtrLess> myset;

No matter how we resolve the issue, it remains that the original code was buggy,
and the culprit that enabled the bug was a non-explicit `operator bool`.
Every `operator bool` should be `explicit`; no exceptions!

---

See also:

* ["Most C++ constructors should be `explicit`"](/blog/2023/04/08/most-ctors-should-be-explicit/) (2023-04-08)
