---
layout: post
title: 'Wordle-like games require two word lists'
date: 2025-05-23 00:01:00 +0000
tags:
  crosswords
  how-to
  playable-games
  wordle
---

Suppose you're implementing a "[Wordle](https://www.nytimes.com/games/wordle/index.html)-like" game such
as Juho Snellman's [Huewords](https://huewords.snellman.net/) or
Alan Bellows' [Omiword](https://www.omiword.com/). The defining principle
of such games (as I'm defining them here, anyway!) is that the solution to
the puzzle is an arrangement of English words which satisfies some criterion,
and the player arrives at that solution by _guessing_ various English words,
such that the game must differentiate "that's a valid word, but not the answer"
from "that sequence of letters isn't even a word."

All of Wordle, Huewords, and Omiword take the additional step of programmatically
generating new puzzles based on a randomized algorithm. (Wordle is the simplest
in all respects: its solution is a single word, and its puzzle-generation algorithm
is "pick a random word from this list of target words.")

If you're programming this kind of game, you absolutely must maintain _two_
word lists: a list of _valid_ words that may appear in the player's guesses,
and a smaller list of _target_ words that may appear in the solution to a
programmatically generated puzzle. The latter list will be a subset of the
former: just those words that are both "fair" and "appropriate" to appear in
an intended solution. By "fair" I mean that we don't want the answer
to the daily Wordle to be, say, ETUIS. Nobody knows that word; it's not fair!
By "appropriate" I mean that we don't want the answer to be... well, fill in
your favorite inappropriate five-letter word here. In crossword construction,
we speak of [the breakfast test](https://archive.is/okZow) (sometimes the "Sunday morning
breakfast" test), which means you shouldn't put anything in a crossword grid you
wouldn't want the proverbial little old lady to encounter over the breakfast
table.

> The breakfast-test criterion should be pretty stringent, and in crossword construction
> pertains to not just the clue but also its entry. Thus, a crossword shouldn't
> contain, say, COCKS, even if it's clued as "Roosters." The other example
> I would have used here was “ABO, even if it's clued as ‘Common blood types’&thinsp;”; but
> [actually that's quite common in the NYT these days](https://www.xwordinfo.com/Finder?w=ABO),
> even if it took a relative hiatus in the 1990s and hasn't been clued as
> "Australian native" since 1980.

However, such "inappropriate" words should absolutely remain on your list of
_valid_ words! When in doubt, put the word _on_ your valid-word list; and when in doubt,
take it _off_ your target-word list. This avoids the following failure modes:

* Suppose you have a questionable word (say, COCKS) _on_ your target-word list.
    Then it might appear in your puzzle's intended solution. But the player, if he's
    familiar with crosswording conventions, might reasonably assume that the intended
    solution can't possibly include COCKS — so, he won't pursue that direction,
    and might frustratedly conclude that the puzzle is unsolvable. In other
    words, correctly solving the puzzle should not require entering profanity.

* Suppose you have a questionable word absent from _both_ lists. Then when the
    player enters the word, your game will tell him it's "not a valid word,"
    which (depending on your player's level of dirty-mindedness) might just seem
    like a bug. For example, Huewords' valid word list originally lacked the
    word TRAMP — to Huewords' author primarily a breakfast-test-failing
    synonym for "skank," but to this player merely an innocuous synonym for "hobo"
    blessed by Frank Sinatra and Walt Disney.

* Suppose you have a questionable word absent from _both_ lists.
    When you generate a new puzzle using words from the _target_ word list,
    naturally you'll ensure its solution is unique: there must be no arrangement
    of words from the _valid_ word list that produces an equally acceptable solution.
    (This doesn't apply to Wordle, but does to Huewords and Omiword.)
    Now, if it's missing from your valid word list, it's possible that there _will_
    be an alternative solution involving the word SKANK! This will at least be a
    source of amusement to the player. In other words, any word missing from the
    valid word list may appear in a "cook" of the puzzle, which is just as interesting
    as the intended solution — perhaps even more so.

Essentially, your game should be liberal in permitting the player to enter profanity,
but profanity should never _help_ the player to find either a solution or a "cook."
This puts everyone's incentives in the right places. And the way you do this is by
having a restrictive target-word list plus a liberal valid-word list.
