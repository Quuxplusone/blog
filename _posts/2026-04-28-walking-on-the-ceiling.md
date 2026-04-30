---
layout: post
title: "_Adventure:_ Walking on the ceiling"
date: 2026-04-28 00:01:00 +0000
tags:
  adventure
excerpt: |
  On 2012-12-01 I wrote to Don Woods (in a postscript to a production update
  on [_Colossal Cave: The Board Game_](https://boardgamegeek.com/boardgame/121751/colossal-cave-the-board-game)):

  > By the way, I just noticed last week that in "Adventure", in the Hall
  > of the Mountain King, the directions NORTH and LEFT are synonyms, as
  > are SOUTH and RIGHT... as are WEST and FORWARD!  West being forward
  > makes sense, if the Hall of Mists is back to the east; but for the
  > rest I suppose the adventurer must be walking on the ceiling. :)  This
  > little mixup is present all the way back to
  > [Crowther's code](https://github.com/Quuxplusone/Advent/blob/master/CROW0005/advdat.77-03-11#L249-L253).
  > I just thought it was funny that nobody had commented on it before, as
  > far as I know.
---

On 2012-12-01 I wrote to Don Woods (in a postscript to a production update
on [_Colossal Cave: The Board Game_](https://boardgamegeek.com/boardgame/121751/colossal-cave-the-board-game)):

> By the way, I just noticed last week that in "Adventure", in the Hall
> of the Mountain King, the directions NORTH and LEFT are synonyms, as
> are SOUTH and RIGHT... as are WEST and FORWARD!  West being forward
> makes sense, if the Hall of Mists is back to the east; but for the
> rest I suppose the adventurer must be walking on the ceiling. :)  This
> little mixup is present all the way back to
> [Crowther's code](https://github.com/Quuxplusone/Advent/blob/master/CROW0005/advdat.77-03-11#L249-L253).
> I just thought it was funny that nobody had commented on it before, as
> far as I know.

Don Woods wrote back the same day:

> Yes, I vaguely recall finding that at some point.  It was wrong in my
> version 1 but got fixed somewhere between there and my version 2.5.

Indeed, where WOOD0350 (circa 1977) [has](https://github.com/Quuxplusone/Advent/blob/master/WOOD0350/advent.dat#L447-L454):

    19    311028  45  36   # 45=N 36=LEFT
    19    311029  46  37   # 46=S 37=RIGHT
    19    311030  44  7    # 44=W  7=FORWA

WOOD0430 (his expanded _Adventure 2.5_, circa 1995) [has](https://github.com/Quuxplusone/Advent/blob/master/WOOD0430/adventure.text#L526-L533):

    19    311028  45  37   # 45=N 37=RIGHT
    19    311029  46  36   # 46=S 36=LEFT
    19    311030  44  7    # 44=W  7=FORWA

The original inconsistency is preserved, without comment, in
KNUT0350, GIBI0375 (_Original Adventure_), LUPI0440, PLAT0550,
LONG0751, and SMIT0370 (Georgia Tech _FunAdv_).

[ROBE0665](https://cs.stanford.edu/people/eroberts/Adventure/) (_Wellesley Adventure_)
and [ARNA0770](https://mipmip.org/adv770/adv770.php) eliminate the
inconsistency — albeit not in an obviously purposeful way — by simply
not recognizing LEFT, RIGHT, and FORWARD as exits from the Hall of the
Mountain King.

ROBE0665 has a superficially similar slip-up at Three-Opening Arch:

    265     To the east stands a wide dark arch opening into three
    265     passages.  All lead eastwards; but the left-handed passage
    265     plunges down, while the right-hand climbs up, and the middle
    265     way seems to run on, smooth and level but very narrow.
    265     To the north of the great arch stands a stone door, half open.
    265     To the west the passage fades into darkness.

    265     0 66   LEFT  NE UP
    265     0 34   RIGHT SE DOWN
    265     0 273  EAST

Here LEFT correctly matches NE, but goes UP where the room description says "plunges down";
and RIGHT correctly matches SE, but goes DOWN where the room description says "climbs up."
I reported that bug to Eric Roberts on 2025-04-23, although at that time I don't think I'd noticed
that LEFT and RIGHT actually worked correctly in that location, and that it was only the UP
and DOWN directions that were wrong.

---

See also:

* ["A bug in _Adventure_'s endgame"](/blog/2020/02/06/water-bottle-bug/) (2020-02-06)
