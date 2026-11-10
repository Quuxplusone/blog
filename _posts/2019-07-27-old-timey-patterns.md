---
layout: post
title: "A cellular automaton that makes beautiful little patterns"
date: 2019-07-27 00:01:00 +0000
tags:
  cellular-automata
  digital-antiquaria
  esolang
  math
  pretty-pictures
  web
---

One of the things I pulled off of my old floppy disks
(["Making floppy disk images under OS X"](/blog/2019/07/26/disk-images-in-os-x/), 2019-07-26)
was a program called "PATTERNS.C". Its timestamp is 2000-09-24, or right about
the start of eleventh grade for me. What does it do? Well, first I'll explain it
mathematically, and then I'll show you what it looks like live.

It's a cellular automaton that runs in a 31-by-31 grid of cells.
Each of these cells can be in either of two states: "live" or "dead."
At each time-step: each cell with an odd number of live neighbors
becomes "live"; each cell with an even number of live neighbors becomes "dead."

The "neighbors" of a cell are its [Von Neumann neighborhood](https://en.wikipedia.org/wiki/Von_Neumann_neighborhood)
(just the four cardinal directions, no diagonals).

> In [Stephen Wolfram's idiosyncratic notation](https://www.stephenwolfram.com/publications/cellular-automata-complexity/pdfs/two-dimensional-cellular-automata.pdf),
> this is "the five-cell outer-totalistic cellular automaton with code $$\tilde{C}$$ = 204."
> [It appears](https://git.zipcode.rocks/kristofer/ImportantPapers/raw/branch/master/Wolfram-NKS-Ch12-PrinCompEquiv.pdf)
> that Wolfram even mentions this specific automaton by name, on page 824 of
> [_A New Kind of Science_](https://amzn.to/2LJfZTp) (2002).

In "PATTERNS.C", the grid does not wrap; instead, cells at the edge of the grid
act as if their missing neighbors are "dead." This will be important in a minute.

In "PATTERNS.C", I actually used four states: the "dead" state and three
different "live" states. The different "live" states are used to track the "age"
of a living cell. On each turn in which a cell doesn't move into the "dead" state,
it moves to the next greater "live" state; unless it has already reached state 3,
in which case it just remains there. The extra states don't change the
mathematical properties of the automaton; they just add some visual interest.

The sad part: No matter what pattern of cells you start with, if you
run the automaton for exactly 32 time-steps, you'll wind up with a grid consisting
entirely of "dead" cells. Each living cell in the starting configuration sends out
"waves" that bounce off the boundaries of the grid and ultimately cancel each other
out.

Now the happy part! The configuration after exactly 31 time-steps
displays surprising, intricate, even beautiful symmetry.

<iframe src="/blog/code/2019-07-27-patterns.html" height="auto" onload="this.height = this.contentWindow.document.body.scrollHeight + 'px';">
[Click here to play with the PATTERNS.C automaton!](/blog/code/2019-07-27-patterns.html)
</iframe>

The vintage-2000 source code is [here](/blog/code/2019-07-27-patterns.c), and
the Javascript/Canvas port embedded in the iframe above is also downloadable
[here](/blog/code/2019-07-27-patterns.html).
