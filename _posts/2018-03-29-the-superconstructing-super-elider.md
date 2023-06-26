---
layout: post
title: 'The Superconstructing Super Elider'
date: 2018-03-29 00:01:00 +0000
tags:
  copy-elision
  move-semantics
  proposal
  sufficiently-smart-compiler
---

Today a coworker asked me why there was an "extra" move happening in the following
code:

    struct Widget {
        int value_;
        explicit Widget(int i) : value_(i) { puts("construct from int"); }
        Widget(Widget&&) noexcept { puts("move-ctor"); }
        ~Widget() { puts("destructor"); };
        static Widget make() { return Widget(42); }
    };

    int main() {
        vector<Widget> vec;
        vec.reserve(100);
        vec.push_back(Widget::make());  // HERE
    }
 
On the line marked `// HERE`, we observe that the compiler generates a call
to `Widget::make()` to construct a `Widget` object on the stack; then it
passes the address of that stack object (as `Widget&&`) to `vec.push_back`,
which uses the *move constructor* of `Widget` to move it into its final
location within the vector's allocated buffer.

This seems inefficient, said my coworker. What we *want* to happen is,
it should construct the `Widget` directly into the allocated buffer,
with no intermediate move. "C++11 to the rescue," right? Let's use that
"perfect forwarding" we've heard so much about!

        vec.emplace_back(Widget::make());

...Same thing. The factory function constructs onto the stack, and then
the vector *moves* the `Widget` from the stack into the buffer. Well,
of course. The signature of `emplace_back(Args&&...)` is not significantly
different from the `push_back(T&&)` that we were using before.


## Eliminating redundant moves via `emplace_back_with_result_of`

This leaves space "underneath" the standard library for us to do better.
Consider the following `fixed_capacity_vector` implementation:

    template<class T, int Cap>
    class fixed_capacity_vector {
        int size_ = 0;
        alignas(T) char buffer_[Cap][sizeof(T)];
    public:
        // ...

        template<class... Args>
        T& emplace_back(Args&&... args) {
            assert(size_ < Cap);
            T *p = new ((void*)buffer_[size_]) T(std::forward<Args>(args)...);
            size_ += 1;
            return *p;
        }
    };

We compile our sample `emplace_back` and observe that the compiler
[has generated a redundant move operation](https://wandbox.org/permlink/n4bfofKx2zXbek96),
just as it does with the real `std::vector`.

Then we add the following "improved" function:

        template<class F>
        T& emplace_back_with_result_of(const F& factory_fn) {
            assert(size_ < Cap);
            T *p = new ((void*)buffer_[size_]) T(factory_fn());
            size_ += 1;
            return *p;
        }
    };

This code is fundamentally different from `emplace_back`, in a way that
allows [fundamentally better codegen](https://wandbox.org/permlink/n4bfofKx2zXbek96).
In the former case,
we call `Widget::make()` to get a new object, and then call
`emplace_back` to do some address computation and ultimately *move* the
object into its final location in the vector. The move is unavoidable.
In the latter case,
we defer the call to `Widget::make()` until the middle of
`emplace_back_with_result_of`. So, by the time we call `Widget::make` (a.k.a.
`factory_fn`), the vector has already done the computation to figure out the
final resting place of our new `Widget` object. It then calls `Widget::make`
with exactly that address in the return slot, which means that we construct
the return value of `Widget::make` directly into its final location in the vector!

    vec.emplace_back(Widget::make());  // Does an extra move

    vec.emplace_back_with_result_of([]{
        return Widget::make();  // Constructs in-place
    });

Neat, huh?

Notice that none of the explanation here has changed since move semantics were
introduced in C++11; and an analogous scenario can be constructed in C++98 using
copy semantics, as well. The only thing that has changed in the last 20 years is
that prior to C++17, compilers were *permitted* to do two moves in the first case
and one (or two?) moves in the latter case, whereas as of C++17, any compiler that
still does that is officially non-conforming. No compiler has actually done those
extra moves for years, though. C++17's "guaranteed copy elision" was really just
catching the Standard up to existing practice.


## Rabbit hole avoidance: Allocators

The real STL's `std::vector` has no `emplace_back_with_result_of`. And it
can't easily add one, because the new method would have to respect the
rules of allocator-awareness. Which means that it could not so blithely call

    T *p = new ((void*)buffer_[size_]) T(factory_fn());

It would actually have to do something like

    T *p = (T*)buffer_[size_];
    allocator_traits<A>::construct(
        get_allocator(), p, factory_fn()
    );

which would still have the extra move (it's calling `factory_fn` before it knows where
the result needs to go), so again we'd have to refactor and call

    T *p = (T*)buffer_[size_];
    allocator_traits<A>::construct_with_result_of(
        get_allocator(), p, factory_fn
    );

which would involve adding a new member to `allocator_traits`, and... so on
down the rabbit hole. So please don't expect `emplace_back_with_result_of`
to arrive in your friendly neighborhood STL anytime soon!

Besides, I have a better idea...


## The Super-Elider

(Thanks to Connor Waters for working this idea out with me today.)

The "extra move" problem with `emplace_back` is closely related to another
C++ WTF that I've known about for a long time:

    template<class T>
    inline constexpr T identity(T x) { return x; }

    Widget y = identity(identity(Widget(42)));

This code constructs a `Widget` with the value 42; and then moves it; and
then moves it again (into its final resting place, the address of `y`).
It seems silly that the compiler doesn't optimize it into a single
construction at address `y` to begin with! But in fact the compiler is
currently *forbidden* to do that optimization.

According to the rules of C++, every "observable" side-effect in the program
*must* take place, unless it is specifically permitted to be elided. And the
only operations that the Standard specifically permits us to elide are
copy-constructions and move-constructions under very specific conditions.
One of these conditions is that the source must *not* be a function parameter.
So the move-constructions from `x` into the return slot of `identity`, and again from
`x` into the return slot of `identity`, are not elidable; but the final move
from the return slot of `identity` into the address of `y` *is* elidable
(because it is moving out of a return slot, and that's [specifically permitted](http://eel.is/c++draft/class.copy.elision#1)
to be elided).

[As Stepanov said in 1998](http://stepanovpapers.com/DeSt98.pdf),

> A component programmer must be able to make some fundamental
> assumptions about the interfaces she uses [...]
> The operations we have discussed here, equality and copy, are central because they
> are used by virtually all programs. They are also critically important because **they are
> the basis of many optimizations which inherently depend upon knowing that copies
> create equal values**, while not affecting the values of objects not involved in the copy.
> Such optimizations include, for example, common subexpression elimination,
> constant and copy propagation, and loop-invariant code hoisting and sinking. These
> are routinely applied today by optimizing compilers to operations on values of built-in
> types. **Compilers do not generally apply them to operations on user types because
> language specifications do not place the restrictions we have described on the
> operations of those types.**

Twenty years later, very little has changed. C++11 made some halting steps in
the right direction by defining notions of "defaulted" and "trivial" special member
functions; but there are still many cases even in the standard library where a
special member function may be *non-trivial* but still, let's say, *Stepanov-friendly*.

    std::string y = identity(identity(std::string("hello world")));

[Compilers generate just terrible code for this kind of thing.](https://godbolt.org/g/GoRTkc)
We want to tell the compiler, here, "I promise that `std::string` is a nice
clean Stepanov-friendly *value type*; please feel free to treat it as such."
And then the inliner could look at this code, determine that it was ultimately
just assigning `"hello world"` into `y`, and *just do that*.

The reason C++ doesn't allow the compiler to just make that assumption by default
is that in C++, the notions of "value" and "object" are still pretty tangled up
together. In C++, *everything* is an object (or else a reference to an object),
and objects have addresses.

    int count_up(const std::string& s) {
        static std::map<const void *, int> m;
        const void *addr = std::addressof(s);
        m[addr] += 1;
        return m[addr];
    }

    int main() {
        std::string x = "hello world";
        assert(count_up(x) == 1);
        assert(count_up(x) == 2);
        assert(count_up(std::string(x)) == 1);  // Whoa!
    }

The output of this program must be the same on every conforming C++ implementation —
because, remember, C++ compilers are not allowed to elide copies or moves (not even
silly-looking ones like `std::string(x)`) unless that elision is *specifically permitted*
by the Standard.

If we were willing to break sneaky code like the above — code that claims to work with
value types, but then deliberately goes and looks at the *object address* of the underlying
non-value C++ *object* — then we could make `std::string(x)` mean exactly the same thing as
`x`, which is quite possibly what new C++ programmers expect is supposed to happen
anyway.

I suggest that a relatively plausible way this could happen is by the introduction of
an attribute which I am going to call `__attribute__((value_semantic))`. (Another option
would be to recycle `__attribute__((pure))`, as in "pure value".
I am specifically *not* going to use C++11 `[[attr]]` syntax in this blog post, so as
not to open _that_ can of worms (["The Ignorable Attributes Rule"](/2018/05/15/the-ignorable-attributes-rule/) (2018-05-15)).

    struct Widget __attribute__((value_semantic)) {
        int value_;
        explicit Widget(int i) : value_(i) { puts("construct from int"); }
        Widget(Widget&&) noexcept { puts("move-ctor"); }
        ~Widget() { puts("destructor"); };
        static Widget make() { return Widget(42); }
    };

In this brave new (fictional) world, when a class type is decorated with
`__attribute__((value_semantic))`, the compiler is permitted to elide
move-construction or copy-construction operations, no matter in what context
they appear: the destination (new) object is always permitted (but never required)
to have the same address as the source object. (This would mean that our `puts`es
in the code above would not reliably be hit: the compiler would be free to
eliminate them, as long as it did so in matched pairs. Please bear in mind that
we are not saying that `Widget` is *trivially* moveable; it is not.
Elision is not about replacing copy with move, or replacing move with `memcpy`;
it is about *coalescing source and destination into the same physical object
so that moving becomes unnecessary.*)

Unfortunately we can never *require* that this kind of elision happen predictably.
Consider this code:

    Widget identity(Widget x) { return x; }

    Widget y = identity(Widget(42));

The compiler would be *encouraged* to elide the extra move when the definition of `identity`
is visible in the current translation unit; but it would be *impossible* for it to
elide the extra move if the definition of `identity` were *not* visible (because
`identity` absolutely must move from `x` into the return slot, unless the return slot
has the same address as `x`; but the caller has no idea that the return slot *should*
have the same address as `x`, since it cannot see the definition of `identity`).

I am not yet sure, but I think all we care about here is *construction*, and of course
the matching destructions. I can't see how to get any clever optimization opportunities
by eliding calls to `operator=`:

    Widget y(41);  // ???
    y = Widget(42);

If `Widget` were "value-semantic", would the user legitimately expect the above code
to be equivalent to

    Widget y(42);

Suppose we give `Widget` and `42` real names; does our intuition change?

    std::unique_lock y(mutexA);
    y = std::unique_lock(mutexB);

(And yes, maybe our intuition changes back, if the constructor being elided is
specifically the *zero-argument* constructor; but default-constructing-into-the-empty-state
is another rabbit hole down which we will find Opinions, so I'd like to save it
for now.)

I have no idea how to express the desired semantics here in standardese.
Connor suggested that it might be as simple as saying that expressions of
`value_semantic` type should always be treated as *prvalues* even when today's
C++ would consider them *xvalues*. I doubt it's that simple, though, since
we somehow have to introduce flexibility into the semantics; we need
"permitted but non-guaranteed elision" to deal with the
`identity`-in-different-translation-unit problem above.

The mainstream of C++ standardization in the last release cycle (2014 to 2017)
was all about *rigidifying*: guaranteed copy elision, guaranteed order of evaluation.
Is the time ripe for deregulation? Should we build a super-elider?

And the biggest open question for me: *Can* we build it — or are the current
rules so thoroughly baked into today's compilers that they wouldn't be
able to exploit the flexibility even if they got the necessary permissions?
The proof-of-concept would be an inliner that could optimize

    Widget y1 = identity(identity(Widget(42)));  // A
    Widget y2(42);                               // B

into the same exact machine instructions. If you've got one, I'd love to see it!

----

See also:

* ["Superconstructing super elider, round 2"](/blog/2018/05/17/super-elider-round-2/) (2018-05-17)
