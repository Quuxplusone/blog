---
layout: post
title: "Mysteries of the map, partly answered"
date: 2026-01-19 00:01:00 +0000
tags:
  adventure
  digital-antiquaria
  rant
---

Previously: ["LONG0751 has been found"](/blog/2025/12/29/long0751/) (no spoilers),
["LONG0751: Mysteries of Donovan's map"](/blog/2025/12/30/long0751-map-mysteries/) (minor spoilers).
This post contains <b>major</b> spoilers for LONG0751!

This post is an expansion of something
[I wrote](https://forums.delphiforums.com/xyzzy/messages/?msg=527.11)
in the Delphi Colossal Cave forum a few days ago.

In my previous post I observed that there are some discrepancies and omissions in
Dennis Donovan's November 1980
[map](https://www.club.cc.cmu.edu/~ajo/in-search-of-LONG0751/compuserve-map/New-Adventure-Map.pdf),
compared to the game we recovered (`ADVENTURE < 6.1/ 3>, 14-Jan-82`).
For example, Donovan's map is missing what we might call the "helicopter arc," comprising
the Conservatory (letter of transit), helipad, silver mine (pumps, ingots),
Secret Garden (parachute, golden apple), and kitchen (dumbwaiter). I theorize that
this is no mistake: Donovan's map is based on a "missing link" version of _Adventure 6_
that didn't yet include those elements. But this "missing link" theory created
a few puzzles of its own:

## The mystery of the well bottom

In the recovered game, there are two ways into the castle. You can take the helicopter;
or, you can go past the leprechaun and boulder to the bottom of the dry well, where you
must PLAY FLUTE (thrice) to charm the rope and climb up it before it falls. (The well is
the only location where PLAY FLUTE charms the rope, as far as I know. For example it can't
help you ascend in the Rainbow Room.)
But the flute is found in the Conservatory, which doesn't exist on Donovan's map.
So how did this puzzle work in the "missing-link" game?

Today I'm pretty confident in the following theory: In the recovered game, you find an "iron handle"
at the bottom of the well, which is required in order to open the drawbridge, which is required
in order to defeat Ralph the Centipede. Donovan's map depicts a long straight object at the
bottom of the well; naturally I assumed this was the winch handle. However, it's not! In the missing-link
game, I theorize, _this is where you find the flute._ Look closely at the depicted object:
what I originally took for cross-hatching on an iron bar is now clearly suggestive of
finger holes in a flute!

That's just good puzzle design: the game presents
an impossible vertical ascent plus a flute, and all you have to do is make the mental connection to
the Hindu rope trick and go find some rope. It also lampshades why PLAY FLUTE works only in this
specific location. By comparison, the 1982 game's puzzle is unfairly difficult and unsatisfyingly
non-sequitur.

So, my theory goes, Long originally had the flute at the bottom of the well, and a fully
operational drawbridge (no handle needed). When he added the helicopter, he needed a place
to put the letter of transit. Naturally [it goes in Sam's piano](https://www.dailyscript.com/scripts/casablanca.pdf),
and the piano goes in a newly added Conservatory of Music. Naturally, then, the flute also moves to the Conservatory.
Long now kills two birds with one stone: To give the player some reason to move the boulder and visit the
well-bottom, and perhaps also to explain the long thin object depicted on Donovan's map, he dismantles the
drawbridge and places the handle down there.

## Stratigraphy via room numbers

The recovered game's data file ([on my GitHub here](https://github.com/Quuxplusone/Advent/blob/master/LONG0751/decompiled-advdat.txt))
lets us see the mapping from rooms and objects to numbers. The room numbers of LONG0751
are almost precisely a superset of the room numbers of ANON0501 and MCDO0551 (that is, except for those games'
additions), and those games' rooms are precise supersets of WOOD0350.

LONG0751's room numbers support my missing-link theory: after LONG0501 ends at room 238,
we see the approach to the castle (240–247, with the parking-lot puzzle), then the
bramblebush puzzle (249) and Sham Rock (250, without the briar pipe at first), and then
the castle and well as depicted by Donovan (251–272) intermingled with the Elephants'
Burial Ground and boulder puzzle (259, 261) and the Centipede's Lair (268).
Donovan's map corresponds to this version of the game, plus or minus
two nondescript castle hallways (273–274).

Then we get the Conservatory (275), helipad (276–283),
mine entrance (284–287), and Secret Garden (288–292). At this point there was still
no engineering room in the mines, no flooding, no statue or wooden box.
Then we get some rat-related ravine stuff (293–297) that makes me think the rat's lair
was originally somewhere in this area instead of beyond the mosquitos (still no briar
pipe at this point). Then the pumps and dumbwaiter setpieces (298–302). Finally
the rat's new lair (302–303).

## Stratigraphy via object numbers

The object numbers, on the other hand, seem to wiggle around much more.
Partly this seems to be due to Long's filling in old gaps with new objects.
Gaps have existed since the beginning: WOOD0350 has no objects at 41–49,
because Woods moved CROW0005's treasures to start contiguously at 50–64;
Woods also moved the steps and bird (née 6–7) to 7–8 in order to insert his
second black rod right after the first one.
Likewise it seems that Long kept shuffling items, creating gaps and backfilling
them with new items. Some of these shuffles I can explain pretty well, and
some I can't:

* WOOD0350's liquids-in-vessels (water and oil, in the bottle) are at 21–22.
    LONG0501 adds the wine and the cask, and moves them all to 81–86.
    LONG0501 fills the gap at 21–22 with the troll bridge (née 32) and clay bridge.
    LONG0751 fills 32 with the briar pipe.

* WOOD0350's rusty door is object 9. LONG0501 adds the tiny door and phone-booth
    door(s), moving all four to 41–44 and filling the gap at 9 with the pole.
    LONG0751 adds the castle door, vault door, and garden door, keeping the doors
    contiguous at 41–47 even though this means moving the flowers from 46 to 173
    and the cloak from 47 to 76.

* WOOD0350's golden chain is object 64. LONG0751 adds the iron chain, moving
    both to 202–203. LONG0751 fills 64 with the swamp's "buzzing sound."

* WOOD0350's set of keys is object 1. LONG0501 adds the tiny brass key (90)
    and inexplicably moves the keys to 102. LONG0751 fills 1 with the large boulder.

* WOOD0350's little bird is object 8. LONG0501 inexplicably moves it to 101.
    LONG0751 adds the black bird, keeping them contiguous at 101–102
    even though this further displaces the set of keys from 102 to 174.

* LONG0751 inexplicably moves objects 35–40 (bear through moss) to 212–217,
    leaving an unfilled gap.

Rather than try to paraphrase _all_ the changes, I've collated the object numbers
from all four versions (CROW0005, WOOD0350, LONG0501, LONG0751) in a CSV file
[here](/blog/code/2026-01-19-object-stratigraphy.csv). I imagine someone could
make a nice colorful flow diagram out of that data, somehow.
(If you do, [email me!](mailto:arthur.j.odwyer@gmail.com))

## The mystery of the candle

Donovan's map depicts a candle at the Altar, despite the fact that LONG0751 uses
the candle only in the dumbwaiter puzzle, which doesn't yet exist in Donovan's
"missing-link" game. What was the purpose of the candle in the missing-link game?
I have no good theory.

One wild-ass conjecture is that the missing-link game had the candle at either 74 or 76,
adjacent to the brambles (75), and the puzzle was simply to burn the brambles with
the candle. (Donovan depicts the candle as already lighted; and BURN BUSH still
"works" in LONG0751, although now it destroys the rose, which in this hypothetical
scenario didn't exist yet.) At some point Long added the matches (127–133)
and moved the candle thematically to 134, before adding the rose (135–136)
and changing the intended solution to the bramblebush puzzle. But why would Long
add the matches before the dumbwaiter existed, and long before the briar pipe?
Unless you used the briar pipe to smoke out the _centipede_ rather than the mosquitoes...
But this is conspiracy-theory reasoning: "How could X be true? Only if Y; and Y couldn't
be true unless Z," and soon you've reasoned your way far off the true path.

> I recently read Joe Klaas'
> [_Amelia Earhart Lives!_](https://archive.org/details/ameliaearhartliv00klaa) (1970),
> which exemplifies this kind of reasoning: Earhart couldn't have vanished unless
> she was shot down by the Japanese; but Earhart couldn't have been shot down
> by the Japanese unless she had been spy-photographing their secret base at Truk;
> but her plane couldn't have reached Truk unless it secretly had super-powered
> engines; and so on.

The sensible but boring theory is that the candle was just set-dressing,
and didn't _do_ anything until later.

## Vestigial treasures

The recovered game's data file ([on my GitHub here](https://github.com/Quuxplusone/Advent/blob/master/LONG0751/decompiled-advdat.txt))
contains definitions for five objects apparently inaccessible within the game itself:

    142     beautiful painting
    0000    A beautiful painting is hung on the wall.
    1000    There is a beautiful painting here!
    150     tarnhelm
    0000    There is a round iron helmet here.
    162     rare stamp
    0000    There is a rare postage stamp here!
    163     golden necklace
    0000    There is a golden necklace here!
    164     valuable furs
    0000    Nearby, some valuable furs have been strewn about.

Sadly none of these seem to be the item depicted in the Great Hall on Donovan's map.

The painting is placed among the golden apple (141), tapestry (143), fleece (144),
and perfume (145); perhaps these treasures were added in one go.

The tarnhelm ([a magic helmet that grants invisibility](https://en.wikipedia.org/wiki/Tarnhelm))
is placed provocatively beside the leprechaun paraphernalia (146, 148, 149).
But it might just as easily be that the stamp through shield started their
lives at 146–149, contributing to a contiguous block of 11 treasures (140–150), before
inexplicably moving to 162–165 and having their places filled with leprechaun stuff.
(Of those 11 treasures, Donovan's map clearly depicts only the shield; still,
the tusk, apple, tapestry, fleece, and perfume are not clearly absent.)

## Surprises in the recovered LONG0501

The recovered version of LONG0501 (`<Adventure--Experimental Version:5.0/6, NOV-78>`)
has some intriguing differences from the
[most recent common ancestor](https://en.wikipedia.org/wiki/Most_recent_common_ancestor)
of ANON0501, MCDO0551, ROBE0665, and LONG0751.

* The radium doesn't exist yet; instead the trident is in the Bubble Chamber,
    rather than in the cavern with waterfall (as in WOOD0350) or on the east side of
    Blue Grotto (as in MCDO0551/LONG0751).
    I guess the Bubble Chamber's punny name inspired the radium, rather than the
    other way around!

* The opal sphere doesn't exist yet; instead the ruby slippers are in the Crystal Palace,
    rather than over the Rainbow (Room) as they are in ANON0501/MCDO0551/LONG0751.
    This is still thematic: the Crystal Palace is where the yellow sandstone path begins.
    Donovan's map depicts them in the Rainbow Room itself, although this might be
    artistic license.

* The four-leafed clover doesn't exist yet; instead the flowers are on the knoll,
    rather than at Ocean Vista.

* In `6.1/3`, the verb WEAR bypasses the burden-checking code so that you can WEAR
    something too heavy to GET. It seems that `5.0/6` has no such bug.

## An unrelated observation on state-changing objects

In WOOD0350 and all its successors, whether the lamp is lighted or not is controlled
by changing the PROP value of the lamp. PROP values do not affect the short inventory
description of an object, only its long description when on the floor: both the lighted
and unlighted lamp are just "brass lantern." If you want an
object's inventory description to change, it needs to secretly be two items.
For example, the black bird (102) is rightly a different object from the
Maltese falcon (152), so as to give them two different inventory descriptions.

LONG0751's tasty/spicey food is also implemented as two objects (188, 159).
Personally, if I had written the game, I would have made "spiceyness" simply a
PROP value; but the current behavior (two inventory descriptions) depends upon
its secretly being two objects.

The soiled paper is two objects (190, 191) despite both having the _same_
inventory description. I can't think of a technical justification for that;
I think "valuableness" could easily have been encoded in the deed's PROP value
instead, just like it is for the cask of wine and the Ming vase.
