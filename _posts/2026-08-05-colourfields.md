---
layout: post
title: "Colourfields"
date: 2026-08-05 00:01:00 +0000
tags:
  math
  puzzles
excerpt: |
  Over on the (inexplicably login-walled) site "NewEnigma,"
  Keith Austin [writes](https://www.newenigma.com/enigma/view/1277/Colourfields):

  > Draw a 4-by-4 grid. Colour each square red or blue. Select any square, S, and write 1 in it.
  > Then write 1 in every square you can reach from S by a series of moves, where each move is from
  > a square to an adjacent, horizontally, vertically, or diagonally, square of the same colour [...]

  In brief: What is the maximum number of _queenwise-connected_ monochromatic regions you
  can make by coloring each square of an $$n\times n$$ grid either red or blue?
---

Over on the (inexplicably login-walled) site "NewEnigma,"
Keith Austin [writes](https://www.newenigma.com/enigma/view/1277/Colourfields):

> Draw a 4-by-4 grid. Colour each square red or blue. Select any square, S, and write 1 in it.
> Then write 1 in every square you can reach from S by a series of moves, where each move is from
> a square to an adjacent, horizontally, vertically, or diagonally, square of the same colour.
>
> Select any empty square, T, and write 2 in it. Then write 2 in every square you can reach from T
> by a series of moves. Repeat this procedure for 3 and then 4 and so on until every square has a
> number. The last number you write down is called the “score” for that colouring. If we imagine
> the grid is the map of a farm, then you have divided the map into fields, one field for each number.
>
> (1) What is the largest score possible?
>
> (2) If we work with a 5-by-5 grid, what is the largest score possible?

In brief: What is the maximum number of _queenwise-connected_ monochromatic regions you
can make by coloring each square of an $$n\times n$$ grid either red or blue? If we were asking about
rookwise-connected regions, the answer would obviously be $$n^2$$. But queenwise-connected is
at least slightly less obvious!

For the 5-by-5 grid, the "obvious" answer is

    rBrBr
    BBBBB
    rBrBr
    BBBBB
    rBrBr

for a total of 10 regions. But the answer for 4&times;4 is not simply

    rBrB
    BBBB
    rBrB
    BBBB

for a total of 5; you can in fact make 6 regions! (Once you've seen the trick it's hard to unsee;
but I didn't see it at first myself.)

Discussion on the [SeqFan](https://groups.google.com/g/seqfan/c/ocmDmccx5mI/m/L3KqPGbiAgAJ) mailing list
tentatively produced the sequence: 1, 2, 5, 6, 10, 12, 17, 19, 26, (28?)... Here's the pattern for
odd $$n$$, and a _conjectured_ pattern for all even $$n>4$$.

![](/blog/images/2026-08-05-colourfields.svg)

For odd $$n$$: $$\lceil n/2\rceil^2 + 1$$ regions.
For even $$n>4$$: $$\lceil n/2\rceil^2 + 3$$ regions.
