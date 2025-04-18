---
layout: post
title: "Most C++ constructors should be `explicit`"
date: 2023-04-08 00:01:00 +0000
tags:
  c++-style
  constructors
  initialization
  initializer-list
  library-design
excerpt: |
  All your constructors should be `explicit` by default.
  Non-`explicit` constructors are for special cases.
---

{% raw %}
> All your constructors should be `explicit` by default.
> Non-`explicit` constructors are for special cases.

The `explicit` keyword disallows "implicit conversion" from
single arguments or braced initializers. Whereas a non-`explicit`
constructor enables implicit conversion —

    struct Im {
      Im();
      Im(int);
      Im(int, int);
    };
    void read_im(const Im&);
    void test_im() {
      Im i1;
      Im i2 = Im();
      Im i3 = Im(1);
      Im i4 = Im(1, 2);
      Im i5 = {};
      Im i6 = 1;
      Im i7 = {1};
      Im i8 = {1, 2};
      read_im({});
      read_im(1);
      read_im({1});
      read_im({1, 2});
    }

— an `explicit` constructor strictly limits the number of syntaxes
by which that constructor can be invoked —

    struct Ex {
      explicit Ex();
      explicit Ex(int);
      explicit Ex(int, int);
    };
    void read_ex(const Ex&);

    void test_ex() {
      Ex e1;
      Ex e2 = Ex();
      Ex e3 = Ex(1);
      Ex e4 = Ex(1, 2);
      read_ex(Ex());
      read_ex(Ex(1));
      read_ex(Ex(1, 2));
    }

I claim that the latter is _almost always_ what you want, in production
code that needs to be read and modified by more than one person. In short,
[`explicit` is better than implicit.](https://peps.python.org/pep-0020/)


## C++ gets the defaults wrong

C++ famously "gets all the defaults wrong":

- `switch` cases fall through by default; you have to write `break`
    by hand.

- Local variables are uninitialized by default; you must write `=0` by hand.
    (In a just world, there'd be loud syntax for "this variable is uninitialized,"
    and quiet syntax for "this variable is value-initialized to zero.")

- Most [variables won't](https://www.hpcalc.org/hp48/docs/columns/aphorism.html);
    but C++ makes non-`const` the default, so that you must write `const` by hand
    in many places. (But please [not too many!](/blog/2022/01/23/dont-const-all-the-things/))

- Most classes aren't actually intended as bases for inheritance, but C++ permits
    deriving from any class, unless you write `final` by hand.

- Constructors correspond to implicit conversions by default; to get them
    to behave only like the explicit constructors in any other language, you must
    write `explicit` by hand.

- Almost all the constructors in the STL are implicit rather than `explicit`;
    for example, `{&i, &j}` implicitly converts to `vector<int>`, which means
    [so does `{"a", "b"}`](https://godbolt.org/z/TWYrqoc4x).

Most of these defaults merely grant license to do things that sane code doesn't do
anyway; arguably the wrong defaults can be ignored. Personally I don't recommend
[writing `const` on all the things](/blog/2022/01/23/dont-const-all-the-things/),
nor writing `final` on non-polymorphic classes. But I _do_ recommend explicitly initializing
scalar variables and data members, and of course you have to write `break` in the right
places. I treat the `explicit` keyword as somewhere in that latter category:
writing `explicit` on all your constructors is _at least_ as important as explicitly
initializing all your scalar variables.

I say you should write `explicit` on "all" your constructors.
What I really mean is, you should write it on 99% of your constructors. There are a handful of special
cases — literally, I can think of four — where it's correct to leave a constructor as non-explicit.


## Implicit is correct for copy and move constructors

C++ loves to make implicit copies of things. If you marked your copy constructor as
`explicit`, then simple copying wouldn't work anymore:

    A a1;
    A a2 = a1;
      // no matching constructor for initialization of `a2`

So never mark a single-argument copy or move constructor as `explicit`.
But do continue to mark your _zero-argument_ constructor
`explicit`; there's no problem with the initialization of `a1` here.


## Implicit is correct for types that behave like bags of data members

In C, the notion of "`struct` type" or "array type" is essentially identical with
"these elements, in this order." So in C, we always initialize structs and arrays with
curly braces because this kind of type — the _aggregate_ — is all we have to work with.

    struct CBook {
      const char *title;
      const char *author;
    };
    struct CBook mycbook = { "Hamlet", "Shakespeare" };

C++ not only adopted the notion of _aggregate types_ directly from C (for backward compatibility),
but also modeled its class types a little too much on C's aggregates. The archetypical C++
class is a "bag of data members":

    class Book {
      std::string title_;
      std::string author_;
    public:
      Book(std::string t, std::string a) :
        title_(std::move(t)), author_(std::move(a)) {}
      std::string title() const { return title_; }
      std::string author() const { return author_; }
    };

    int add_to_library(const Book&);

    Book mybook = { "Hamlet", "Shakespeare" };

If our intent is that a `Book` should be _identical_ with the notion of "a `title` plus an `author`,
in that order," forever, then there is absolutely nothing wrong with treating
`{"Hamlet", "Shakespeare"}` as a `Book`. That's just "uniform initialization," the
same thing `std::pair<std::string, std::string>` does.

But in the real world, "bags of data members" are surprisingly uncommon.
If you think you've found one, you'll probably be surprised in a few years
to find out that you were wrong! For example, we might eventually realize that
every `Book` also has a `pagecount`. So we change our class to

    class Book {
      std::string title_;
      std::string author_;
      int pagecount_;
    public:
      Book(std::string t, std::string a, int p) :
        title_(std::move(t)), author_(std::move(a)), pagecount_(p) {}
      // ~~~
    };

Now `Book("Hamlet", "Shakespeare")` is no longer a valid expression; we are forced to go
find all the places that explicitly construct `Book`s and update them. However, the
braced initializer `{"Hamlet", "Shakespeare"}` remains valid; it's just no longer implicitly
convertible to `Book`. It might now prefer to convert to something else. For example,
consider this overload set ([Godbolt](https://godbolt.org/z/d8T4f6dGj)):

    int add_to_library(const Book&); // #1

    template<class T = void>
    int add_to_library(std::pair<bool, const T*>); // #2

    add_to_library({"Hamlet", "Shakespeare"});

If `Book`'s implicit constructor takes two `std::string`s, then this is a call to
`add_to_library(const Book&)` with a temporary `Book`. But if `Book`'s implicit
constructor takes two `string`s and an `int`, then this is a call to
`add_to_library(pair<bool, const void*>)`. There's no ambiguity in either case as far as
the compiler is concerned. Any ambiguity you and I see in this code is ambiguity in the human sense,
inserted by the human programmer who decided that `{"Hamlet", "Shakespeare"}` was
all it took — and all it _would_ take, forever — to make a `Book`.

> Software engineering is programming integrated over time.
>
> &emsp; &emsp; —["Software Engineering at Google"](https://amzn.to/3GsmAMX) (Titus Winters, Tom Manschreck, Hyrum Wright)

So if our `Book` is _not_ just a bag of data members, like `std::pair` or `std::tuple` is,
then this particular excuse fails to apply to our `Book`. There's a good way to tell if
this excuse applies: Is your type actually _named_ `std::pair` or `std::tuple`? No?
Then it's probably not a bag of data members.

    pair(first_type f, second_type s);
      // non-explicit

On the flip side: If you're considering making a type that is _literally_ an aggregate,
you should think long and hard. The more future-proof path is always to give it a
constructor, so that later you can reorder its fields or add new fields without an API break.
(For types that don't form part of an API — internal implementation details — I don't claim
it'll matter either way.) Observe the cautionary tale of [`std::div_t`](https://en.cppreference.com/w/cpp/numeric/math/div).


## Implicit is correct for containers and sequences

The previous item boils down to "Types that behave like C structs should get implicit constructors
from their 'fields.' " This item boils down to "Types that behave like C arrays should get
implicit constructors from their 'elements.' "

Every single-argument constructor from `std::initializer_list` should be non-`explicit`.

    vector(std::initializer_list<value_type> il);
      // non-explicit

This also, arguably, explains why `std::string` and `std::string_view` have implicit constructors
from string literals. You could think of

    std::string s = "hello world";

as "implicitly converting" a Platonic string datum from one C++ type to another; but arguably
you could also think of it as "initializing" the characters of `s` with the characters of that
string literal.

    const char a[] = "hello world";  // same deal

Note that the ability to have "elements of a sequence" isn't connected with
ownership. A `string_view` can be constructed from a string, and a `span` or `initializer_list`
can be constructed from a braced initializer.

> C++20 `span` provides another cautionary tale: as of 2023, it is constructible from a
> braced initializer list, but only explicitly. So you can call
> `void f(span<const int>)` as `f(span<const int>({1,2,3}))` or `f(il)` or even `f({{1,2,3}})`,
> but not as `f({1,2,3})` directly ([Godbolt](https://godbolt.org/z/e4a5E5fGb)).
> [P2447](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2023/p2447r3.html)
> is attempting to fix that retroactively. Meanwhile, try not to reproduce `span`'s
> mistake in your own code: initializer-list constructors should always be non-`explicit`.

Types constructible from `initializer_list` should also have implicit default constructors:
a little-known quirk of C++ is that `A a = {};` will create a zero-element `initializer_list`
if it must, but it'll prefer the default constructor if there is one. So if you intend that
syntax to be well-formed, you should make sure your default constructor is either non-declared
or non-`explicit`.


## Implicit is correct for `string` and `function`

C++ types that deliberately set out to mimic other types should probably have non-`explicit`
single-argument "converting constructors" from those other types. For example, it makes sense
that `std::string` is implicitly convertible from `const char*`; that `std::function<int()>`
is implicitly convertible from `int (*)()`; and that your own `BigInt` type might be implicitly
convertible from `long long`.

Another way to think of this is: Types that represent the same Platonic domain should probably
be implicitly interconvertible. For example, it makes sense that all integer types are implicitly
interconvertible, and your `BigInt` type maybe should join them. But beware: usually when we have
more than one C++ type, it's because there's a difference we're trying to preserve. `string_view`
is convertible from `string` because they're both "strings" in the Platonic sense; but `string`
is not implicitly convertible from `string_view` because when we're writing code with `string_view`
we don't expect implicit memory allocations — it would be bad if our `string_view` quietly converted
to `string` and allocated a bunch of memory when we weren't looking. The same idea (but worse)
explains why `int*` and `unique_ptr<int*>` don't implicitly interconvert: We don't want to
quietly take ownership of, or quietly release or duplicate ownership of, a pointer.

In fact, `unique_ptr<int>` also has an invariant that `int*` doesn't: an `int*` can point anywhere,
but a `unique_ptr<int>` can only (reasonably) point to a heap allocation. Similarly, a `string`
can hold any kind of contents, but a `std::regex` can only (reasonably) hold a regular expression
— all regular expressions are strings, but not all strings are regular expressions. We wouldn't
want to write a string in the source code, and have some other part of the code quietly start
treating it like a regular expression. So,
[`regex`'s constructor from `string`](https://en.cppreference.com/w/cpp/regex/basic_regex/basic_regex)
is correctly marked `explicit`.

    void f(std::string_view);  // #1
    void f(std::regex);        // #2
    void test() {
      f("hello world"); // unambiguously #1
    }


## Explicit is correct for most everything else

The vast majority of constructors fall under "everything else."

When you write a constructor, remember that omitting `explicit` will enable implicit conversions
from whatever the constructor's parameter types are. Then remind yourself that braced initialization
should be used _only_ for "elements." In this constructor parameter list, do the parameters' values
correspond one-for-one with the "elements" of my object's value?

    struct MyRange {
      MyRange(int *first, int *last);
      MyRange(std::vector<int> initial_values);
    };

These two implicit constructors can't both be correct. The first enables implicit conversion
from `{&i, &j}`; the second enables implicit conversion from `{1, 2}`. A `MyRange` value might
be a pair of pointer values, or it might be a sequence of integer values,
but it can't possibly be both at once. Our default assumption should be that it is neither:
we should declare both of these constructors `explicit` until we see a good reason why not.


### `explicit operator bool() const`

You should never declare conversion operators (`operator T() const`) at all;
but if you must break that resolution, it'll probably be for `operator bool`. You might
think that conversion to `bool` is usually implicit, in contexts like `(s ? 1 : 2)`.
But in fact C++ defines a special "contextual conversion" just for `bool`,
making each of these cases happy to call your `explicit operator bool`:

    struct S {
        explicit operator bool() const;
    };
    S s;
    if (s)               // OK
    int i = s ? 1 : 2;   // OK
    bool b1 = s;         // Error
    bool b2 = true && s; // OK
    void f(bool); f(s);  // Error

Therefore, `operator bool` should always be `explicit`; you'll lose no "bool-like" functionality
(the "OK" lines), while preventing some unwanted implicit conversions (the "Error" lines).


## A stab at a complete guideline

* `A(const A&)` and `A(A&&)` should always be implicit.

* `A(std::initializer_list<T>)` should always be implicit.
    If you have both `A(std::initializer_list<T>)` and `A()`, then
    `A()` should also be implicit. Example: `vector`.

* Whenever `std::tuple_size_v<A>` exists, the corresponding `A(X,Y,Z)`
    should be implicit. That is, the well-formedness of `auto [x,y,z] = a`
    should imply the well-formedness of `A a = {x,y,z}`. Examples:
    `pair`, `tuple`.

* Type-erasure types intended as drop-in replacements in APIs should have
    implicit constructor templates from the types they replace.
    Examples: `string_view`, `function`, `any`.

* Every other constructor (even the zero-argument constructor!) should be
    `explicit` or have a very well-understood domain-specific reason why not.
    Example: `string(const char*)`.

* `operator bool` should always be `explicit`.
    Other `operator T`s probably need to be implicit in order to
    do their job; but you should prefer named getter methods, anyway.

In short: You're going to see the `explicit` keyword a lot, and there's nothing wrong
with that. Treat it as a keyword that means "Look out! Here comes a constructor
declaration!"

----

See also:

* ["Is your constructor an object-factory or a type-conversion?"](/blog/2018/06/21/factory-vs-conversion/) (2018-06-21)
* ["The Knightmare of Initialization in C++"](/blog/2019/02/18/knightmare-of-initialization/) (2019-02-18)
* ["How the STL uses `explicit`"](/blog/2024/06/25/most-stl-ctors-arent-explicit-but-yours-still-should-be/) (2024-06-25)
{% endraw %}
