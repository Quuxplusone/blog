---
layout: post
title: "Socrates on why there's no `erase_if` for `span`"
date: 2023-07-16 00:01:00 +0000
tags:
  library-design
  parameter-only-types
  socrates
---

_With apologies to Plato; but not to Socrates, who's already got one._

{% raw %}
A month ago, I was passing through [the marketplace](https://cppalliance.org/slack/)
when I happened to meet with Eryximachus. He had an idea about `std::erase_if`
he wanted to share, and as I had [written on the subject](/blog/2020/07/08/erase-if/)
only a few years earlier, we struck up a conversation.

You know, said Eryximachus, I think there should be a version of `std::erase_if`
that applies to `std::span<T>`. It would be easy to implement; look here.

    template<class T, class Pred>
      requires !is_const<T>
    size_t erase_if(span<T>& sp, Pred p) {
      auto it = std::remove_if(sp.begin(), sp.end(), p);
      size_t count = sp.end() - it;
      sp = span<T>(sp.begin(), it);
      return count;
    }

He continued: This function is clever, if a bit unintuitive at first. You see, when
we erase from a `vector`, we actually destroy elements right then and there,
because the vector owns its elements. A `span` doesn't own its elements; it merely
views them. But a `span<T>` (for non-const `T`) is allowed to modify those elements.
So this version of `erase_if` simply shuffles the
filtered elements to the end of the existing range, and then snips off its own tail
so that those elements aren't viewed by the `span` any longer.
Those elements won't be destroyed until the underlying container goes out of scope,
but they'll be eliminated from the span, which is what we asked for.

I thought this was an interesting idea, but after some brief reflection I said:
No, I don't think that would be a good addition to the library at all.
I think the library's authors were right not to create such a function.

Why do you say that, Socrates?

I believe it would harm the ability to use `span` as a drop-in replacement for
`vector&`.

I don't see how adding a _new_ function can harm any _existing_ ability of the
library; can't it only add functionality? I think you'd better explain what you mean.
I know you've talked about "drop-in replacements" before, in the context of what
you call "parameter-only types."

That's right. For example, if I have a function taking a parameter `const string& s`,
it's generally (although not always) possible for me to replace that parameter with
`string_view s`. Most of the operations that worked on `const string& s` will continue
to work just fine with `string_view s`: printing, indexing, getting iterators into
the string, `.substr()`, comparison, and so on. Operations that a `string_view` can't provide,
such as [`.c_str()`](/blog/2020/03/20/c-str-correctness/), `.capacity()`, and `+`,
will give me a compiler error.

Right; if my function's body uses `s.c_str()` or `s.capacity()`, I won't be able to change
its parameter `const string& s` into `string_view s`, because the code would no
longer compile. That's what you meant when you said "(although not always)."

That's right. But here's the important point: Many operations continue
to work just fine with `string_view`, and many operations rightly give
compiler errors, but there are _very few_ operations that will continue to compile
yet produce different behavior. Quietly producing different behavior is one of the
worst things that can happen in a refactoring.

You say "very few," but not "none." Could you give an example of a `string` operation
that quietly produces different behavior with `string_view`?

Certainly. Suppose we make a copy of the parameter `s`, and then try to modify only the copy:

    void f(const std::string& s, int myInt) {
      auto t = s;
      if (myInt != -1) {
        t = "the answer is " + std::to_string(myInt);
      }
      std::cout << t << '\n';
    }

When `t` is `string`, this works as intended. When `t` is `string_view`, we end up
binding our `string_view` to a temporary object, which dangles immediately, and so
the final line of the function prints garbage ([Θειοκεραυνός](https://godbolt.org/z/13acjGhT3)).

That doesn't look good, Socrates!

I agree. To make `string_view` _really_ fit for its purpose as a "parameter-only type"
and a drop-in replacement for `const string&`, the library author should have deleted
its assignment operator. But even so, I really did have to go far out of my way to craft
that example. I couldn't just assign directly into `s`, because in the code we started with,
`s` would have been a `const string&`, immutable. So I had to make the example fairly
complicated and unnatural. Besides, the ability to assign `string_view` objects, and use them as
regular types, is useful in its own way. We'll go into that some other time.

All right; I'll hold you to that! But for now I'd like to know how this philosophy
applies to `std::span`.

My notion of drop-in replacement applies to `span` in pretty much the same way as
it applied to `string_view`. In fact, for spans of const types, it's so similar that
we needn't go over it again; I'll just remark that `span<const T>` is a drop-in replacement
for `const vector<T>&`, and say no more. But there's a wrinkle with `span` that
didn't occur with `string_view`.

I suppose you're referring to the fact that `span<T>` allows you to modify its elements,
while `string_view` doesn't.

That's right. We'll say that `span<T>`, without the `const`, is a drop-in replacement
for a mutable `vector<T>&` parameter.

How does that lack of `const` change things?

Well, it means we must be much more careful about operations that quietly produce different
behavior! Consider all the things one can do with a `string` that one can't do with a
`string_view`: there's `+=`, and `resize`, and `insert` and `push_back`, and `erase` and `pop_back`,
and `clear`, and many other operations.

Yes, but nobody would ever try to do those operations on a `const string&` parameter, because
they simply wouldn't compile.

Right; and they won't compile if you try to do them with a `string_view` either.
But you _can_ do operations like `resize` and `clear` on a mutable `vector<T>&` parameter,
so we must be vigilant in order to preserve our "drop-in replacement"–ness: each operation must
either do the same thing with both `vector` and `span`, or else it must not compile.
For example:

    void g(std::vector<int>& v) {
      std::ranges::sort(v);
      v.clear();
    }

    void g(std::span<int> v) {
      std::ranges::sort(v); // OK, does the same thing
      v.clear(); // OK, does not compile
    }

At this, Eryximachus furrowed his brow. Socrates, he said, I'm thinking of your earlier
example. You called it unnatural and contrived because you couldn't assign straight into
`s`. But with a mutable reference parameter `vector<T>& v`, you _could_ assign straight
into `v`, like this:

    void h(std::vector<int>& v) {
      std::vector<int> v2 = {1,2,3};
      v = v2;
    }

This obviously does something different when `v` is a `span<int>` than when it's a
`vector<int>&`.

That's true, I said, and again it shows that `std::span` doesn't _quite_ live up to
the Platonic ideal of a parameter-only type. Just as with `string_view`, the library authors
decided to make it a regular type, so that you can put spans inside containers and so on;
and in exchange, it loses some of this property we're trying to give it.
On the other hand, I can't resist pointing out that we do have _some_ safety here:

    v = {1,2,3};

compiles when `v` is `vector<int>&` (and incidentally also when `v` is `span<const int>`),
but safely fails to compile when `v` is `span<int>`. That's because `span<int>`, like
`reference_wrapper<int>`, binds only to mutable lvalues; it can't bind to an rvalue like that.

> UPDATE, 2023-12-28: Here Socrates originally used "archaic" double-braces `{{1,2,3}}` for
> C++23 compatibility; see ["`std::span` should have a converting constructor..."](/blog/2021/10/03/p2447-span-from-initializer-list/)
> (2021-10-03). [In C++26](https://github.com/cplusplus/draft/pull/6691)
> `span<const int>` no longer needs double braces.

To resume — said Eryximachus — we're talking about the operations provided by `vector<int>&`, and
asking for each operation whether it does the same thing on `span<int>`, or a different thing.
In the latter case it should refuse to compile.

Right.

But how do you define "to do the same thing"?

Very simply, Eryximachus: We consider the effect on the caller's vector. Anyone who calls
our original function `void h(vector<int>&)` must have an lvalue vector to pass in. `h` will
have some effect on that vector: It might sort the vector, or add 2 to every element of the vector,
or increase the length of the vector. When we change the signature of `h` to `void h(span<int>)`,
we expect that `h` will continue to have the same effect — to "do the same thing" — or else
refuse to compile.

Well, said Eryximachus, if the original function `h` increased the length of the vector, then
`h(span<int>)` must certainly refuse to compile, because there's no way the `span` could increase
the length of the original vector without access to it!

That's right, I said. So `span` must not have a `push_back` method. Now what do you think about
`pop_back`?

Hm, said Eryximachus, I suppose the same logic applies. If the original function `h(vector<int>&)`
performed `v.pop_back()` to decrease the length of the caller's actual vector, then `h(span<int>)`
had better refuse to compile.

And likewise `clear`, `resize`, `insert`, and `erase`?

Yes, `span` had better not provide any of those.

Now, what about non-member functions? [Herb Sutter says](http://www.gotw.ca/publications/mill02.htm)
a non-member function can be just as much "part of" a class as its member functions are.

Yes, Socrates. For example, `swap`, or operators like `<<`, can't be member functions,
but would certainly be considered "part of" a class like `vector` or `span`. Even `operator==`
is frequently a hidden friend, not a member.

And what about non-members that aren't called via ADL, such as `std::erase_if`?

Yes, I suppose each overload of `std::erase_if` is also "part of" the specific class
it was designed to support.

So we can say that `vector` "provides `std::erase_if`"?

Yes, Socrates.

Should `span`, then, also provide `std::erase_if`? Can we ensure that it "does the same thing"
as `vector`'s `std::erase_if`?

No, Socrates. If the original function `h(vector<int>& v)` performed `std::erase_if(v, pred)`,
it would decrease the length of the caller's actual vector. But `h(span<int> v)` can't do that.
So it had better refuse to compile.

So, you no longer think there should be a version of `std::erase_if`
that applies to `std::span<T>`?

Indeed, I've modified my views on that subject. But for the sake of argument, please, let's
imagine I knew of someone you had not yet convinced; do you think there's another argument
you could make that would convince even the strongest holdout?

Well, I don't know about the _strongest_, I said. But if you'll indulge me a moment longer,
there is one more tack I could try.

By all means.

This holdout I'm trying to convince — do you think he'd
agree that `span` is correct not to provide `.clear()`? I mean, wouldn't it be disastrous if

    void k(vector<int>& v) { v.clear(); }
    int main() { std::vector<int> v = {1,2,3}; k(v); }

were refactored into `void k(span<int> v)`, compiled silently, and then cleared only the
span, instead of clearing `main`'s actual vector?

I think he'd agree that that would be disastrous, and that `span` quite properly
doesn't provide a `clear` method.

Then could he possibly disagree with the same example reformulated to use `std::erase_if`,
like this?

    bool True() { return true; }
    void k(vector<int>& v) { std::erase_if(v, True); }
    int main() { std::vector<int> v = {1,2,3}; k(v); }

No, Socrates, I don't think even my imaginary holdout could possibly disagree with
that example. Your logic seems to work on airy folk like him just as thoroughly as
it works on flesh-and-blood people like me; it's really quite marvelous.

Then I will bid you both good day.

Good day, Socrates.
{% endraw %}
