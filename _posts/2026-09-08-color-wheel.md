---
layout: post
title: "Color Wheel combinatorics"
date: 2026-09-08 00:01:00 +0000
tags:
  board-games
  help-wanted
  math
  puzzles
---

Over the weekend I was introduced to a Looney Labs pyramid game called
["Color Wheel."](https://www.looneylabs.com/content/color-wheel)
This is a solitaire (or purely cooperative) game similar to
[peg solitaire](https://en.wikipedia.org/wiki/Peg_solitaire). It's played
on a decagonal 40-space board, with three trios of pyramids in each of
seven colors: 63 pieces total.

After populating the board with 40 random pyramids from the stash such that
no two adjacent pyramids share a color, your goal is to use the fewest moves
to finish the game — where a "move" swaps two pyramids of the same color and/or
size, and the game is "finished" when each color occupies its own contiguous
region of the board.

The official rules encourage you to finish the game within 27 moves, mainly
because the _Pyramid Arcade_ box happens to contain 27 more pyramids, and
you use those to keep score.
If the game had said "within 23 moves," then you could have used
the 23 leftover colored pyramids to keep score instead.

<b>How many possible Color Wheel setups are there in total?</b>

A quick Monte Carlo experiment
([source](/blog/code/2026-09-08-color-wheel-counter.py))
suggests that there are about
$$2.003\times 10^{46}$$ distinct Color Wheel setups, up to rotation and reflection.
There are another $$2.195\times 10^{49}$$ distinct midgame positions (i.e. positions
flouting setup's no-color-adjacency criterion). Both kinds of positions
have an average branching factor of roughly 327 (standard deviation about 5).

<b>What fraction of Color Wheel setups are solvable within 27 moves? Within 23?</b>

I have no idea. Pretty trivially we can observe that every Color Wheel game
must take at least six moves to complete: The game starts with 40 connected components,
and the best possible move replaces 8 components with 2. Five perfect moves
could reduce the number of components to 10, but a solved board has even fewer
(no more than 7).

It's easy to construct a valid setup that's solvable in seven moves.
I doubt that any valid setup is solvable in six.

![A configuration solvable in seven moves](/blog/images/2026-09-08-solvable-in-7.jpg)

How about the number of moves that suffices to solve any setup — what's
[God's Number](https://mathworld.wolfram.com/GodsNumber.html)
for Color Wheel? I've got only another trivial observation: the 40 pyramids
on the board can be selection-sorted into any configuration at all using no more
than 39 unconstrained swaps, and any unconstrained swap can be simulated by at most
5 valid moves. (If the pieces match in size, swap them in one move; otherwise, if any
of the six pieces that share color with one and size with the other is on the board,
use three swaps; otherwise, find $$p_1, p_2$$ of the same size, $$p_1$$ sharing color
with one and $$p_2$$ sharing color with the other, and use five swaps.) That gives us
a comically loose upper bound — 195 moves certainly suffice.

<b>Challenge: Write a Color Wheel solver.</b>

Here's [a vibe-coded Color Wheel solver](/blog/code/2026-09-08-color-wheel-solver.py)
that can do the average board in about 33.8 moves (standard deviation about 4.5);
[the worst case](/blog/code/2026-09-08-worst-case.txt) I've seen so far took 50 moves.
This is dramatically worse than the rules' suggestion of 27 moves! Is that more readily
explained by the badness of this solver's heuristics, or by the conjecture that most
Color Wheel boards are not solvable in 27 moves?

![Histogram of solution lengths found by this solver](/blog/images/2026-09-08-solver-histogram.png)

The Python code that generated that plot was:

    import matplotlib.pyplot as plt
    plt.hist(values, bins=[x-0.5 for x in range(20, 52)], edgecolor="white", color="#4f81bd")
    plt.xticks(range(20, 51, 2))
    plt.xlabel("Number of moves taken by the heuristic solver")
    plt.show()
