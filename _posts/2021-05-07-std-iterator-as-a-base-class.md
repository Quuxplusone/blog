---
layout: post
title: "Removing an empty base class can break ABI"
date: 2021-05-07 00:01:00 +0000
tags:
  abi
  cppnow
  llvm
  standard-library-trivia
---

Earlier this week Eric Fiselier pointed out something that came as an unpleasant surprise
(to me, at least): Even though C++11 deprecated inheritance-from-`std::unary_function`,
and C++14 deprecated inheritance-from-`std::iterator`, the actual _library vendors_
cannot remove those deprecated relationships from their libraries without taking an ABI break!

This is eerily apropos to Bryce Adelstein Lelbach's [closing keynote](https://cppnow2021.sched.com/event/ixTG)
at C++Now 2021, in which ABI breakage was a major theme.


## Background

C++98 didn't have `auto`, so it was useful for "function objects" like `std::plus<T>`
to expose a member typedef `result_type`, so that you could write nice generic code like

    template<class F, class T>
    typename F::result_type
    apply_over(F f, T x, T y, T z) {
        return f(f(x,y),z);
    }

Also `first_argument_type` and `second_argument_type`. Writing these three member typedefs over
and over was a bit tedious, so the C++98 STL provided a base class template
[`std::binary_function`](https://en.cppreference.com/w/cpp/utility/functional/binary_function)
and mandated that `plus` (and `less` and so on) should inherit from that.
[N1905 [lib.base]](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2005/n1905.pdf) was literally
this simple:

    template<class Arg1, class Arg2, class Result>
    struct binary_function {
        typedef Arg1 first_argument_type;
        typedef Arg2 second_argument_type;
        typedef Result result_type;
    };

    template<class T>
    struct plus : binary_function<T,T,T> {
        T operator()(const T& x, const T& y) const
            { return x + y; }
    };

The same pattern occurs with the standard iterator model. Every iterator, if you want it
to work with `std::iterator_traits`, must provide five member typedefs: `value_type`, `difference_type`,
`pointer`, `reference`, and `iterator_category`. (C++20 finally eliminates the requirement
to provide `pointer` and `reference`!) Writing these five lines was a bit tedious, so C++98
provided a base class template [`std::iterator`](https://en.cppreference.com/w/cpp/iterator/iterator)
and mandated that the standard iterator adaptors should inherit from it.

    template<class Category, class T, class Distance = ptrdiff_t,
        class Pointer = T*, class Reference = T&>
    struct iterator {
        typedef T value_type;
        typedef Distance difference_type;
        typedef Pointer pointer;
        typedef Reference reference;
        typedef Category iterator_category;
    };

    template<class Iterator>
    class reverse_iterator : public iterator<
        typename iterator_traits<Iterator>::iterator_category,
        typename iterator_traits<Iterator>::value_type,
        typename iterator_traits<Iterator>::difference_type,
        typename iterator_traits<Iterator>::pointer,
        typename iterator_traits<Iterator>::reference>
    {
    protected:
        Iterator current;
    public:
        reverse_iterator();
        // and so on
    };

Notice in passing that `reverse_iterator` suffers from the same addiction
to `protected` members that plagues `std::queue` and `std::insert_iterator`.

Anyway, that was the situation in C++98.

C++11 gave us `auto` and `decltype`, and also lambdas. Giving
a bunch of member typedefs to every function object suddenly seemed like
a dumb idea. So C++11 deprecated the `std::unary_function` and `std::binary_function`
base class templates (and also introduced `std::function`, which was
_not_ a base class), and removed them as base classes of `std::plus` et al.

Notice that C++11 did _not_ say that `std::plus` must _not_
inherit from `binary_function`! As far as I know, standard types may inherit
from whatever types they want. Heck, `std::vector<int>` could inherit from `std::regex`
if it wanted to.

So, vendors continued to make `plus` inherit from `binary_function`, since it
was mandated in C++98 mode and not actively harmful in C++11 mode.

C++11 preserved `std::iterator` as-is, since nothing was particularly
changing in that area.

A very late-breaking issue, [LWG2438](https://cplusplus.github.io/LWG/issue2438),
modified C++14 to remove `iterator` as a base class of `reverse_iterator` et al;
but did not actually deprecate `iterator`.

Then in C++17 we _almost_ got Concepts, which made it conspicuously awkward that `std::iterator`
was sitting on a great library name. And Ranges was starting to refactor which
of those typedefs you even needed. So
[P0174 "Deprecating Vestigial Library Parts in C++17"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2016/p0174r2.html#2.1)
deprecated `iterator` — but unfortunately not speedily enough for Concepts, which is how we
got stuck with the sesquipedalian [`std::input_or_output_iterator`](https://en.cppreference.com/w/cpp/iterator/input_or_output_iterator)
in C++20.

Meanwhile, [P0090 "Removing `result_type`, etc."](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2015/p0090r0.htm)
deprecated `std::plus`'s `result_type`, `first_argument_type`, and `second_argument_type`
member typedefs for C++17, and
[P0619 "Reviewing Deprecated Facilities of C++17 for C++20"](http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0619r4.html)
formally removed them in C++20.
(Of course vendors are _allowed_ to provide those typedefs even in C++20.)

So the situation in C++20, as I understand it, is:

- `reverse_iterator` must provide those five member typedefs somehow, but not necessarily by inheriting from `iterator`.

- `plus` needn't provide those three member typedefs at all, let alone by inheriting from `binary_function`. (But it might.)


## Now for the ABI break

Pretend we're a standard library vendor. Our existing implementation looks like this (simplified):

    template<class Iterator>
    class reverse_iterator : public std::iterator< ~~~ > {
        Iterator current;
    };

    template<class T>
    class vector {
        struct iterator { T *ptr_; ~~~ };
        using reverse_iterator = std::reverse_iterator<iterator>;
        reverse_iterator rbegin();
    };

Consider some user code like

    std::vector<int> v;
    auto it = std::make_reverse_iterator(v.rbegin());

`it` is now a variable of type `reverse_iterator<reverse_iterator<vector<int>::iterator>>`.

> You might think reversing a `reverse_iterator` should just unwrap it. Maybe. But
> weird special cases are hard to reason about. If `make_reverse_iterator(t)` didn't always
> return a `reverse_iterator<T>`, you _know_ I'd be blogging about some weird pitfall
> caused by that!

The class layout of `decltype(it)` is:

- Empty base class `iterator<random_access_iterator_tag, T, ptrdiff_t, T*, T&>`

- Member `current`, which is of type `vector<int>::reverse_iterator`, i.e.
    `std::reverse_iterator<std::vector<int>::iterator>`

The class layout of `decltype(it.current)` is:

- Empty base class `iterator<random_access_iterator_tag, T, ptrdiff_t, T*, T&>` (again!)

- Member `current`, which is of type `vector<int>::iterator` and occupies `sizeof(T*)` bytes

So `it` contains two empty base classes, _both_ of type
`iterator<random_access_iterator_tag, T, ptrdiff_t, T*, T&>`. In C++, base-class subobjects
are distinct objects in their own right, and two objects of the same type cannot occupy the same
address; so the [EBO](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#ebo-ebco) applies to
only one of them. (Jonathan Wakely has called this the "empty-base exclusion principle.")
Our reversed reverse-iterator object ends up occupying 16 bytes, not just 8!
([Godbolt](https://godbolt.org/z/7a946Tx6K) showing the behavior on libstdc++.)

> If you try this on libc++, you'll find that the original `reverse_iterator` occupies 16 bytes
> already, because libc++'s `reverse_iterator` wraps a pair of iterators instead of just one.
> [LWG2360](https://cplusplus.github.io/LWG/issue2360) is related; and sadly the extra pointer
> can't be dropped without... say it with me... breaking ABI. Yes, [this is insane.](https://godbolt.org/z/4PcMzPrb9)

So, on a sane library (not you libc++) that supports C++11, `sizeof(it)` is 16 bytes.
Now let's pretend that the library vendor removes that `std::iterator` base class,
as C++14 permits.

    template<class Iterator>
    class reverse_iterator
    #if __cplusplus < 201402L
        : public std::iterator< ~~~ >
    #endif
    {
        Iterator current;
    };

Well, _now_ the class layout of `decltype(it)` doesn't have those empty base classes
anymore! So `sizeof(it)` drops to 8 bytes. And _that_ changes the size of `Widget`,
and therefore the calling convention of `make_widget` in
([Godbolt](https://godbolt.org/z/n499TnGab)):

    struct Widget {
        std::reverse_iterator<std::reverse_iterator<int*>> rr;
        bool b;
    };
    Widget make_widget() {
        static_assert(std::is_trivially_copyable_v<Widget>);
        return Widget{ {}, false };
          // A trivially copyable 24-byte type is returned on the stack.
          // A trivially copyable 16-byte type is returned in registers, instead.
    }

Any C++11 caller linking against a C++14 `make_widget`, or vice versa, will expect
the `Widget` result in the wrong place and occupying the wrong number of bytes.

Conclusion: For a library vendor, removing the deprecated empty base class from
`reverse_iterator` counts an ABI break.

----

UPDATE, 2021-05-08: It's been brought to my attention that the C++20 Ranges library
starts the whole damn cycle over again! Ranges provides a convenience base class

    struct view_base {};

which isn't even a template at all — it's literally no more than a tag type —
but then the CRTP base class [`view_interface`](https://en.cppreference.com/w/cpp/ranges/view_interface)
inherits from that, and then _every range adaptor_
inherits from some specialization of `view_interface`!

    template<class Crtp>
    struct view_interface : view_base { ~~~ };

    template<class V>
    class reverse_view : public view_interface<reverse_view<V>> { ~~~ };

The result is that you _would_ get the same bloated class layout from `reverse_view`
as you get from `reverse_iterator`... except that Ranges
actually does the "optimization" where reversing a `reverse_view`
produces the original view again! That is, `std::views::reverse` is overloaded
so that when its argument `x` is a `reverse_view`, the result of `std::views::reverse(x)`
is just `x.base()`. ([Godbolt.](https://godbolt.org/z/nsWq1dMPq))

If Ranges is still around 10 years from now, I predict at least a couple papers
deprecating inheritance from `view_base`.
