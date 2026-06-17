---
layout: post
title: "Bit-vector manipulations in standard C++"
date: 2022-11-05 00:01:00 +0000
tags:
  benchmarks
  implementation-divergence
  llvm
  proxy-references
  standard-library-trivia
  stl-classic
---

Recently on the std-proposals mailing list, Madhur Chauhan [proposed](https://lists.isocpp.org/std-proposals/2022/10/4916.php)
that C++ needs a better way to find the first set bit in an arbitrary-length bit-string.
Today, if you want to manipulate bit-strings of length `N`, you have these options:

* For `N <= 64`, use `unsigned long long`.
* On some platforms, for `N <= 128`, use `__uint128`.
* On Clang 13, use `_ExtInt(N)`. (Clang 14 drops support for `N > 128`, which removes any reason to use `_ExtInt` as far as I know.)
* Use something like my [`Wider<T>`](/blog/2020/02/13/wide-integer-proof-of-concept/), which is statically sized and stack-allocated.
* Use `std::bitset<N>`, which is statically sized and stack-allocated.
* Use `std::vector<bool>`, which is dynamically resizeable and heap-allocated.
* Use [`boost::dynamic_bitset<>`](https://www.boost.org/doc/libs/1_80_0/libs/dynamic_bitset/dynamic_bitset.html), which is dynamically resizeable and heap-allocated.

Suppose we have a bit-string stored in one of these ways, and we want to
find the second-lowest 1-bit.

> Conventionally, the lowest-order bit will be
> `(x & 1)` for integer-like types which are written high-to-low, but
> `x[0]` for sequence-like types which are written low-to-high. So
> the operation that for`__uint128` and `std::bitset<N>` we spell
> `>>` ("right-shift") is, for `vector<bool>`, spelled
[`std::shift_left`](https://en.cppreference.com/w/cpp/algorithm/shift).

We'll generalize the problem by defining a function `f(n, x)` that finds the lowest
1-bit in `x` whose index is greater than or equal to the given index `n`, or returns
some large value if no such 1-bit exists.
The original problem, of finding the index of the second-lowest 1-bit in `x`, becomes:

    y = f(f(0, x) + 1, x);
    if (y < N) ~~~

Here are the current state-of-the-art methods for implementing `f`, as far as I know.
Notice that I'll use `int` for bit indices, since 2147483647 bits
[ought to be enough for anyone](https://quoteinvestigator.com/2011/09/08/640k-enough/).
[Godbolt:](https://godbolt.org/z/qzvsdYx5E)

    int f(int n, unsigned long long x) {
        return n >= N ? N : std::countr_zero(x >> n) + n;
    }

    #ifndef _MSC_VER
    static int f(int n, __uint128_t x) {
        constexpr unsigned long long mask = ~0uLL;
        return (x == 0) ? 128 :
               (n < 64 && (x & mask) != 0) ?
                   std::countr_zero((unsigned long long)(x >> n)) + n :
               f(n - 64, x >> 64) + 64;
    }
    #endif

    int f(int n, const Wider<T>& x) {
        Wider<T> mask = Wider<T>(1) << n;
        for (int i = n; i < N; ++i, mask <<= 1) {
            if (x & mask) return i;
        }
        return N;
    }

    int f(int n, const std::bitset<N>& x) {
    #ifdef __GLIBCXX__
        return (n == 0) ? x._Find_first() : x._Find_next(n-1);
    #else
        for (int i = n; i < N; ++i) {
            if (x[i]) return i;
        }
        return N;
    #endif
    }

    static int f(int n, const std::vector<bool>& x) {
        if (n >= N) return N;
        return std::find(x.begin() + n, x.end(), true) - x.begin();
    }

    static int f(int n, const boost::dynamic_bitset<>& x) {
        if (n >= N) return N;
        return (n == 0) ? x.find_first() : x.find_next(n-1);
    }

For `unsigned long long`, we can't _just_ call `std::countr_zero(x >> n)`
because `n` might be out of range, making `x >> n` undefined behavior.

libc++ provides `std::countr_zero` for `__uint128_t`, but GNU libstdc++ does not,
and MSVC doesn't support `__uint128_t` at all. See also
["Is `__int128` integral?"](/blog/2019/02/28/is-int128-integral/) (2019-02-28).

GNU libstdc++ provides implementation-detail methods `_Find_first` and `_Find_next`
on `std::bitset`, with exactly the same semantics as the public `find_first`
and `find_next` methods on `boost::dynamic_bitset`. Oddly, libstdc++ does not
provide those methods for their `vector<bool>`, even though the two types' elements
are laid out in the same way.

## STL algorithms for bit iterators

libc++ provides clever specializations of certain STL algorithms for bit iterators
specifically, so that they can exploit full-word-length instructions instead of
extracting "elements" bit by bit. For example, [`std::find` can use `rep bsfq`](https://godbolt.org/z/dn5PGM6T9),
and [`std::count` can use `popcntq`](https://godbolt.org/z/zx6nrfb8o).
The full list of algorithms that libc++ optimizes is:
[`std::find`](https://github.com/llvm/llvm-project/blob/ed2d364/libcxx/include/__bit_reference#L253-L261),
[`count`](https://github.com/llvm/llvm-project/blob/ed2d364/libcxx/include/__bit_reference#L327-L335),
[`fill`, `fill_n`](https://github.com/llvm/llvm-project/blob/ed2d364/libcxx/include/__bit_reference#L400-L422),
[`copy`](https://github.com/llvm/llvm-project/blob/ed2d364/libcxx/include/__bit_reference#L551-L559),
[`copy_backward`](https://github.com/llvm/llvm-project/blob/ed2d364/libcxx/include/__bit_reference#L696-L704),
[`move`, `move_backward`](https://github.com/llvm/llvm-project/blob/ed2d364/libcxx/include/__bit_reference#L708-L724),
[`swap_ranges`](https://github.com/llvm/llvm-project/blob/ed2d364/libcxx/include/__bit_reference#L878-L887),
[`rotate`](https://github.com/llvm/llvm-project/blob/ed2d364/libcxx/include/__bit_reference#L923-L970), and
[`equal`](https://github.com/llvm/llvm-project/blob/ed2d364/libcxx/include/__bit_reference#L1099-L1107).
Oddly, as of this writing libc++ does not optimize `std::mismatch` for bit iterators, even though
`mismatch` can be considered a building block of `equal`. Maybe a `mismatch` optimization will be added
along the way to implementing `vector<bool>::operator<=>`.
Even more oddly, libc++ fails to optimize
`copy_n`, so in Clang 15 `std::copy(first, first+n, dest)` [is 1000x faster](https://godbolt.org/z/d57e763bx)
than `std::copy_n(first, n, dest)` when `first` is a bit iterator.
I've submitted a patch for the issue and expect it'll be fixed soon.

> You might expect libraries also to specialize `std::all_of`, `any_of`, and `none_of`.
> The problem with those algorithms is that they don't default their `Predicate` argument:
> you must provide a predicate, making `any_of` more like `count_if` than like `count`.
> Thus, on libc++, `std::count(v.begin(), v.end(), false) == 0` will run much faster than
> `std::ranges::all_of(v, std::identity())`.

Microsoft STL optimizes `fill`, `fill_n`, `find`, and `count` for bit iterators.
(Search [the code](https://github.com/microsoft/STL/blob/705265e007283870b648856579c99f3fbc748593/stl/inc/xutility)
for uses of `_Is_vb_iterator`.)

libstdc++ optimizes only [`std::fill`](https://github.com/gcc-mirror/gcc/blob/72886fc/libstdc%2B%2B-v3/include/bits/stl_bvector.h#L1562-L1580)
for bit iterators.

----

From the vantage point of 2022, you might think that `std::bitset<N>` is to `std::vector<bool>`
in the same way that `std::array<T, N>` is to `std::vector<T>`. That is, it uses stack storage
with a fixed size instead of dynamic storage, but ought to provide basically the same iterator API
otherwise. Sadly, this is not the case!

`std::bitset` is one of those "not quite really STL" types, like `std::string` and `std::valarray`,
that tries to fit a couple of use-cases but none of them are "STL iterable container." Arguably, the
hint is in its name: whereas `vector<bool>` is clearly a _vector_, a sequence container, of boolean values,
`bitset` is a _set_, a collection of small integer indices. `bs.test(42)` is the equivalent of `s.find(42)`;
`bs.set(42)` is the equivalent of `s.insert(42)`; and so on. (As usual, these named methods throw
`std::out_of_range` on error, and `bitset` provides `operator[]` for faster unchecked access.)

If you were going to iterate over the "contents" of such an object, what would you expect to see?

    std::bitset<1000> bs;
    bs.set(42);
    for (auto elt : bs) {
        std::cout << bs;
    }

Surely you wouldn't expect to see "false, false, false, false, ..."!
So `std::bitset` (and `boost::dynamic_bitset` too) simply don't provide iterators at all.
Instead, `dynamic_bitset` provides `find_first` and `find_next`, and libstdc++'s `bitset`
provides secret `_Uglified` versions of the same methods.

Like a Python set, `bitset` provides overloaded `&`, `|`, `^`, `~` that perform intersection,
union, symmetric-difference, and invert operations. But then, as if the type author were free-associating
on the other meanings of those operators, it goes on to provide `<<` and `>>`. Which makes no sense at all
if you're thinking of `bitset` either as a sequence container of bits (like `vector<bool>`) or a set of
indices (as we were just doing in the previous paragraph), but perfect sense if you're thinking of
it as an integer type.

`std::bitset<N>` ends up behaving similarly to an integer type with `N` bits... except that it
can't do the full suite of arithmetic operations out of the box. You can still implement those operations
tediously by hand. (Thanks to Maciej Hehl [on StackOverflow](https://stackoverflow.com/a/4068918/1424877) for this code.
Note that adding operators to a type we don't own, such as `bitset`, is very ill-advised in practice;
we'd do better to name this function `plus`, or wrap the `bitset`s in a class of our own devising.)

    template<size_t N>
    auto operator+(const std::bitset<N>& a, const std::bitset<N>& b) {
        auto carry = a & b;
        auto result = a ^ b;
        while (carry.any()) {
            auto shifted = carry << 1;
            carry = result & shifted;
            result ^= shifted;
        }
        return result;
    }

> For a move-semantic type like `string` or `set<int>`, we'd want to pass `a` by
> value and accumulate the result in `a` instead of making a whole new copy.
> But for giant trivial stack-storage types like `bitset` and `array`, move buys
> us nothing and [NRVO](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#rvo-nrvo-urvo)
> buys us much. So, for these types, we pass by reference and return in ways that can be NRVO'ed.

## Conclusion

There's not much to conclude here, except that C++ currently has a bunch of ways
to do bit-strings, and none of them are particularly well crafted.
`vector<bool>` is iterable but not arithmetic'able; `bitset<N>` is arithmetic'able
but fails to be iterable or even resizable; different library vendors provide vastly
different performance profiles for STL algorithms on bit iterators.

Madhur and I produced a benchmark ([backup](/blog/code/2022-11-05-bitset-benchmark.cpp)) comparing all these different ways of
"find-next'ing" in a bit-string of a million elements. [Here](https://godbolt.org/z/z1adndrY6)
are the results on libc++; you can see that libc++'s `std::find` is a huge winner.

    ----------------------------------------------------
    Benchmark               Time        CPU   Iterations
    ----------------------------------------------------
    VectorBoolManual     1862 ns    1862 ns       100000
    VectorBoolStdFind      69 ns      44 ns     20286454
    BitsetManual         7300 ns    2357 ns       100000
    DynBitsetManual      6864 ns    2876 ns       252099
    DynBitsetFindnext      73 ns      41 ns     15469004


[Here](https://godbolt.org/z/r95rnYMoG)
are the results on libstdc++; note that libstdc++'s `std::find` is a huge loser,
even compared to manual iteration:

    ----------------------------------------------------
    Benchmark               Time        CPU   Iterations
    ----------------------------------------------------
    VectorBoolManual     2017 ns    2016 ns       100000
    VectorBoolStdFind    8323 ns    3479 ns       100000
    BitsetManual         3532 ns    3481 ns       100000
    BitsetFindnext         57 ns      30 ns     18880905
    DynBitsetManual      3611 ns    2099 ns       331986
    DynBitsetFindnext      47 ns      30 ns     29609779

These benchmark numbers were prooduced on VMs at godbolt.org, so they should be
taken with a grain of salt especially when comparing across the two benchmark
runs. But no amount of salt will erase a 100x speedup or slowdown.
