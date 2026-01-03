---
layout: post
title: '_Infinite Craft_'
date: 2024-02-08 00:01:00 +0000
tags:
  ai
  hyperrogue
  playable-games
  web
---

I'm enjoying the heck out of Neal Agarwal's web game [_Infinite Craft_](https://neal.fun/infinite-craft/).
This is a ["crafting game"](https://gaming.stackexchange.com/questions/26441/what-does-crafting-imply-across-games)
where you start with just four elements and repeatedly combine pairs of elements to create new elements —
for example, Water plus Fire equals Steam. The clever gimmick is that instead of hard-coding a finite number
of possible combinations, _Infinite Craft_'s backend (certainly looks as if it) simply asks an LLM what
the result ought to be. So the combinations literally never run out.

Here's a partial map of the starting elements' neighborhood:

![A partial local neighborhood of the starting elements](/blog/images/2024-02-08-local-map.png)

The game's UI is quite different on mobile versus desktop; I like the mobile version much better.
It's also easy to create your own UI for it, e.g. at the command line using Python `requests` to
hit the `neal.fun/api/infinite-craft/pair` API endpoint. (Out of respect for his backend, I'll refrain
from posting the _exact_ code. He's got what looks like a proper rate-limiter, though.)

The cleverest part of the design is that the backend doesn't go to the LLM every time; that would be
expensive and slow. The backend (certainly looks as if it) keeps the entire history of the world in something
like [memcached](https://memcached.org/about). Every time you combine a pair, the backend looks to see if
that pair has ever been combined before, and if so, it serves you the cached response. Only completely
new pairings are sent to the LLM, and then the response cached for posterity. This means that the same
pairing will give the same result every time, for every player. Cutely, this also allows the
game to detect if you've just created a never-before-seen element! For example, everyone knows that
Water plus Fire equals Steam, but maybe you're the first person ever to see that Brunch plus
Sandwich equals Sandbrunch. (Hey, it's an LLM. They can't all be winners.) So then the UI can display
a little "First Discovery" flair on that element for you. Sadly, as of this writing the flair is shown
only on desktop, not on mobile.

_Infinite Craft_ is a sandbox game; but you can give yourself a goal, such as to create Math, or Potato,
or Hamlet. (I've achieved Hamlet. Math and — surprisingly — Potato remain elusive.) Play competitively
by racing to create an element in the shortest time, or with the smallest number of byproducts. Play
on one device by taking turns à la pick-up sticks or Jenga: the first player to fail to create a new
result on their turn loses. The possibilities, like the combinations, are endless!

![The shortest recipe for Don Quixote](/blog/images/2024-02-08-don-quixote.png)

Since the number of possible interactions grows quadratically with the number of elements you've
already found, _Infinite Craft_ reminds me of
[_HyperRogue_](https://en.wikipedia.org/wiki/HyperRogue)
in that the number of positions reachable in $$n$$ steps is counterintuitively large and the task of
retracing one's steps (say, to remember the recipe for Hamlet) is counterintuitively difficult.
