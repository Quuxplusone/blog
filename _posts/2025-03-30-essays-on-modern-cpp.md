---
layout: post
title: "_Essays on Modern C++_ released"
date: 2025-03-30 00:01:00 +0000
tags:
  c++-learner-track
  litclub
  training
  typography
---

Way back in 2022, I got the idea to do the Martin Gardner thing — to collect a few of the best posts
I'd written for this blog and publish them as a proper paper book. I chickened out at the time,
but this month I decided the stars were right to revisit that old project and finally publish my
first collection of [_Essays on Modern C++_](https://leanpub.com/essays-on-modern-cpp).

Okay, "proper paper book" is an exaggeration: At least for now the book is available
only in PDF/ePub format through Leanpub.com. (This is the same platform used for direct sales
by [Jason Turner](https://leanpub.com/u/jason_turner) and
[Nicolai Josuttis](https://leanpub.com/u/josuttis), among others.) But I've got a print version
ready to go, if Leanpub ever starts offering print sales or if a publisher reaches out.

I see this collection of my old blog posts as perhaps of little interest to someone
who already has ready access to my blog; just as
[Gardner's collections](https://martin-gardner.org/jem/David-Langford.html) might have been
of little interest to someone who already had a full back catalog of his _Scientific American_ columns.
But if you were just waiting for a nicely typeset PDF to convince your coworker of a particular
pet peeve — well, maybe this book is for you. Also, to give a little extra incentive even to regular
readers of this blog (who are, after all, the most likely buyers of such an e-book), I
round out the baker's dozen with one previously unpublished chapter — a quick five pages
on the implementation of `std::tuple_cat`.

The essays collected in this first book are:

* ["`const` is a contract"](/blog/2019/01/03/const-is-a-contract/) (2019-01-03)
* ["How to erase from an STL container"](/blog/2020/07/08/erase-if/) (2020-07-08)
* ["What `=delete` means"](/blog/2021/10/17/equals-delete-means/) (2021-10-17)
* ["Value category is not lifetime"](/blog/2019/03/11/value-category-is-not-lifetime/) (2019-03-11)
* ["What is ADL?"](/blog/2019/04/26/what-is-adl/) (2019-04-26)
* ["What is the `std::swap` two-step?"](/blog/2020/07/11/the-std-swap-two-step/) (2020-07-11)
* ["How do `using namespace` directives work?"](/blog/2020/12/21/using-directive/) (2020-12-21)
* ["SCARY metafunctions"](/blog/2018/07/09/scary-metafunctions/) (2018-07-09)
* ["Iteration is better than recursion"](/blog/2018/07/23/metafilter/) (2018-07-23)
* "How is `tuple_cat` implemented?" (previously unpublished)
* ["Why don't Concepts do definition checking?"](/blog/2019/07/22/definition-checking-with-if-constexpr/) (2019-07-22)
* ["Why do we require `requires requires`?"](/blog/2019/01/15/requires-requires-is-like-noexcept-noexcept/) (2019-01-15)
* ["Concepts can't do quantifiers"](/blog/2020/08/10/concepts-cant-do-quantifiers/) (2020-08-10)

Each essay has been “remastered” for the 2020s (specifically, for 2022); for example, outdated references to
"C++2a" have been replaced with "C++20." The essays have also been remastered for print;
for example, inline code snippets replace the original posts' Godbolt links, and informative footnotes
replace or augment many of the blog's simple hyperlinks. The book also includes an exhaustive four-page
subject index.

Errata and comments are of course welcome: [send me an email!](mailto:arthur.j.odwyer@gmail.com)
