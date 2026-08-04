---
layout: post
title: "Möbius-strip crosswords"
date: 2026-08-04 00:01:00 +0000
tags:
  crosswords
  math
  pretty-pictures
  puzzles
excerpt: |
  Randall Rothenberg in the _New York Times_ (1988-08-10) writes that although ["Puzzle Makers Exchange Cross Words"](https://archive.is/K2Hw8)
  about the fall of ERNE and ESNE and the rise of XEROX and PEACE PLAN, one
  Robert Guilbert Sr.

  > was ignorant of the dispute two years ago when he developed a competitive crossword game
  > called Pago Pago and first became aware of crossword fans' passion for the pastime.
---

Randall Rothenberg in the _New York Times_ (1988-08-10) writes that although ["Puzzle Makers Exchange Cross Words"](https://archive.is/K2Hw8)
about the fall of ERNE and ESNE and the rise of XEROX and PEACE PLAN, one
Robert Guilbert Sr.

> was ignorant of the dispute two years ago when he developed a competitive crossword game
> called Pago Pago and first became aware of crossword fans' passion for the pastime.

"Guilbert invented the crossword-based game Pago Pago" is repeated in
[a news-or-magazine article](https://kolynychboss8.blogspot.com/2012/03/fan-passes-word-on-puzzle-hall-of-fame.html)
reprinted sans bibliographic data on a blog from 2012-03-14. That article begins
"Robert Guilbert Sr. wants to open a Crossword Puzzle Hall of Fame. Or — as Guilbert might say from force of habit —
a museum, shrine, repository, gallery or pantheon of immortals." (If you know where this article originally came from,
tell me and I'll update this post!) Anyway, that's all I can find in print about Pago Pago.

Helene Hovanec's article ["Robert Guilbert's Crossword Academy"](https://www.scribd.com/document/266289455/Robert-Guilbert-s-Crossword-Academy-by-Helene-Hovanec)
(_CROSSW RD_, Nov/Dec 1992) contains no text about Pago Pago, but does include this uncaptioned photograph
of Guilbert (1911–1990) hard at work:

![](/blog/images/2026-08-04-guilbert.jpg)

David Steinberg of the "Pre-Shortzian Puzzle Project" blog has mentioned Guilbert
[several times](http://www.preshortzianpuzzleproject.com/search/label/Robert%20Guilbert)
in connection with Guilbert's short-lived Crossword Academy, and even received in 2016
[a very nice blog comment from Guilbert's son Jonathan](http://www.preshortzianpuzzleproject.com/2014/11/american-crossword-puzzle-academy.html)
and [a little more via email](http://www.preshortzianpuzzleproject.com/2017/09/charles-gersch-will-weng-submission-guidelines.html#:~:text=Robert%20Guilbert%20Update:).
Jonathan wrote, in part:

> In the photo he can be seen working on a puzzle that he designed as a "Moebius Strip" — an
> "Infinity Crossword Puzzle" — a puzzle without a beginning and an end. He was going to have
> this game manufactured and marketed under the name "Pago Pago," I believe.

Personally I suspect that Jonathan's hunch was wrong: that the Möbius-strip puzzle and the "Pago Pago" puzzle-game
(whatever its nature was) were two separate projects. However, apparently on the strength
of Jonathan's guess, the Möbius–Pago Pago hypothesis was reproduced as fact in Adrienne Raphel's dissertation
["The Crossword Mentality in Modern Literature and Culture"](https://dash.harvard.edu/server/api/core/bitstreams/b332a223-4785-4322-b81c-5e504bd2fde5/content)
(Harvard, 2018):
"In 1986, Robert Guilbert, a former ad copywriter from Milwaukee who enjoyed
spending his retirement doing the puzzle, invented a game called Pago Pago, an infinite
crossword on a Möbius strip, and began to peddle it around to crossword fans."
The same sentence appears in Raphel's book
[_Thinking Inside the Box_](https://www.adrienneraphel.com/thinking-inside-the-box.html) (2020).
The game of telephone continues in Natan Last's [_Across the Universe_](/blog/2026/07/28/puzzle-miscellany/) (2025):
"he'd invented a game called Pago Pago, an infinite crossword on a Möbius strip, and wanted to hawk
it to the greats."

---

But what does a crossword on a Möbius strip look like, anyway? said I to myself.

Presumably it would be a crossword that loops around on itself from bottom to top (and vice versa).
A crossword that does that on all four edges would be "toroidal," like this one
(which I found on [Keith Calkins' "tori crosswords" page](https://www.andrews.edu/~calkins/math/webtexts/geom04xw.htm),
although I infer that Jeff Weeks was the author):

![A 4x4 toroidal crossword](/blog/images/2026-08-04-weeks-torus-crossword.svg)

[Jeff Weeks](https://en.wikipedia.org/wiki/Jeffrey_Weeks_(mathematician)) has made a bunch of these; see
[this PDF](https://momath.org/wp-content/uploads/me_handout_Weeks.pdf) from MoMath. In fact he's even made
crosswords "on a Klein bottle." Here the right and left edges connect as before,
while the top and bottom edges connect with a Möbius half-twist.

![A 5x5 Klein-bottle crossword containing two 9-letter entries](/blog/images/2026-08-04-weeks-kleinbottle-crossword.svg)

The above crossword is neat because despite being 5x5, its two longest entries are 9 letters each.
But it's less than perfectly satisfying in that half of the Across entries read right-to-left; and in
that if it actually wrapped around on itself with a half-twist, wouldn't the letters wind up backwards
too? Then 6-Down wouldn't be MESSENGER but
rather <nobr>ME<span style="display:inline-block;transform:scale(-1,1);">S</span><span style="display:inline-block;transform:scale(-1,1);">S</span><span style="display:inline-block;transform:scale(-1,1);">E</span><span style="display:inline-block;transform:scale(-1,1);">N</span><span style="display:inline-block;transform:scale(-1,1);">G</span>ER,</nobr> and
so on. (2-Down mirror-reverses very nicely, though!)

Now, a real mathematical Möbius strip, [says Martin Gardner](https://www.jstor.org/stable/24927595),

> is what topologists call "nonorientable."
> Imagine the strip to be a true surface of zero thickness.
> Embedded in this 2-space are [flat creatures](/blog/2021/11/05/the-fourth-dimension/)
> that are mirror-asymmetric (not
> identical with their mirror images). If such a creature moves once around the
> band to rejoin his fellows, he will have changed his parity and will have become
> a mirror reflection of his former self.

If our "flat creatures" are the letter-forms of the crossword entries, then when the crossword
loops around on itself the letter-forms will change parity: S becomes <span style="display:inline-block;transform:scale(-1,1);">S</span>,
and so on. So the only letters we seem to be able to use are the vertically symmetric ones, and
we'll have great difficulty constructing any crossword more complex than the following
infinite jest:

![A Möbius strip with "HAHAHA..." written vertically](/blog/images/2026-08-04-haha.jpg)

Or we could loop horizontally with horizontally symmetric letters (HEEHEEHEE...). But to
create entries that aren't infinitely long, we should create a _diagonal_ "ladder" of words instead
— as I fancy I see Robert Guilbert doing in that photo! If our "ladder" runs diagonally
down and left, like this:

      ABC
     DEF
    GHI

then we'll be able to use letters whose axis of symmetry is on that 45-degree line, such as (of course) O and X,
but also L: "LOX" horizontally will flip to produce "XOL" as one of the Down entries. In fact we can also use W,
which (if you draw it just right) flips to become E; H, which flips to become I (with serifs); C, which flips
to become U; and even D, which (with long serifs) flips to become a nice round capital A.
This gives us enough degrees of freedom to create the following Möbius-strip crossword:

![A Möbius crossword](/blog/images/2026-08-04-mobius-crossword.svg)

Print out this strip and tape the ends together with a half-twist. Fill it out with a heavy
ink marker and you'll find you only need to solve the first five clues — then you can flip the
strip over and read off the answers to the last five clues in the letter-forms that have bled
through the paper! (Or hold the back of the paper up to the light; or, for the really lazy,
just lay a mirror alongside it.)

Mine is not a _good_ crossword. Not only does it have awful fill, it repeats entries:
six of the Acrosses are repeated among the Downs. But it was the best three-letter ladder I could find
even with computer assistance. My dictionary of _possible_ answers consisted of only 23 pairs
of words. (Remember, every Across entry flips into a Down entry: if we have LAW across, we necessarily
get EDL down. And since EDL isn't a word, LAW isn't in our dictionary either.)
Words mirrorable this way of _more_ than three letters are rare as hen's teeth: HOWL flips into LEO I,
ALOE into WOLD, and ALOUD into A COLD; that's about all.

Can you come up with a better Möbius-strip crossword? or do you have additional information on
Robert Guilbert's Pago Pago project? [Email](mailto:arthur.j.odwyer@gmail.com) and tell me about it!

---

See also:

* ["Fully Schrödinger crosswords"](/blog/2025/01/03/double-crossword/) (2025-01-03)
