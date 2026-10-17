---
layout: post
title: "_Paradise Lost_, diffed"
date: 2026-10-17 00:01:00 +0000
tags:
  litclub
  memes
  old-shit
  poetry
  transcription
  web
excerpt: |
  One of my book clubs will be starting Milton's _Paradise Lost_ soon.
  Preparing for that reading, I learned that there are actually two
  different versions of _Paradise Lost_: the “original” 1667 edition
  in ten books, and the 1674 revised edition in twelve.

  Are the differences between the two editions as significant as, say,
  the differences between the 1818 and 1831 editions of Mary Shelley's
  _Frankenstein_? To find out, I wanted a website similar to
  Eduardo Villuendas' [_Frankendiff.com_](https://frankendiff.com/).
  So, I made one.

  Presenting <i>[Paradise Lost, Diffed](https://quuxplusone.github.io/Wikisource/Milton/paradise-lost-diffed.html)!</i>
---

One of my book clubs will be starting Milton's _Paradise Lost_ soon.
Preparing for that reading, I learned that there are actually two
different versions of _Paradise Lost_: the “original” 1667 edition
in ten books, and the 1674 revised edition in twelve.

Are the differences between the two editions as significant as, say,
the differences between the 1818 and 1831 editions of Mary Shelley's
_Frankenstein_? To find out, I wanted a website similar to
Eduardo Villuendas' [_Frankendiff.com_](https://frankendiff.com/).
So, I made one.

Presenting <i>[Paradise Lost, Diffed](https://quuxplusone.github.io/Wikisource/Milton/paradise-lost-diffed.html)!</i>
Unlike _Frankendiff_, which was vibe-coded, this page was painstakingly
hand-crafted, starting from [Project Gutenberg e-text #20](https://www.gutenberg.org/ebooks/20)
and comparing it to the Internet Archive's scans of real books from 1668 and 1674.
(This turned up about 106 errata in e-text #20, which I have reported to PG.)
This resulted in a single 531-kilobyte plain HTML page — which sounds gigantic,
but is coincidentally just about the size of _Frankendiff_’s JavaScript.

The answer, by the way, is "no": the differences between the ten-book and
twelve-book editions of _Paradise Lost_ are extremely minor. I actually marked
with asterisks in the margin each instance where the difference affects the
sense of the verse; there are only 18, even counting instances like
"<strike>add</strike> <u>and</u> the humble Shrub"
(<a href="https://archive.org/details/paradiselostpoem00milt_1/page/n208">VII.322</a>),
which (1) has basically the same sense and (2) is probably a typographical error
in the first place.

![](/blog/images/2026-10-17-anakin-milton.jpg){: .meme}

As usual with this sort of thing, I'll offer a bug bounty: if you find
a mistake on that page, [send me an email](mailto:arthur.j.odwyer@gmail.com) and
I'll send you $5. (Per error that you're the first to report. I expect
these days that someone'll throw an LLM at it, so let me add an "AI rider":
if your agent sends me an unwieldy number of suggestions, I reserve the right
to look at them and fix any that are real, but if an obviously LLM-generated
list has more false positives than true positives, you won't get any bounty.)
