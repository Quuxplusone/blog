---
layout: post
title: 'Chinese fonts, and METAFONT'
date: 2018-04-15 00:02:00 +0000
tags:
  blog-roundup
  knuth
  typography
---

A couple of really interesting and well-written articles on Quartz,
coincidentally by the same author:

- ["The long, incredibly tortuous, and fascinating process of creating a Chinese font", Nikhil Sonnad (2015-12-18)](https://qz.com/522079/the-long-incredibly-tortuous-and-fascinating-process-of-creating-a-chinese-font/)
- ["What fonts tell us about the global economics of the internet", Nikhil Sonnad (2014-07-29)](https://qz.com/237851/what-fonts-tell-us-about-the-global-economics-of-the-internet/)

I'm a big fan of Donald Knuth's book [_Computer Modern Typefaces_](https://amzn.to/2HCef9C),
Volume E of the "Computers and Typesetting" series. (For those keeping track,
the first four volumes are [the TeXbook](https://amzn.to/2H2QdYu), [TeX itself](https://amzn.to/2EOkDHZ),
[the METAFONTbook](https://amzn.to/2HB7FjE), and [METAFONT itself](https://amzn.to/2vi2kLF) —
and Knuth has actually finished this book series!) It really shows what goes into crafting a font
for Latin typography, and also for mathematical typography; but Chinese typography is something
else again. Sonnad's excellent article above doesn't really get into what kind of software the
Chinese font designers are using for their work.

I was a big fan of METAFONT back in the day — I mean I guess I still am; I just haven't had much cause
to use it lately. METAFONT is essentially a compiler that accepts "programs" for drawing shapes and
compiles them into fonts usable by TeX. These "programs" look something like this:

    beginchar("w",9.5u#,asc_height#,0); "Welsh w";
     pickup oval.nib;
     rt x1=w-u; top y1=x_height;
     lft x3=u; y3=.5x_height;
     x2=.5[x1,x3]; bot y2=-o;
     pickup rect.nib; top y4=h;
     z4=z1+whatever * dir (90+pentilt);
     z3d=(x3-x2,1.5*y3-y2);
     path ovpath;
     ovpath = z1..{-dir pentilt}z2..tension 2.0..{dir (90+pentilt)}z3
              ..{dir pentilt}z4;
     pickup oval.nib; draw ovpath;
     pickup rect.nib; draw point 3 of ovpath;
    endchar;

This is a piece of a program I wrote back in March 2007 to draw the character better known as
[`U+1EFD LATIN SMALL LETTER MIDDLE-WELSH V`](http://www.fileformat.info/info/unicode/char/1efd/index.htm).
I forget why I wanted to do that. Maybe just to noodle around with METAFONT. Anyway, [here's](/blog/code/welsh-w.sh)
the whole program with enough context to render a PDF that looks like this, where the non-Latin character
is the one produced by the above snippet:

![I have no idea what this says, if anything](/blog/images/welsh-w.png)

By the way, METAFONT is the only programming language I know in which `whatever` is a keyword.
See also: [VALGOL](http://web.mit.edu/freebsd/head/games/fortune/datfiles/fortunes).
