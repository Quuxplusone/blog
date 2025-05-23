---
layout: post
title: 'Two old crosswords of mine'
date: 2021-08-24 00:01:00 +0000
tags:
  crosswords
  knuth
  typography
  word-ways
---

My lovely wife and I have just moved house, and while boxing up the contents
of my desk I ran across physical paper copies of two weakly themed crosswords
I made in 2011. Might as well immortalize them in the digital realm...

I created crosswords for Carnegie Mellon's student newspaper,
[_The Tartan_](https://thetartan.org/2006), circa 2003–2006.
In those years, I developed a suite of crossword-related software utilities
to help a crossword constructor (that is, me) with things like
laying out a symmetrical grid and even filling in the "fill" between
theme entries. (Filling in an irregular space with words from a dictionary
can be approached as an exact-cover problem, for which Donald Knuth's
[Dancing Links](https://en.wikipedia.org/wiki/Dancing_Links) algorithm
is very well suited.) However, _these_ two crosswords are dated
2011-11-10, and were definitely constructed without recourse to
`xword-fill`. They're also hand-numbered "#2" and "#3" respectively;
I no longer have any recollection of what happened to "#1."

I decided this would be a good time to pull out the old `xword-typeset`
utility and see if it still worked. After a little massaging of the
15-year-old code to deal with recent innovations such as "unused variable
warnings" and "UTF-8"... it works great! See the code on GitHub [here](https://github.com/Quuxplusone/xword).

    git clone https://github.com/Quuxplusone/xword
    cd xword
    make xword-typeset
    curl -O https://quuxplusone.github.io/blog/code/2021-08-24-crossword2.txt
    ./xword-typeset 2021-08-24-crossword2.txt > 2021-08-24-crossword2.tex
    pdflatex 2021-08-24-crossword2.tex
    open 2021-08-24-crossword2.pdf

See the printer-ready crossword PDFs here:

* [Crossword #2](/blog/code/2021-08-24-crossword2.pdf) ([.txt source](/blog/code/2021-08-24-crossword2.txt))

* [Crossword #3](/blog/code/2021-08-24-crossword3.pdf) ([.txt source](/blog/code/2021-08-24-crossword3.txt))

----

Another way these puzzles are products of their time: `CRT` appears as a fill entry
in both. According to [XwordInfo.com](https://www.xwordinfo.com/Finder),
2011 was right on the exact cusp where the _New York Times_ stopped
pairing `CRT` with clues like "Computer screen, for short" and started
giving it clues such as "Passé PC piece," "Old oscilloscope part, briefly,"
and "Bygone monitor, for short."

----

For those reading the code of `xword-typeset` and wondering what "HWEB" is:
It was my own prehistoric dialect of what today we'd call
[Markdown](https://en.wikipedia.org/wiki/Markdown). The syntax, like
the name, was just some mashup of bits of HTML and bits of
Knuth's CWEB. Its only relevance to `xword-typeset` is that to
italicize part of a clue you use `/slashes/`
(whereas modern Markdown uses `_underscores_`).
