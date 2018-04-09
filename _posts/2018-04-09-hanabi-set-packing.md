---
layout: post
title: 'Hanabi set-packing puzzle'
date: 2018-04-09 00:03:00 +0000
tags:
  board-games
  math
---

Background reading: my previous post on [computerized Hanabi hint strategies](/blog/2018/03/29/hat-guessing-in-hanabi).

Suppose our computer player is looking at the public information known about a
given player's hand. (Ignore the *actual contents* of the player's hand; we can't
use it because it's not public knowledge. Also ignore any information we know about
*other* players' hands.) For each of the cards in the hand, our knowledge about
that card's identity can be summarized as a table of possibilities (this is
`CardPossibilityTable` in [InfoBot's source code](https://github.com/Quuxplusone/Hanabi/blob/master/InfoBot.cc)).
In fact, let's not even consider the _value_ of the card, but merely its _color_.

For example, here's one possible table representing the public information about
the colors of the target player's hand:

|          | Red | White | Yellow | Green | Blue
| -------- | --- | ----- | ------ | ----- | ----
|  Newest  |  1  |    1  |    1   |    1  |  .
|          |  .  |    1  |    1   |    .  |  .
|          |  .  |    .  |    1   |    1  |  .
|  Oldest  |  1  |    .  |    .   |    .  |  .

This table reports that the player's oldest card is definitely red; their next card
is either yellow or green; their next card is either white or yellow; and their newest
card is definitely not blue.

Now consider all the color hints we could give this player. There are only (at most)
five such hints: "red", "white", "yellow", "green", and "blue".
Some of these hints might be known-impossible to give (for example, "blue" is definitely
not a valid hint in the example above). Some hints might be known-possible to give
(for example, "red" is definitely a valid hint in the example above).
In many cases, we won't know for sure whether a given hint (e.g. "white") is possible
or not, just from the public information.

For any given hand, we can _partition_ the five color hints
into one or more sets, such that each set in the partition 
definitely contains at least one valid hint.
For example, in the above example, a trivially correct partition is

    (red, white, yellow, green, blue)

but a better (higher-cardinality) partition is

    (red, blue), (white, yellow, green)

Notice that it doesn't matter into which subset we place the known-invalid hint
"blue". Also notice that the target player doesn't necessarily have any white cards
at all; nor any yellow cards at all; nor any green cards at all; but we know that he
must have at least one white *or* yellow *or* green card, and so the set
`(white, yellow, green)` must contain at least one valid hint, even though we
can't say for sure which of the three hints is the valid one.

Notice also that this partition is _maximum-cardinality_. It is possible
that the target player's
hand is composed entirely of red and yellow cards, such that there *are* only two
valid color hints. And we found a partition into two sets. Can't do better than that!

The puzzle is: to find an algorithm that can compute a _maximum-cardinality_ partition
for any input table of possibilities.


Solution
--------

This problem can be reduced to the problem of selecting a maximum-cardinality
_set of rows_ from the table, such that the _columnwise sum_ of all the selected
rows does not exceed `(1, 1, 1, 1, 1)`.

|                        |  Red  | White | Yellow | Green | Blue
| ---------------------- | ----- | ----- | ------ | ----- | ----
| Oldest                 |   1   |    1  |    1   |   1   |  .
|                        |   .   |    1  |    1   |   .   |  .
|                        |   .   |    .  |  **1** | **1** |  .
| Newest                 | **1** |    .  |    .   |   .   |  .
| ---------------------- | ----- | ----- | ------ | ----- | ----
| **Sum of bolded rows** |   1   |    .  |    1   |   1   |  .

Another way to express the "columnwise sum" constraint is to say that the
rows we pick must all be *disjoint*.

This is known as a [set packing problem](https://en.wikipedia.org/wiki/Set_packing),
and the general case (for arbitrarily many "colors" and arbitrarily many "cards in hand")
is known to be NP-complete.

Once we find our maximal disjoint set (which can be done by a brute-force search
of all 2<sup>4</sup> possible subsets of 4 rows), we turn each row into a set
of colors —

    (red), (yellow, green)

— and then produce a partition of the full color space by placing the untouched
colors (`blue` and `white`, in our example) into the partition at arbitrary
spots.

    (red, blue), (yellow, green, white)

Q.E.D.!
