---
layout: post
title: "Explainable Minesweeper and solvable nonograms"
date: 2025-07-06 00:01:00 +0000
tags:
  blog-roundup
  playable-games
  puzzles
---

Via Hacker News: ["Making Explainable Minesweeper"](https://sublevelgames.github.io/blogs/2025-07-06-making-explainable-minesweeper/)
(July 2025). By "explainable Minesweeper," the blogger means that we generate a Minesweeper level that is guaranteed
solvable by perfect play — that is, for which the solution never requires "guessing" at an unknown cell (and possibly losing
the game at that point). The blogger describes a procedure for creating partially explainable levels: simply generate
a random level, then simulate solving it with a computer player that knows only certain heuristics. If the computer player
completes the level without guessing, then it's good; otherwise, generate a new random level and repeat.

> This is merely a "procedure," because (unlike an "algorithm") it is not mathematically guaranteed to terminate.

Another way to make a Minesweeper game that never requires guessing is to change the rules of the game: any time the player
clicks a square that doesn't _necessarily_ contain a mine, let the game quietly shuffle around the mines so that that square
_won't_ contain a mine. Dave Kordalewski's [Optimistic Minesweeper](https://github.com/kord/optimistic-minesweeper)
([play here](https://minesweeper.therestinmotion.com/)) works this way — almost. He makes it so that whenever you're
_forced_ to guess, you won't hit a mine; but if you click randomly when the game thinks you should know better, you may
hit a mine. IMHO this makes it hard to tell whether the logic is working or not, because "whether the game thinks you should
know better" is not immediately obvious from the state of the board. I'd like to see a _really_ optimistic version of Minesweeper
that _always_ works its hardest to keep the player alive, up to the point where no safe distribution of mines can possibly
satisfy the already-revealed hints.

> If you create or find such a "really optimistic Minesweeper," please let me know!

Along the same lines, here's a site that invites the world to solve ["Every 5x5 Nonogram"](https://pixelogic.app/every-5x5-nonogram)
(Joel Riley, June 2025). You might think there should be exactly 2<sup>25</sup> = 33,554,432 five-by-five
[nonograms](https://en.wikipedia.org/wiki/Nonogram); but that's not the case, because some nonograms are ambiguous
and therefore lack a unique "solution." For example:

      1 1 1 1 2     1 1 1 1 2
    1 X . . . .   1 . X . . .
    1 . X . . .   1 . . X . .
    1 . . X . .   1 X . . . .
    2 . . . X X   2 . . . X X
    1 . . . . X   1 . . . . X

In fact there are exactly 25,309,575 solvable 5x5 nonograms; this is [OEIS sequence A242876](https://oeis.org/A242876). But Joel
uses the same procedure as above to reject nonograms that don't succumb to an "explainable" computer player: that is, each
5x5 grid must be solvable by repeated application of these eight "obvious foothold" tactics:

        0 . . . . . => _ _ _ _ _
    1 1 1 . . . . . => X _ X _ X
      2 1 . . . . . => . X . . .
      2 2 . . . . . => X X _ X X
        3 . . . . . => . . X . .
      3 1 . . . . . => X X X _ X
        4 . . . . . => . X X X .
        5 . . . . . => X X X X X

plus a few more equally "obvious" tactics, such as:

        1 . . X . . => _ _ X _ _
      2 1 . . . . X => . . . _ X

Here's an example of a 5x5 nonogram which is technically solvable — it has a unique solution — but which does
not offer any "obvious foothold" to begin with. (Massive hat tip to
[HN commenter "duskwuff"](https://news.ycombinator.com/item?id=44147420)!)

          1 1   1
        2 1 1 2 1
      2 . . . . .
      2 . . . . .
    1 1 . . . . .
      2 . . . . .
    1 1 . . . . .

This nonogram is one of the 333,064 technically solvable nonograms that you won't find anywhere on
["Every 5x5 Nonogram"](https://pixelogic.app/every-5x5-nonogram).

Hint: Cell B2, in the upper left, is a good place to start solving it.

---

Previously on this blog:

- "Meta-Sudoku" [I](/blog/2018/10/26/sudoku-stories/), [II](/blog/2018/11/18/meta-sudoku-round-2/), [III](/blog/2018/11/19/meta-sudoku-round-3/),
    [IV](/blog/2018/11/20/meta-sudoku-round-4/) (2018)

- ["Evirdle"](/blog/2022/02/27/evirdle/) (2022-02-27)
