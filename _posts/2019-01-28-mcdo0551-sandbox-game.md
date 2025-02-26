---
layout: post
title: "Colossal Cave Adventure: open world challenge"
date: 2019-01-28 00:02:00 +0000
tags:
  adventure
  digital-antiquaria
  puzzles
  web
---

The other day @NextRoguelike tweeted this game idea:

<div class="fake-tweet">
  <div>
    <div class="fake-tweet-firstline">
      <div>
        <a href="https://twitter.com/NextRoguelike"><img src="/blog/images/twitter-avatar-NextRoguelike.jpg" class="fake-tweet-avatar"/></a>
      </div>
      <div class="fake-tweet-byline">Your Next Roguelike<br><span class="fake-tweet-light">@NextRoguelike</span></div>
      <a href="https://twitter.com/NextRoguelike/status/1084791562515726336"><img src="/blog/images/twitter-logo.svg"/></a>
    </div>
    <p>A mashup of Colossal Cave with a sandbox.</p>
    <p class="fake-tweet-light">7:37 AM · Jan 14, 2019</p>
  </div>
</div>

You know, that game kind of exists already — it's David Long's _Adventure 5_ (1979)!

This branch of the [_Adventure_ family tree](http://advent.jenandcal.familyds.org)
is best known today via Doug McDonald's 551-point _Adventure 6_ (1990). (Sadly, the 751-point
"New Adventure" written entirely by Long and distributed by CompuServe in the 1980s is
[lost forever](http://www.club.cc.cmu.edu/~ajo/in-search-of-LONG0751/readme.html) as far as anyone
— including Long himself — knows.)

Long's version was heavily inspired by _Zork_ (or MIT's "Dungeon" program, as it was known back then).
Starting from Don Woods' source code, Long added a rudimentary health system, containers and
containment, objects of different weights (and a puzzle based on weight), a vehicle, wearable items,
adjectives, prepositions (`PUT RARE BOOK IN SAFE`), and multi-noun commands (`GET BOOK AND LAMP`,
`DROP ALL`). All of this by extending Crowther and Woods' original Fortran code! This was quite an
impressive technical achievement, and makes for a rich and rewarding game —
definitely my favorite branch of the _Adventure_ tree.

But all this technical cleverness came with a price, or at least a funny little sidecar. Some of
Long's additions had little tiny bugs, which are exploitable by the player. As a result, many of
the properties that were solidly "invariant" in Woods' game are actually _not_ invariant in Long's.

Ever wanted to imitate [_Colossal Cave: The Board Game_](https://amzn.to/2CTdYx5)
and bring the friendly bear back to the Well House? Well, in the Fortran original of MCDO0551,
you can do it!

![The gang's all here.](/blog/images/2019-01-28-mcdo0551-sandbox-game.png)

This screenshot comes from Scott Healey's amazing emulation of the "bugs-included" Fortran game
at [gobberwarts.com](http://www.gobberwarts.com/index-551.html). You can also download the
Fortran source code from [my repo](https://github.com/Quuxplusone/Advent/tree/master/MCDO0551)
and build it yourself. Finally, for those readers with no sense of whimsy at all, you can play
my "bug-fixed" C port of MCDO0551 online [here](https://quuxplusone.github.io/Advent/play-551.html).

----

Puzzle: The screenshot above shows the bear and oyster (and, incidentally,
nothing _else_) in the Well House on turn 112. Can you achieve the same feat in under 100 moves?
[Test your solution on gobberwarts.com.](http://www.gobberwarts.com/index-551.html)
