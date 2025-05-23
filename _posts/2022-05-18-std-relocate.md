---
layout: post
title: "`std::relocate`'s implementation is cute"
date: 2022-05-18 00:01:00 +0000
tags:
  pearls
  relocatability
---

My paper [P1144 "Object relocation in terms of move plus destroy"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2020/p1144r5.html)
is still on R5 as of this writing, but I hope to publish P1144R6 Real Soon Now.
I recently updated my Clang fork on [p1144.godbolt.org](https://p1144.godbolt.org/z/T7ozTx34d)
to reflect the changes in R6.

One of those changes is that R0's algorithm `std::relocate_at(T* source, T* dest)` is joined
by [`T std::relocate(T* source)`](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p1144r6.html#wording-relocate).
This new algorithm `std::relocate` takes the object pointed to by `source` and relocates it into
the return slot, giving you a prvalue. You can use this prvalue in combination with the
[superconstructing super elider (2018-05-17)](/blog/2018/05/17/super-elider-round-2/)
to emplace the relocated object directly into a container ([Godbolt](https://godbolt.org/z/cqPP4oeE9)):

    void relocate_in(std::vector<W>& v, W *pw) {
        struct S {
            W *pw_;
            operator W() const { return std::relocate(pw_); }
        };
        v.emplace_back(S{pw});
    }

    void test_relocate() {
        alignas(W) char buffer[sizeof(W)];
        W *pw = ::new (buffer) W();
        std::vector<W> v;
        relocate_in(v, pw);
        // now v[0] is alive and *pw is dead
    }

Notice that `test_relocate` calls `::new` to begin the lifetime of the
object at `*pw`; but it never calls `pw->~W()` to destroy that object.
Instead, it _relocates_ the object from `*pw` into the newly emplaced `v[0]`;
and then the vector takes care of destroying `v[0]` at the closing curly
brace.

P1144 `std::relocate` is tantamount to a memcpy for trivially relocatable types;
for non-trivially relocatable types it falls back to move-plus-destroy. You can
see that in the Godbolt if you replace `Widget`'s `std::string` member with a
`std::list<int>` member instead.

Now for the main point of this post. `std::relocate` is _tantamount_ to a memcpy,
but how do we actually implement that? Standard C++ doesn't give us any way to
access the return slot directly; even NRVO wants its named return variable to
be _constructed_, not plopped on the stack as a bag-of-bits we memcpy into!
So the solution will involve UB, and knowledge of the calling convention on our ABI.
It happens that on the Itanium ABI, whenever the return type's destructor is
non-trivial, the callee receives a "hidden pointer" to the return slot in the
first parameter slot. The real first parameter is shifted to the
second parameter slot. So, from the Itanium ABI's point of view, `T foo(T* source)`
has the same signature as `T* foo(T* dest, T* source)`.

Vice versa, `T* memcpy(T* dest, T* source, size_t n)` has the same signature
as `T memcpy(T* source, size_t n)`... and we can use that!
Our library-vendor implementation of `std::relocate`, for the Itanium ABI,
can look just like this:

    template<class T>
        requires is_trivially_relocatable_v<T> &&
                 !is_trivially_destructible_v<T>
    T relocate(T *source) noexcept {
        auto magic = (T(*)(void*, size_t))memcpy;
        return magic(source, sizeof(T));
    }

Cute, right?

----

Of course there's also a second overload for types that are either non-trivially
relocatable (`std::list`), where we must move-plus-destroy for correctness; or
fully trivial for purposes of ABI (`int`), where the memcpy trick doesn't work
because the calling convention is different. This second overload is a bit
more complicated because it's non-noexcept:
we must ensure that we destroy `*source` even if the move
constructor throws. Using ["The `Auto` macro"](/blog/2018/08/11/the-auto-macro/) (2018-08-11)
to hide the boilerplate, we might write that overload as follows:

    template<class T>
    T relocate(T *source)
        noexcept(is_nothrow_move_constructible_v<T>)
    {
        Auto(source->~T());
        return std::move(*source);
    }


## Further reading

For more on how prvalues essentially (since C++17) smuggle around "recipes" for creating objects,
read Sy Brand's blog post
["Guaranteed Copy Elision Does Not Elide Copies"](https://blog.tartanllama.xyz/guaranteed-copy-elision/) (December 2018).
For more on the Itanium ABI calling convention and the "return slot," watch the first eight minutes
of my CppCon 2018 talk ["RVO is Harder Than It Looks."](https://www.youtube.com/watch?v=hA1WNtNyNbo)
