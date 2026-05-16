---
layout: post
title: "Optimizing `string::append` is harder than it looks"
date: 2021-04-17 00:01:00 +0000
tags:
  exception-handling
  llvm
  rant
  standard-library-trivia
  war-stories
---

Back in early March, Lukas Böger asked on StackOverflow:
["Where do standard library or compilers leverage noexcept move semantics (other than vector growth)?"](https://stackoverflow.com/questions/66459162/where-do-standard-library-or-compilers-leverage-noexcept-move-semantics-other-t/66498981#66498981)
So I got nerdsniped into looking at all the places libc++ tests the noexceptness
of any user-defined operation. This circuitously led to my finding an absolutely _massive_
number of corner-case bugs in libc++'s `string::append` member function.

> In hindsight, it was a mistake for C++ to use the same keyword for "making a function (conditionally)
> noexcept" and "testing the noexceptness of a user-defined operation." It makes grammatical
> sense — see ["Why do we require `requires requires`?"](/blog/2019/01/15/requires-requires-is-like-noexcept-noexcept/)
> (2019-01-15) — but it is absolutely awful to grep. You can't easily grep for all places
> noexceptness is _tested_, because it ends up also finding all the places noexceptness is
> _applied_. Sifting out the false positives is an Augean task, and it's only getting worse.
>
> libc++ could solve this by consistently using a macro such
> as <nobr><code>-D_IFNOEXCEPT_(...)=<wbr>noexcept(__VA_ARGS__)</code></nobr>
> everywhere it _tests_ noexceptness, but it might be hard to train maintainers to use this
> macro properly, since libc++ already provides a syntactically indistinguishable macro
> `_NOEXCEPT_` for _applying_ conditional noexceptness. If you used the wrong macro,
> the compiler wouldn't catch you. Bit-rot would be inevitable.

`string::append` is defined quite simply in the Standard ([[string.append]/14](http://eel.is/c++draft/string.append#14)):

>     template<class InputIterator>
>     constexpr basic_string&
>       append(InputIterator first, InputIterator last);
>
> Constraints: `InputIterator` is a type that qualifies as an input iterator.
>
> Effects: Equivalent to:<br>
> `return append(basic_string(first, last, get_allocator()));`

The words "Equivalent to" are words of power: they mean that implementations are allowed to do anything
they like, but only if what they do is _observably indistinguishable from_ the sample implementation.
"Observably indistinguishable" is quite a strong constraint. In particular, notice that the standard's sample
implementation effectively mandates the [strong exception guarantee](https://en.cppreference.com/w/cpp/language/exceptions#Exception_safety).
This is by design. Many operations on `std::string` and `std::vector` enforce the strong exception
guarantee, even when similar operations on `std::list` or `std::deque` wouldn't, because
`string` and `vector` are so fundamental to "basic" C++, used so widely, and used in so much old legacy code.

But, at the same time, notice that the sample implementation is _highly pessimized._

    std::string m;
    char world[] = " world, or something long enough to defeat SSO";
    m.reserve(1000);
    m.assign("hello");
    m.append(std::begin(world), std::end(world));

Despite that `m.capacity() > m.size() + std::size(world)`, the sample implementation
will not simply blat one buffer's contents into the other; it will instead allocate a new
temporary `basic_string` object initialized with `(world.begin(), world.end(), get_allocator())`
(heap-allocation, O(n) memcpy) and then append from that temporary string into `m`
(O(n) memcpy). That's quite unfortunate!

So, a high-quality library implementation of `string::append` perhaps ought to do something
cleverer than the sample implementation in the standard.

> Alternatively, high-quality user code ought to be aware of this consideration and
> prefer to call a less generic overload of `append`. In this specific case, where `world`
> is a simple char array with no embedded null bytes, we should prefer a simple `m.append(world)`
> for readability, never mind performance.

Our candidate "clever" implementation of `string::append` is:

* Look at the length of the range we're copying from: `n = std::distance(first, last)`.

* Reserve enough spare capacity such that `size() + n <= capacity()`, using
    at most one heap-allocation. If `n` is very large, don't trigger multiple geometric resizings:
    do it all in one go.

* Now that we have enough space, copy the chars from `[first, last)` into our buffer.
    Finally, update our `size()`.

Study this algorithm for a minute or two. How many places can you find where it might
go wrong? How many corner cases do we need to consider? Remember that `first` and `last`
are arbitrary iterator types; and remember that the Standard requires us to provide
the strong exception guarantee, no matter what happens.

----

Here are all the interesting corner cases I found in libc++'s former "clever"
implementation of `string::append`. This includes some cases which the former
implementation successfully handled, and some cases where it accidentally
failed to provide the strong guarantee.

### Input iterators

    std::string s = "hello";
    auto it = std::istream_iterator<char>(std::cin);
    s.append(it, {});

The first step in our clever algorithm was "Look at the length of the range we're copying from."
If that range is an _input_ range, then iterating it is a destructive operation!
So the clever algorithm is a complete non-starter unless `first` and `last`
are at least forward iterators.

Fortunately, this is easy to handle using SFINAE and/or tag dispatch.


### Self-appending which requires reallocation

    std::string s = "barely long enough to break SSO";
    assert(s.capacity() == 31);
    s.append(&s[11], &s[18]);
    assert(s == "barely long enough to break SSO enough");

This case doesn't involve exception guarantees.
We just have to be careful in our second step: when we do
"Reserve enough spare capacity," _if_ that's going to reallocate
our buffer, then it will also invalidate any iterators that
point into our buffer — including `first` and `last`!

Okay, if `[first, last)` is a sub-range of our heap-allocated buffer,
_and_ reallocation is needed, then we just won't try the clever thing.


### Self-appending your own null terminator

    std::string s = "hello";
    s.reserve(100);
    s.append(s.data(), s.data() + 6);
    assert(s == std::string("hellohello\0", 11));

The addressable range of a `vector<char>` goes from `v.data()`
up to `v.data() + v.size()`. But strings always carry a null terminator,
so the addressable range of a `string` actually goes from `s.data()`
up to `s.data() + s.size() + 1`. (Yes,
[this is mandated.](http://eel.is/c++draft/basic.string#general-3.sentence-1))
And of course `s.data()[s.size()]` will be the first character we overwrite.

So, even if reallocation _isn't_ needed, we _still_ have to be careful
that `[first, last)` doesn't overlap with our heap-allocated buffer —
or at least that it doesn't overlap with the trailing null terminator!


### Self-appending which isn't apparent from `first`

    std::vector<std::string> v = {
        "barely long enough to break SSO",
        "barely long enough to break SSO",
        "barely long enough to break SSO"
    };
    std::string& s = v[1];
    std::vector<std::string_view> sv(v.begin(), v.end());
    auto F = [&](auto& x) -> const char& { return x[3]; };
    auto R = sv | std::views::transform(F);
    auto [first, last] = std::ranges::subrange(R);
    s.append(first, last);
    assert(s == "barely long enough to break SSOeee");

`R` is a random-access range of `char`, so you might be forgiven for
assuming that `R` overlaps `s` if and only if `*first` is in `s`.
But in fact, `*first` is _not_ in `s`; it's in `v[0]`.
Only `*(first + 1)` is in `s`!

So, figuring out whether `[first, last)` overlaps with our heap-allocated
buffer _in general_ is simply a non-starter. We can't look at an arbitrary
user-provided iterator and see whether it's going to overlap us or not.
But at least if `first` is a known-well-behaved iterator type, such as
`char*` or `std::vector<int>::const_iterator`, then we can maybe still
apply the clever algorithm.


### Iterator operations that throw exceptions

Recall our clever algorithm: First we traverse `[first, last)` to compute
its length; then we reallocate our buffer if needed; then we traverse
`[first, last)` a second time to copy its characters.

What if that second traversal throws an exception?

    int count = 10;
    struct ThrowIt {
        using value_type = char;
        using iterator_category = std::forward_iterator_tag;
        using difference_type = int;
        const char *p_;
        ThrowIt& operator++() {
            ++p_;
            if (count-- == 0) throw "oops";
            return *this;
        }
        ThrowIt operator++(int);
        const char& operator*() const { return *p_; }
        friend bool operator==(ThrowIt, ThrowIt) = default;
    };

    std::string s = "barely long enough to break SSO";
    const char t[] = "another";
    ThrowIt first = ThrowIt{t};
    ThrowIt last = ThrowIt{t+7};
    const char *p = &s[0];
    try {
        s.append(first, last);
    } catch (...) {
        assert(*p == 'b');
    }

In this example, we create a custom forward-iterator type `ThrowIt`
with an `operator++` that throws on the tenth call. Seven of those
calls will be eaten up by our clever algorithm's `std::distance`;
then we'll reallocate `s`'s buffer; then we'll start copying characters
and — oh no! — it'll throw an exception after copying the third character.

Our clever algorithm can detect that exception and restore the null terminator
as if nothing ever happened... but we can't undo the fact that we reallocated
our buffer. Reallocating the buffer invalidates iterators and pointers, such
as `p`. The dereference of `*p` in our catch-block now has undefined behavior.
That's not very strong-exception-guarantee of us!

So, we can't apply our clever algorithm when the iterator's operations might
throw.

However, the previous section already established that we can't
apply our clever algorithm for _any_ user-defined iterator type. So
the whole "iterator operations might throw" thing is a bit of a red herring.

> Recall that I started digging into this topic because libc++ had some
> really convoluted uses of `noexcept` in this area. Well, it was trying
> to detect this specific situation. And, as we've now seen, it didn't
> need to detect it, because the clever algorithm is _never_ allowed for
> user-defined iterator types.


### Conversion operators that throw exceptions

    struct Char { int ch; operator wchar_t() const; };

    std::wstring s = L"hello ";
    const Char ch[] = {'w', 'o', 'r', 'l', 'd'};
    const wchar_t *p = &s[0];
    s.append(ch, ch+5);

Here we are appending from an array of `Char`. `first` is simply `const Char*` —
a raw pointer type. We know the iterator operations won't throw exceptions.
We know the input range won't overlap our heap-allocated buffer.
(Technically, `char` can alias anything — but `wchar_t` can't! Ha ha!)

So this seems like a pretty rock-solid scenario where our clever algorithm
will be okay, right?...

    Char::operator wchar_t() const { throw "Nope!"; }

Our clever algorithm computes `std::distance(ch, ch+5)`, reallocates `s`'s
buffer, and then goes to copy the characters. The very first evaluation of
_some-lvalue_ `= *first;` throws an exception! As in the previous section, we
can't provide the strong guarantee if we've already reallocated our buffer.

So, even when `first` and `last` are known-well-behaved raw pointer types,
we still have to look at their `value_type`, and if _it_ might be user-defined,
then we mustn't do our clever algorithm.

"Well, maybe we can still do the clever algorithm if we observe that the
expression `declval<char&>() = *first` is noexcept!"  ...Nope.


### Self-appending via conversion operators

This one has the same structure as "Self-appending which isn't apparent from `first`."
([Godbolt.](https://godbolt.org/z/xash5YM9M))

    std::wstring g = L"hello";

    struct Evil {
        operator wchar_t() const noexcept {
            return L'!' + g[5];
        }
    };

    const Evil ch[2];
    g.reserve(100);
    g.append(ch, ch+2);
    assert(g == L"hello!!");

Notice that:

* We're calling `g.append(first, last)` with iterators of type `const Evil*`.

* `is_nothrow_convertible_v<Evil, wchar_t>`.

* In fact, `is_trivial_v<Evil>`, for whatever that's worth!

* `g.capacity()` is plenty big enough to append two more chars without reallocation.

And yet we _still_ can't use our clever algorithm! Because if we do, then we'll
copy characters from `[ch, ch+2)` directly into `g[5]` and `g[6]` — and we'll wind
up with a string containing `L"hello!B"`, because overwriting `g[5]` changes the
computed "value" of `ch[1]`.

So looking at the noexceptness of the conversion operation buys us exactly nothing.


## So, can we _ever_ do the clever algorithm?

I believe that whenever `first` and `last` are raw pointers of the form `T*`,
where `T` is arithmetic (and therefore not a class or enum type), then the clever
algorithm is safe _as long as_ `first` doesn't point to any character of the
string. (Which we can find out — not in theory, but in practice — with a couple of pointer
comparisons.)  So this is what libc++ will implement from now on,
as soon as [D98573](https://reviews.llvm.org/D98573) lands.

The very first snippet we looked at—

    std::string m;
    char world[] = " world, or something long enough to defeat SSO";
    m.reserve(1000);
    m.assign("hello");
    m.append(std::begin(world), std::end(world));

—will still trigger the clever algorithm, because `first` is of pointer-to-arithmetic type
`char*` and doesn't overlap `[m.data(), m.data() + m.size()]`.


### Er, actually...

Remember how `char` can alias anything?

    std::string s = "hello world";
    char *first = (char*)&s;
    char *last = (char*)(&s + 1);
    s.append(first, last);

Even after D98573 lands, libc++ will still be non-conforming for inputs along these lines,
because modifying the string's size or capacity will affect the bytes being copied from
`[first, last)` in this example. To deal with this, we could:

* add a second "overlap" check to see if `[first, last)` overlaps with `[this, this+1)`;

* stop _ever_ trying to do the clever algorithm; or

* just accept that libc++ is non-conforming on this pathological input.

I believe GNU libstdc++ has (deliberately or accidentally?) gone with that third option.
It was surprisingly hard to convince libstdc++ to misbehave, but I _think_ I finally
succeeded with [this test case](https://godbolt.org/z/131csnPP1). Strangely, it reproduces
only at `-O2`, not `-O1`. I haven't figured out why.

_Anyway._

----

The first moral of the story is:

> Checking noexceptness is always a code smell.

Don't _ever_ think that "checking the noexceptness of a user-provided operation"
will allow you to use some cleverer algorithm. Show me a function template that switches
behavior on noexceptness, and I will show you a function template with bugs in it.

Also, remember [what Kernighan said about cleverness](https://en.wikiquote.org/wiki/Brian_Kernighan),
one of many verbal gems in [my favorite programming book ever](https://amzn.to/2X5W1H0):

> Everyone knows that debugging is twice as hard as writing a program in the first place.
> So if you're as clever as you can be when you write it, how will you ever debug it?

----

See also:

* ["Attribute `noexcept_verify`"](/blog/2018/06/12/attribute-noexcept-verify/) (2018-06-12)

* ["'Exception Handling: A False Sense of Security'"](/blog/2019/06/17/tom-cargill-on-exception-handling/) (2019-06-17)
