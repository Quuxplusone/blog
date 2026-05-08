---
layout: post
title: "C++Now video tidbits"
date: 2019-07-02 00:01:00 +0000
tags:
  blog-roundup
  cppnow
---

You know [that post on hidden friends that I was going to write back in April](/blog/2019/04/26/what-is-adl/)?
Well, Anthony Williams just ate my lunch! (["The Power of Hidden Friends in C++"](https://blogs.accu.org/?p=1974), June 2019.)

----

The other day I watched Matthew Fleming's C++Now 2019 talk,
["An Alternate Smart Pointer Hierarchy."](https://www.youtube.com/watch?v=Hs0CA4vIcvk)
He starts with a shout-out to little old me! Thanks. I'm sorry I wasn't in the audience.
(I was at Gašper Ažman's ["Points of Order"](https://www.youtube.com/watch?v=WbW8A5QXn5I) at the time.)

The highlights of Matthew's talk include:

- @9:10 — Matthew quotes Louis Brandy (["Curiously Recurring C++ Bugs at Facebook,"](https://www.youtube.com/watch?v=lkgszkPnV8g&t=20m57s)
  CppCon 2017): "Is `shared_ptr` thread-safe? If you have to ask, the answer is _no_." Hear hear. (See also:
  "If you can't spot the sucker, the sucker is you.") I have another similar mantra, which I will probably
  blog about at some point: "What does `volatile` do? It makes reads and writes _really happen._ If you don't know
  what the phrase _really happen_ means in the context of your computer, then you shouldn't be using `volatile`."

- @35:50 — If you `=delete` a constructor to improve your error messages, you *must* make the deleted constructor
  `explicit`; otherwise it will pop in and ambiguate your overload resolution at funny times!
  Matthew doesn't show an example in the talk, but [here's one](https://godbolt.org/z/wI6ABg).

- @50:55 — Working around C++'s lack of [covariant return types](/blog/2019/01/20/covariance-and-contravariance/)
  for smart-pointer return types (as opposed to native-pointer return types).

----

I've also now watched Conor Hoekstra's extremely prize-winning talk ["Algorithm Intuition."](https://www.youtube.com/watch?v=48gV1SNm3WA)
It's worth the watch! Personally, I would take many of his stylistic recommendations with a huge grain of salt,
but it's a very entertaining and informative session.

I think it might not be coincidental that Jonathan Boccara released two blog posts on `std::is_permutation`
([1](https://www.fluentcpp.com/2019/06/25/understanding-the-implementation-of-stdis_permutation/),
[2](https://www.fluentcpp.com/2019/06/28/how-to-use-is_permutation-on-collections-of-different-types/))
not too long after Conor's talk hit YouTube. That's one of the algorithms that
Conor [claimed](https://www.youtube.com/watch?v=48gV1SNm3WA&t=28m18s)
one could implement in terms of `std::reduce` (a.k.a. "fold").
I think what that would look like is something like this:

    template<class FwdIt1, class FwdIt2>
    bool is_permutation(FwdIt1 first1, FwdIt1 last1,
                        FwdIt2 first2, FwdIt2 last2)
    {
        return std::reduce(
            first1, last1,
            true,
            [&](bool acc, const auto& value) {
                if (acc) {
                    if (auto ct = std::count(first2, last2, value)) {
                        return ct == std::count(first1, last1, value);
                    }
                }
                return false;
            }
        );
    }

which is a really terrible $$O(n^3)$$ algorithm, despite my clever attempts at optimization.
This is one of my pet peeves about many of the "higher-level" STL algorithms:
their computational complexity is always optimizing for _something_, but it may not be for the thing you
really care about. `std::is_permutation` optimizes for memory usage at the expense of running time.
`std::sort` optimizes for number of comparisons, at the expense of number of move-constructions. That kind of thing.

However, it is still pretty cool that both GCC and Clang are able to
[constant-fold this entire function away](https://godbolt.org/z/-N1p8W) at compile time.

----

I missed "Algorithm Intuition" at C++Now because I was planning to go see Ryan Dougherty's
["Experiences in Teaching Modern C++ to Beginners"](https://www.youtube.com/watch?v=GV1r7uJkPH4) instead.
And then I missed Ryan's talk because I had just finished my own "Trivially Relocatable" talk and ended up
in a deep hallway discussion that ended up consuming the whole time slot. But I caught Ryan's talk on YouTube,
and I guess I didn't miss too much — it feels kind of light on takeaways, and what takeaways there are
mostly just reinforce my preconceived notions. For example:

- Yes, we still have to teach pointers and arrays. Students are going to see them in real life,
  so they have to know what they mean and what they do. (And how C++ arrays are different from Java
  arrays!)

- Students mostly learn by example, and by question-and-answer.

- Students with no prior programming experience will omit curly braces and end up with confusing bugs,
  unless you set an immaculate example. Curly-brace all your control flow structures, people, even in slide code!

I thought it was counterintuitive that Ryan prompts his students (non–CS majors, all) to watch CppCon talks
and give reports on them. I would expect the vast majority of CppCon talks to be way over the head of the
average CS 101 student. (But maybe less so at CppCon 2019, thanks to its [Back to Basics track](https://cppcon.org/staff/)!)
