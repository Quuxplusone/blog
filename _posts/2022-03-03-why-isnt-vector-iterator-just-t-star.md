---
layout: post
title: "What if `vector<T>::iterator` were just `T*`?"
date: 2022-03-03 00:01:00 +0000
tags:
  argument-dependent-lookup
  implementation-divergence
  library-design
  llvm
  stl-classic
---

Every major C++ standard library vendor defines wrappers for their
contiguous iterator types — `vector<T>::iterator` and so on. You might
think that they should just define these iterators as aliases for `T*`
instead.

| Type   | libstdc++    | libc++  | MSVC |
|--------|--------------|---------|------|
| `initializer_list<T>::iterator`<br>([mandated](https://eel.is/c++draft/initializer.list.syn)) | `const T*` | `const T*` | `const T*` |
| `array<T, 10>::iterator`          | `T*` | `T*` | `std::_Array_iterator<int,10>` |
| `span<T>::iterator`               | `__gnu_cxx::__normal_iterator<int*, std::span<int>>` | `std::__wrap_iter<int*>` | `std::_Span_iterator<int>` |
| `string::iterator`                | `__gnu_cxx::__normal_iterator<char*, std::string>` | `std::__wrap_iter<char*>` | `std::_String_iterator<std::_String_val<std::_Simple_types<char>>>` |
| `string_view::iterator`           | `const char*` | `const char*` | `std::_String_view_iterator<std::char_traits<char>>` |
| `ranges::iterator_t<valarray<T>>` | `T*` | `T*` | `T*` |
| `vector<T>::iterator`             | `__gnu_cxx::__normal_iterator<T*, std::vector<T>>` | `std::__wrap_iter<T*>` | `std::_Vector_iterator<std::_Vector_val<std::_Simple_types<T>>>` |
{:.smaller}

All these wrappers impose a pretty high cost in terms of compilation speed,
debug performance, and even object-file size (because of all those super-long
mangled names). Could a library vendor decide to simply _change_ their definition of
`vector<T>::iterator` to `T*`?

Nothing prevents a green-field STL implementation from using `T*` for all these types
(as far as I know), but you'll never see any _existing_ library vendor switch to `T*`
by default: it would break too much existing code. Besides the obvious ABI breakage,
it would actually be an API break as well! Here's a list of code snippets that would
change their behavior if `vector<T>::iterator` became an alias for `T*`.
([Godbolt.](https://godbolt.org/z/n53fvfnaz))


## Implicit conversion from `T*`

    void overload_resolution() {
        extern void f(std::vector<int>::iterator);
        extern void f(const char *);
        f(nullptr);
    }

The null pointer constant `nullptr` of course can be implicitly converted to `T*`;
but there's no reason it should be implicitly convertible to `vector<T>::iterator`.
libc++'s iterator wrapper makes that conversion private; libstdc++ makes it public
but `explicit`.

If `vector<int>::iterator` were an alias for `int*`, then the call to `f` above
would become ambiguous. Today, in practice, every library vendor considers it unambiguous.


## Mixing iterators from different kinds of containers

    void container_conversion() {
        std::vector<char>::iterator vi;
        std::string::iterator si;
        std::swap(vi, si);
    }

If both `vector<char>::iterator` and `string::iterator` were aliases for `char*`,
then this would compile. In fact, it compiles on libc++ today, because libc++
makes both iterator types an alias for `__wrap_iter<char*>`; but it won't compile
on libstdc++ or MSVC because their aliases are less [SCARY](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#scary-iterators)
(and thus more expensive) than libc++'s.


## Derived-to-base conversions

    struct Base {};
    struct Derived : Base {};

    void derived_to_base_conversion() {
        std::vector<Derived>::iterator id;
        std::vector<Base>::iterator ib;
        ib = id;
    }

`Derived*` is implicitly convertible to `Base*`; but there's no
reason to permit conversion of iterator-to-`Derived` to iterator-to-`Base`.
libstdc++'s iterator wrapper disallows such conversions.


## Const-qualification conversions

    void const_contravariance() {
        std::vector<Base>::iterator p;
        const std::vector<Base>::const_iterator *pp;
        pp = &p;
    }

`Base*` and `const Base*` are [similar types](https://eel.is/c++draft/conv.qual),
but `__wrap_iter<Base*>` and `__wrap_iter<const Base*>` are not similar.
No iterator wrapper can allow this kind of pointer conversion, because it
involves only primitive types.


## Built-in types don't trigger as much ADL

    template<class T> struct Holder { T t; };

    void avoid_adl() {
        struct Incomplete;
        std::vector<Holder<Incomplete>*>::iterator i;
        (void)(i == i);
    }

See ["ADL can interfere even with uglified names"](/blog/2019/09/26/uglification-doesnt-stop-adl/) (2019-09-26)
for details on the `Holder<Incomplete>` pattern. For our purposes here,
suffice it to say that equality-comparing two pointers of type `Holder<Incomplete>**`
won't trigger the completion of `Holder<Incomplete>`; but comparing two
objects of the class type `__wrap_iter<Holder<Incomplete>**>` _will_ trigger
the completion of `Holder<Incomplete>`.

This could be altered by ADL-proofing `vector<T>::iterator`, that is,
by defining it as `__wrap_iter<T*>::type`. See
["How `hana::type<T>` disables ADL"](/blog/2019/04/09/adl-insanity-round-2/) (2019-04-09),
and my recent LWG paper [P2538 "ADL-proof `std::projected`"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2538r0.html)
for details. But of course that would be both an ABI and API break. For example,
it would alter the ADL behavior of the following snippet:

    struct A {
        // Warning: Don't try this at home!
        // iter_swap customizations rightly belong on the
        // iterator type, not on the element type!
        friend void iter_swap(std::vector<A>::iterator, std::vector<A>::iterator);
    };

    void respect_adl() {
        std::vector<A>::iterator i;
        iter_swap(i, i); // A's friend, or std::iter_swap?
    }

If `vector<A>::iterator` is `A*`, this calls the only candidate:
`A`'s hidden-friend `iter_swap`. If `vector<A>::iterator` is `std::__wrap_iter<A*>`,
it'll also consider `std::iter_swap` as a candidate, but `A`'s hidden friend will be
the better match. But, if `vector<A>::iterator` is `std::__wrap_iter<A*>::type`,
then `std::iter_swap` will be the only candidate!


## Modifiable prvalues

Hat tip to Ben Craig for this addition:

    void modify_prvalue() {
        std::vector<int> v = {1,2,3};
        auto it = --v.end();
    }

If `vector<int>::iterator` is a class type, then this code is valid C++;
`it` is initialized with the result of `v.end().operator--()`.
But if it's `int*`, then `--v.end()` is ill-formed: the built-in `--`
operator requires a modifiable lvalue, and `v.end()` would be a prvalue!

---

See also:

* ["True names matter in C++"](/blog/2025/08/01/true-names/) (2025-08-01)
