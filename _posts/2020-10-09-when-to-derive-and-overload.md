---
layout: post
title: 'Inheritance is for sharing an interface (and so is overloading)'
date: 2020-10-09 00:01:00 +0000
tags:
  c++-style
  classical-polymorphism
  metaprogramming
  templates
---

This blog post was inspired by [my answer to a question](https://codereview.stackexchange.com/a/250416/16369)
on Code Review StackExchange. To utterly oversimplify the question: When is it appropriate
to use classical inheritance?

    struct Cat {
        void meow();
    };
    struct Dog {
        void bark();
    };

    // Should I write a struct AbstractAnimal?

Here's what the same code would look like, with an `AbstractAnimal` base class
(and omitting the [NVI](/blog/2019/08/02/the-tough-guide-to-cpp-acronyms/#nvi)
for pedagogical reasons):

    struct AbstractAnimal {
        virtual void do_speak() = 0;
    };

    struct Cat : public AbstractAnimal {
        void meow();
    private:
        void do_speak() override { meow(); }
    };

    struct Dog : public AbstractAnimal {
        void bark();
    private:
        void do_speak() override { bark(); }
    };

The *only* reason to use a base class here is in order to write polymorphic code that operates
on (a pointer or reference to) that base class.

    // If all my code looks like this, I don't need a base class.
    void operateOnCat(Cat *c) { c->meow(); }
    operateOnCat(myCat);

    // I need the base class only if I want to write code like this.
    void operateOnAnything(AbstractAnimal *a) { a->do_speak(); }
    operateOnAnything(myCat);
    operateOnAnything(myDog);

----

If you don't need to write `operateOnAnything`, then you *shouldn't* write `AbstractAnimal`.
Abstraction is a tool for solving problems — and, as such, it is also a signal to the reader
that a problem is being solved! If your code doesn't actually _need_ polymorphism in order
to work, then you should avoid making an inheritance hierarchy.

> Keep it simple, stupid.

The design principles of polymorphic programming apply equally to generic programming...


## When should I name two methods the same?

Notice that in our original `Cat`/`Dog` code, I named the two methods `Cat::meow()`
and `Dog::bark()`. It's very tempting to give them the same name:

    struct Cat {
        void talk();  // meows
    };
    struct Dog {
        void talk();  // barks
    };

However, this is once again a signal to the reader that something indispensable is
going on here! Meowing and barking are vaguely similar at an _abstract_ level, yes;
but abstraction is a tool for solving problems, and so we should avoid it unless
a problem exists to be solved.

The *only* reason to use the same name for meowing and barking, here, is in order
to write generic code that operates on an object of unknown type `T`.

    // If all my code looks like this, I don't need to reuse the
    // same name in both classes.
    void operateOnCat(Cat& c) { c.meow(); }
    operateOnCat(myCat);

    // I need the same name only if I want to write code like this.
    template<class T>
    void operateOnAnything(T& t) { t.talk(); }
    operateOnAnything(myCat);
    operateOnAnything(myDog);

Notice that another way to ask "When should I name two methods the same?"
is to ask "When should I define a C++20 `concept` modeled by both these classes?"
The rules of classical OOP are isomorphic to the rules of generic programming with
concepts: just replace "base class" with "concept" and "derives from" with "models."

Let's go deeper.


## When should I put two functions into an overload set?

The previous section was about reusing the same name _outside_ of an overload set:
`Cat::talk` and `Dog::talk` aren't "overloads" per se. But what about something like
this?

    struct Snake {
        void eatPellet(const Pellet&);
        void eatCat(const Cat&);
    };

Should we rename both methods to just `Snake::eat`, creating an overload set?

The *only* reason to use the same name for eating pellets and eating cats, here,
is in order to write generic code that passes arguments of unknown type `T`.

    // If all my code looks like this, I don't need to create
    // an overload set.
    void consumePellet(Pellet& p) { mySnake.eatPellet(p); }
    consumePellet(myPellet);

    // I must create an overload set only if I want to write code like this.
    template<class T>
    void consumeAnything(T& t) { mySnake.eat(t); }
    consumeAnything(myPellet);
    consumeAnything(myCat);

The major place we see overload sets being used for their intended purpose in C++
is in constructors and assignment operators. Constructor overload sets help us to write
generic functions like `std::make_shared<X>(args...)` and `vector<X>::emplace(args...)`
which forward `args...` along to some constructor in the overload set (and we don't
have to care which one). Assignment-operator overload sets help us to use generic
STL functions like `std::fill`.

The major place we see overload sets being _misused_ in C++ is the STL containers' `erase`
method:

    v.erase(first, last);  // erase a range of elements
    v.erase(first);  // oops, erase just one element

I see this pitfall in real code more often than I want to. And the worst part is
that the overload set on `erase` serves no purpose! Nobody is writing generic code like

    template<class... Args>
    void eraseAnyhow(Args... args { myVec.erase(args...); }
    eraseAnyhow(it);
    eraseAnyhow(first, last);

The overload set is not motivated by genericity; therefore it should not exist.
Imagine how many fewer bugs we'd have had over the past 20 years if the original
STL containers had given these two different facilities two different names:

    v.erase(first, last);
    v.erase_at(it);


### Wait, really?

I wrote:

> The *only* reason to use the same name for eating pellets and eating cats, here,
> is in order to write generic code that passes arguments of unknown type `T`.

Aren't there any other valid reasons to do function overloading? Well, here's one:
refactoring. If you're changing the type of the edibles in your game from `class Pellet`
to `class Fruit`, it might be perfectly reasonable to add the `Snake::eat(Fruit)`
overload before removing `Snake::eat(Pellet)`. This is kind of the same logic as
in my quotation above: you're not writing "generic code" per se, but you're still
dealing with code where the code's author didn't know what type they were dealing with.
When they _wrote_ the code, they were passing `Pellet`; but _now_ they're
passing `Fruit`. Refactoring is kind of like generic programming with templates,
turned 90 degrees through the temporal axis.

The other reason to create an overload set in C++ is because _conceptually_ you
have only one function, but for technical reasons (usually performance) you
have to implement it as an overload set. For example, it's common to overload
`push_back(const Foo&)` and `push_back(Foo&&)` — conceptually one single facility, but
implemented as an overload set for performance. In C++14 it was common to overload
`foo(const std::string&)` and `foo(const char*)` — conceptually one facility, but
implemented as an overload set for performance. In C++17 we were finally able to
ditch that overload set in favor of a single function `foo(std::string_view)`.

[Titus Winters calls](https://www.youtube.com/watch?v=2UmDvg5xv1U&t=4m44s)
the overload set "the atom of C++ API design" — functions themselves being
maybe the protons or something. So putting two functions in the same overload
set is saying that they're _so closely related_ that nobody ever needs
to know which of them is being called. I'm merely turning that logic around:
putting two functions in the same overload set is tantamount to saying that
your codebase contains at least one place in which somebody _cannot afford to know_
which of them is being called! If you're not in that situation, don't overload.


## Conclusions

* Keep it simple, stupid.

* [Inheritance is for interface compliance.](https://isocpp.org/wiki/faq/objective-c#objective-c-and-inherit)
    (Anyone got a less clunky version of this mantra?)

* Use a single name for a single entity. Never use two names where one is appropriate.
    Even more importantly, never use one where two would be more appropriate.

* Inheritance, genericity, and overloading are abstractions. Abstraction is a tool
    to solve problems. If you apply the tool in a situation where
    there is no problem to be solved, you're implicitly lying to your reader,
    and may confuse them.

----

UPDATE: I gave a talk at C++Now 2021 expanding on this blog post;
watch it [here](https://www.youtube.com/watch?v=OQgFEkgKx2s).
