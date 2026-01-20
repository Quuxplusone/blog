---
layout: post
title: "Prefer core-language features over library facilities"
date: 2022-10-16 00:01:00 +0000
tags:
  c++-learner-track
  c++-style
  concepts
  coroutines
  rant
excerpt: |
  There are many places in C++ where the working programmer has a choice between
  doing something via a standard-library facility, or via a similar (often newer) core-language
  feature. I keep expanding my mental list of these, so let's write them all down in
  one place.
---

{% raw %}
There are many places in C++ where the working programmer has a choice between
doing something via a standard-library facility, or via a similar (often newer) core-language
feature. I keep expanding my mental list of these, so let's write them all down in
one place.

First, the caveats: This post's title is expressed as advice to _working programmers_,
not _language designers_. One of the coolest things about C++ is that "C++ doesn't have
a string type"; things that other languages _must_ do in the core language, C and C++
can and often will do in the library instead. (Another classic example is C's `printf`.)
If you're designing a language, I think it's a fantastic idea to make a small core language
that's powerful enough to implement most things in the library instead. The Committee should
indeed prefer to extend the library where possible, rather than keep adding to the core language.
But as a working programmer, you don't need to worry about what the core language _should_ provide;
you just need to know when a standard-library facility has been obsoleted by expansions to
the core language. Here are some examples.

These examples deliberately include some that will make you say "Duh, everyone knows
that one," and some that will make some people say "Whoa, I disagree there!" However,
these are all cases that fall into the same category for me personally: places where
the core-language feature should be preferred.


## Prefer `alignof` over `std::alignment_of`

    static_assert(std::alignment_of_v<Widget> == 8);  // worse
    static_assert(alignof(Widget) == 8);  // better

Training-course students regularly ask me why `<type_traits>` provides `std::alignment_of`
at all, if `alignof` exists. The answer (as far as I know) is that the C++11 cycle took
eight years. So they brought in `std::alignment_of` from Boost, and then at some later point —
maybe _years_ later — they invented the `alignof` keyword. This made `std::alignment_of` redundant;
but it's a lot harder to remove something than to add it originally, especially since people
were already using `std::alignment_of` in real "C++0x" code.

The same situation, even worse, applies to...


## Prefer `alignas` over `std::aligned_storage_t`

    std::aligned_storage<sizeof(Widget)> data;  // utterly wrong
    std::aligned_storage_t<sizeof(Widget)> data;  // still kind of wrong
    std::aligned_storage_t<sizeof(Widget), alignof(Widget)> data;  // correct but bad
    alignas(Widget) char data[sizeof(Widget)];  // correct and better

`std::aligned_storage` was also brought in from Boost during the C++11 cycle,
before they invented the `alignas` keyword. It has many problems: The actual class
type `aligned_storage<T>` doesn't provide storage; instead, you must
be careful to use `aligned_storage<T>::type` a.k.a. `aligned_storage_t<T>`.
The template takes a second parameter that's defaulted and thus easy to forget;
if you omit that parameter, you might get storage that's _not_ aligned. Or worse,
overaligned and thus [bigger than expected](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=61458)!

The core-language `alignas(T)` attribute has none of these problems. It Just Works.

Even WG21 agrees `std::aligned_storage` is bad: C++23 deprecated it
([P1413](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2021/p1413r3.pdf)).


## Prefer `char` over `std::byte`

This is a controversial one, but I stand firmly by it. [`std::byte`](https://en.cppreference.com/w/cpp/types/byte)
is a library facility that arrived in C++17 specifically as an enumeration type for talking about octets.
To use it, you must include `<cstddef>`. It's not an integral type (good?), but on the other hand it
still supports all the integral bitwise operations such as `&`, `|`, `<<`, which (because it is an enum type
instead of an integral type) it must provide via overloaded operators in `namespace std`. It's just
[so heavyweight](https://vittorioromeo.info/index/blog/debug_performance_cpp.html#it-gets-worse-much-worse)
for what it is... which, again, is nothing but a verbose reinvention of the built-in primitive type `char`.

    std::byte data[100];  // worse
    char data[100];  // better

Similarly...


## Prefer `void*` over `std::byte*`

I believe this example is less controversial than the preceding one. Suppose you're writing a function
to copy some dumb bytes from point A to point B. How should you write its parameter types?
I've heard at least one person say "I'll pass a `std::byte*`, because my function deals in bytes."
But that requires casting at every call-site! Instead, we should use `void*`, which is the
C++ vocabulary type for "pointer to untyped memory" (i.e., just a bunch of dumb bytes).

    int data[10];

    // worse
    void mycopy(std::byte *dst, const std::byte *src, size_t n);
    mycopy((std::byte*)data, (std::byte*)(data+5), 5 * sizeof(int));

    // better
    void mycopy(void *dst, const void *src, size_t n);
    mycopy(data, data+5, 5 * sizeof(int));

C++ inherited from (post-K&R) C the notion of `void*` as a vocabulary type. K&R C didn't use `void`;
it just passed around `char*` all over the place (e.g., in K&R1's [`char *alloc()`](https://archive.org/details/TheCProgrammingLanguageFirstEdition/page/n148/mode/1up)).
This was rightly seen as a mistake and the C standardization committee quickly invented
`void*` to replace `char*`. As a bonus, C++ permits any object pointer to implicitly convert to `void*`,
whereas it's rightly more difficult to create a `char*` or `int*` or `std::byte*`.

See also: ["Pointer to raw memory? `T*`"](/blog/2018/06/08/raw-memory-and-t-star/) (2018-06-08).


## Prefer lambdas over `std::bind`

Yet another example of C++11 first taking a C++98-flavored thing (`std::bind`) from Boost into the library,
and later taking another thing into the core language in a way that essentially obsoleted the library facility.
However, `std::bind` didn't become _truly_ obsolete until C++14 gave us init-capture syntax.

    auto f = std::bind(&Widget::serve, this, std::placeholders::_1, 10);  // worse
    auto f = [this](int timeoutMs) { this->serve(timeoutMs, 10); };  // better

The lambda version is better in so many ways. It doesn't require including `<functional>`.
It gives us a place to document both the type of `x` and its purpose (via naming).
It constrains `f` to accept only one parameter, whereas the `std::bind` closure object
will happily accept any number of arguments and silently ignore the excess.
It gives us a place to specify the return type of `f`, in case we want it to be different
from the return type of `serve` (or, as in this case, to be `void`).
And `std::bind` has [unintuitive behavior](https://stackoverflow.com/a/15884700/1424877)
when one closure is nested as an argument to another, whereas lambdas compose
straightforwardly.

    auto h = std::bind(f, std::bind(g, _1));  // worse
    auto h = [](const auto& x) { return f(g(x)); };  // better

    auto j = std::bind(std::plus<>(), std::bind(f, _1), 1);  // worse
    auto j = [](const auto& x) { return f(x) + 1; };  // better


## Prefer ranged `for` over `std::for_each`

    std::for_each(v.begin(), v.end(), [](int& x) { ++x; });  // worse
    for (int& x : v) { ++x; }  // better

But the `std::for_each` algorithm can actually be the most natural way to express your intent
if you're writing an STL-style algorithm where you actually started with an iterator-pair `[first, last)`.
In that case, you don't have to use the library to break down `v.begin(), v.end()`; you'd
actually have to use the library to build `first, last` back up into a `for`-friendly range!

    for (auto&& x : std::ranges::subrange(first, last)) { myCallable(x); }  // worse
    std::for_each(first, last, myCallable);  // better

A particularly outlandish example of library misuse is the `std::accumulate` in Conor Hoekstra's
["Structure and Interpretation of Computer Programs"](https://www.youtube.com/watch?v=7oV7hiAsVTI&t=2215s) (CppCon 2020),
where he's basically reinventing a `for` loop but having to track all of his mutable bits
through the elements of a `std::pair` accumulator he manually created, instead of getting
the compiler to do it automatically via local variables on a stack frame.

    // worse
    auto [biggest, secondbiggest] = std::accumulate(
        v.begin(), v.end(),
        std::make_pair(0, 0),
        [](std::pair<int, int> acc, int x) {
            auto [biggest, secondbiggest] = acc;
            if (x > biggest) return std::make_pair(x, biggest);
            if (x > secondbiggest) return std::make_pair(biggest, x);
            return acc;
        }
    );

    // better
    int biggest = 0;
    int secondbiggest = 0;
    for (int x : v) {
        if (x > biggest) {
            secondbiggest = std::exchange(biggest, x);
        } else if (x > secondbiggest) {
            secondbiggest = x;
        }
    }

See also: ["The STL is more than `std::accumulate`"](/blog/2020/12/14/no-raw-loops-you-say/) (2020-12-14).


## Prefer `struct` over `std::tuple`

There are reasonable reasons to prefer `std::variant` over `union`, because `std::variant`
adds type-safety (it throws if you access the wrong alternative at runtime, instead of just
having [UB](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#ub)).
But there is no such reason to prefer `std::tuple` over `struct`.

    // worse
    std::tuple<std::string, IPAddress, double> getHostIP();
    auto info = getHostIP();
    std::cout << std::get<0>(info);
    auto [hostname, addr, ttl] = info;

    // better
    struct HostInfo {
        std::string hostname;
        IPAddress address;
        double ttlMs;  // "time to live"
    };
    HostInfo getHostIP();
    auto info = getHostIP();
    std::cout << info.hostname;
    auto [hostname, addr, ttl] = info;  // still works!

The `struct` version has many advantages over the `std::tuple` version: It gives us a
strong type `class HostInfo` instead of a vague tuple of data. It gives us a single
declaration for each member, so we have a place to document its purpose (via naming
and/or code comments). It gives us names for each member: `info.hostname` is less
error-prone and more readable than `std::get<0>(info)`.

`std::tuple` also generates really long mangled names, so switching to named class types
might lighten the load on your linker. Compare:

    void f(std::tuple<std::string, IPAddress, double>);
      // _Z1fSt5tupleIJNSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEE9IPAddressdEE

    void f(HostInfo);
      // _Z1f8HostInfo

> So why does `std::tuple` exist at all, you ask? Mainly, variadic templates.
> You're allowed to create a `std::tuple<Ts...>` with a variadic number of elements,
> but (as of 2022) not allowed to create a `struct S { Ts... ts; }`.


## Prefer core-language arrays over `std::array`

    std::array<int, 3> a = {1, 2, 3};  // worse
    std::array<int, 3> a = {{1, 2, 3}};  // even worse
    int a[] = {1, 2, 3};  // better

See ["The array size constant antipattern"](/blog/2020/08/06/array-size/) (2020-08-06).

> The trick with the "even worse" line above is that `std::array` is technically an aggregate,
> and the paper standard doesn't specify what its members actually are. So initializing an
> `array` with `{1, 2, 3}` is guaranteed to work because of [brace elision](https://stackoverflow.com/a/40238583/1424877),
> but initializing it with `{{1, 2, 3}}` could conceivably (on a hypothetical perverse implementation)
> fail to compile.


## Prefer C++20 `co_yield` over manual frame management

In the `std::accumulate` example above, we were using a temporary "accumulator"
to hold data in a library data structure, when it really should have been in
local variables on a stack frame (so that those variables could be seen and understood
by the compiler's optimizer; by the debugger; by the IDE; and so on, not to mention
by our own coworkers). Another place you sometimes see people managing their own
"stack frame" data structures is in coroutines.

For example, here's a hand-coded generator of Pythagorean triples;
see ["Four versions of Eric's Famous Pythagorean Triples Code"](/blog/2019/03/06/pythagorean-triples/) (2019-03-06).

    struct TriplesGenerator {
        int x_ = 1;
        int y_ = 1;
        int z_ = 1;
        int n_;
        explicit TriplesGenerator(int n) : n_(n) {}
        std::optional<Triple> next() {
            if (n_-- == 0) {
                return std::nullopt;
            }
            for (; true; ++z_) {
                for (; x_ < z_; ++x_) {
                    for (; y_ < z_; ++y_) {
                        if (x_*x_ + y_*y_ == z_*z_) {
                            return Triple{ x_, y_++, z_ };
                        }
                    }
                    y_ = x_ + 1;
                }
                x_ = 1;
            }
        }
    };

    auto tg = TriplesGenerator(5);
    while (auto opt = tg.next()) {
        auto [x,y,z] = *opt;
        printf("%d %d %d\n", x, y, z);
    }

The programmer of `TriplesGenerator` is having to manage the lifetimes of `x_`, `y_`, `z_`, and `n_`
by hand; and even worse, having to reason about the state of those variables each time the `next()`
function is exited and re-entered from the top.

Contrast the above manual code with this C++20 Coroutines code, which uses a `std::generator<T>`
class similar to [the one](https://eel.is/c++draft/coro.generator.class) currently slated for C++23.

> "But wait, I thought you said library facilities were bad!"

Not at all. I said that _some_ library facilities can be replaced with core-language features.
Other library facilities complement core-language features in a good way. The core-language
feature we're going to use in the following code is `co_yield`, so that we can stop using
data members of `TriplesGenerator` and start using plain old local variables on a stack frame
(well, a coroutine frame) that the compiler will manage for us. [Godbolt:](https://godbolt.org/z/Wxcd41GKs)

    auto triples(int n) -> generator<Triple> {
        for (int z = 1; true; ++z) {
            for (int x = 1; x < z; ++x) {
                for (int y = x; y < z; ++y) {
                    if (x*x + y*y == z*z) {
                        co_yield { x, y, z };
                        if (--n == 0) co_return;
                    }
                }
            }
        }
    }

    for (auto [x,y,z] : triples(5)) {
        printf("%d %d %d\n", x, y, z);
    }

Notice [on Godbolt](https://godbolt.org/z/Wxcd41GKs) that GCC can optimize the hand-coded `TriplesGenerator`
version down to almost nothing, whereas the `co_yield`-based version has a lot more machine instructions.
On Clang the two versions are more coequal; that's because Clang does a much worse job optimizing
`TriplesGenerator`, not because it does any better at optimizing `co_yield`.

If I had to maintain one of these two snippets in a real codebase, long-term (and I didn't have any qualms about
the long-term stability of Coroutines support), I'd definitely prefer to maintain the shorter simpler
`co_yield`-based version. Let the compiler deal with allocating variables on frames — computers are really
good at that! Save the programmer's brain cells for other things (like [dangling-reference bugs](/blog/2019/07/10/ways-to-get-dangling-references-with-coroutines/)).


## Prefer `::new (p) T(args...)` over `std::construct_at`

This is another controversial one. There's no observable difference between
a call to the library function `std::construct_at` and a manual call to `::new (p) Widget`,
so why should we prefer one over the other? For me, it's partly (unintuitively?) about
readability: I find the line involving `Widget(x, y)` looks more like a constructor call
than the line involving `(Widget*)data, x, y`. And partly it's consistent application
of the mantra of this blog post: When we can accomplish something using a shorter syntax
known to the compiler, or a longer syntax that requires library support,
as a general rule we should prefer the core-language syntax.

    alignas(Widget) char data[sizeof(Widget)];

    Widget *pw = std::construct_at((Widget*)data, x, y);  // worse
    Widget *pw = ::new ((void*)data) Widget(x, y);  // better

Ditto for `std::destroy_at` versus `p->~T()`:

    std::destroy_at(pw);  // worse
    pw->~Widget();  // better

(Observe that `std::destroy_at` can also destroy arrays, starting in C++20; whereas
a direct call to `~T()` cannot. But if you're in a situation where `T` might be an
array type, and you actually want to support that, I'm okay with forcing you to
break this guideline.)

(C++20 also begins a weird window of historical time where `std::construct_at` and
`std::allocator<T>::allocate` are required to be constexpr-friendly where `::new`
and `new` are not; so constexpr code may need to use the library facilities for now,
in the same way that C++0x code might have needed to use `std::alignment_of` or
`std::bind` before the core language caught up and obsoleted them again. I predict
this current window will close again by C++26 at the latest.)


## Prefer C++20 `requires` over `std::enable_if`

I don't use C++20 Concepts syntax in my day-to-day programming yet, but if you're writing
C++20 code, then you should almost certainly prefer `requires`-clauses over the more
old-school (and backward-compatible) SFINAE techniques.

    template<class T, std::enable_if_t<std::is_polymorphic_v<T>, int> = 0>
    void foo(T t);  // worse

    template<class T>
        requires std::is_polymorphic_v<T>
    void foo(T t);  // better

It's more readable, and [might](/blog/2021/09/04/enable-if-benchmark/) even compile faster.


## Prefer assignment over `.reset`

    auto p = std::make_unique<Widget>();
    p.reset();  // worse
    p = nullptr;  // better

There are valid use-cases for `.reset`; for example, when you're using `unique_ptr<T, Deleter>`
as an implementation detail.

    std::unique_ptr<FILE, FileCloser> fp = nullptr;
    if (should_open) {
        fp.reset(fopen("input.txt", "r"));
    }

But in general, `.reset` (and also `.get`, and even the rarely-used `.release`) is a code smell
that's worth investigating closely.


## Prefer `'\n'` over `std::endl`

    std::cout << "Hello world" << std::endl;  // worse
    std::cout << "Hello world\n";  // better

On modern platforms, the standard output is almost always _at least_ line-buffered, so flushing
the stream immediately after a newline (as `endl` does) is redundant.
`std::endl` ("newline and flush") can always be reduced to `'\n'` ("newline and let the OS
do its thing, just like you would in any other language").

See [this StackOverflow question](https://stackoverflow.com/questions/213907/stdendl-vs-n) for various
more or less interesting differences between `endl` and `'\n'`. One interesting tidbit is that `std::endl`
is a function template! Thus `decltype(std::endl)` is ill-formed, and `std::cout << std::endl` works only
thanks to the long-despised rule [[temp.deduct.funcaddr]](http://eel.is/c++draft/temp.fct.spec#temp.deduct.funcaddr),
the same rule that permits things like

    template<class T> T f(T);
    int (*p1)(int) = f;                      // OK
    std::ostream& (*p2)(std::ostream&) = f;  // OK

If it weren't for iomanipulators like `endl` and `flush`, maybe we wouldn't need that rule in the core language?


## Prefer `std::thread(lambda)` over `std::thread(f, args...)`

This is a slight variation on ["Prefer lambdas over `std::bind`."](#prefer-lambdas-over-stdbind)

C++11 `std::thread` supports a variadic number of arguments;
but you should never pass any arguments. Just bundle them into a plain old core-language lambda, so that the compiler
can see what you're doing; this will be cheaper _and_ (more to the point) permit much better error messages.

    void worker(int, int&);

    auto t = std::thread(
        worker, x, std::ref(y)      // worse
    );

    auto t = std::thread([&, x]() {
        worker(x, y);               // better
    });

The lambda version avoids using (and instantiating) `std::ref` for arguments you intended to capture by reference.
But the main reason you should always use the lambda version is that if you ever get the argument types wrong,
you trade a library error spew like this ([Godbolt](https://godbolt.org/z/PzdvYvcv9)):

    In file included from /usr/include/c++/15.0.0/thread:49:
    /usr/include/c++/15.0.0/bits/std_thread.h:156:17: error: static assertion failed
    due to requirement '__is_invocable<void (*)(int, int &), int, std::reference_wrapper<float>>::value':
    std::thread arguments must be invocable after conversion to rvalues
      156 |         static_assert( __is_invocable<typename decay<_Callable>::type,
          |                        ^~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
      157 |                                       typename decay<_Args>::type...>::value,
          |                                       ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    note: in instantiation of function template specialization 'std::thread::thread<void (&)(int, int &),
    int &, std::reference_wrapper<float>, void>' requested here
       11 |     auto t = std::thread(worker, x, std::ref(y));
          |              ^
    In file included from /usr/include/c++/15.0.0/thread:49:
    /usr/include/c++/15.0.0/bits/std_thread.h:290:31: error: no type named 'type'
    in 'std::thread::_Invoker<std::tuple<void (*)(int, int &), int, std::reference_wrapper<float>>>::__result<std::tuple<void (*)(int, int &), int, std::reference_wrapper<float>>>'
      290 |           typename __result<_Tuple>::type
          |           ~~~~~~~~~~~~~~~~~~~~~~~~~~~^~~~
    /usr/include/c++/15.0.0/bits/std_thread.h:236:13: note: in instantiation of template class
    'std::thread::_Invoker<std::tuple<void (*)(int, int &), int, std::reference_wrapper<float>>>' requested here
      236 |         _Callable               _M_func;
          |                                 ^
    [...24 more lines...]

for a nice simple compiler error:

    In lambda function:
    error: cannot bind non-const lvalue reference of type 'int&' to a value of type 'float'
       12 |         worker(x, y);
          |                   ^
    note:   initializing argument 2 of 'void worker(int, int&)'
        4 | void worker(int x, int& y) {
          |                    ~~~~~^


## Prefer `(void)expr` over `std::ignore = expr`

[Added in January 2026.]
This should have gone without saying, but now I've really seen people debating this in the wild:

    std::ignore = f();  // worse
    (void)f();  // better

The latter is better partly because it can be done without a superfluous `#include`, but also because
_it is the idiomatic way you discard a value._ Compilers have understood cast-to-void's
semantics for decades, dating back to C89.
[It's literally mentioned in the Standard.](https://eel.is/c++draft/dcl.attr.nodiscard#4.sentence-1)

I think the logic here is, "The compiler complains when I discard this expression, so, I'll trick the
compiler into _thinking_ that I wrote a nice safe assignment instead." Don't try to trick the compiler!
Compilers generally evolve over time to avoid being tricked. So while something like

    std::ignore = f();
    std::ignore = g();

might compile silently today, tomorrow's compiler might complain about a dead write on
the first line, or even inline the whole thing and point out that the assignments are superfluous.
If you use the core-language syntax, `(void)`, you know the compiler will understand: you're
_talking its language_, using the idioms that are built into it, rather than trying to invent your own.

I've seen an even wilder suggestion that you could "discard" a value by using it to initialize a
variable named `_` — what C++26 now calls a
["name-independent declaration."](https://eel.is/c++draft/basic.scope.scope#def:declaration,name-independent)
(A C++26 compiler must permit multiple definitions of this name even within the same scope, and is
not supposed to warn if such a variable goes unused.)

    auto _ = f();  // much worse

This doesn't require a library header, but it's certainly much worse than either alternative above —
for human readability, for the compiler's internals (which must now track a declaration rather than
a relatively simpler expression statement), for runtime performance (variable `_` lasts to
the end of the scope, whereas `(void)f()` destroys the temporary result right away),
and for correctness (`auto` will Do The Wrong Thing here if `f` returns a reference type).


## Finally, some near misses

Regular readers of this blog will know that I advise strongly against [CTAD](/blog/tags/#class-template-argument-deduction),
which means that I strongly prefer the library syntax `std::make_pair(x, y)` over `std::pair(x, y)`,
[`std::make_reverse_iterator(it)` over `std::reverse_iterator(it)`](/blog/2022/08/02/reverse-iterator-ctad/),
and so on. Of course if `std::pair(x, y)` and `std::make_pair(x, y)` did the same thing,
it wouldn't matter which you used, and you might well prefer the shorter, "corer" syntax.
But they _don't_ do the same thing, and in fact CTAD is so pitfally that I just prefer to
ban it outright.

Either CTAD is the [exception that proves the rule](https://en.wikipedia.org/wiki/Exception_that_proves_the_rule),
or the explanation is that _both_ syntaxes involve standard library facilities:
my preferred syntax involves a standard library function, and my disfavored syntax involves
a set of deduction guides (some implicitly generated) associated with a standard library type.
So we have a choice between two different library facilities, and we should choose the one
that's less likely to lead to a bug.

> Also note that in contexts where you get to choose between `std::make_pair(x, y)` and
> a simple braced initializer `{x, y}`, I'd often recommend the latter.

Regular readers of this blog will also know that I tend to prefer
`T(x)` over `static_cast<T>(x)`, [despite some caveats](/blog/2020/01/22/expression-list-in-functional-cast/).
Both of those are core-language syntaxes. But it's probably no coincidence
that I prefer the shorter, less angle-brackety of the two.

---

See also:

* ["_The Elements of Programming Style_ (1974)"](/blog/2024/11/27/software-classics/) (2024-11-27)

{% endraw %}
