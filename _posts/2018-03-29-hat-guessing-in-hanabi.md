---
layout: post
title: 'Hat-game strategies in Hanabi'
date: 2018-03-29 00:02:00 +0000
tags:
  board-games
  hofstadter
---

This past week or two, I've been once again playing around with my
[framework for _Hanabi_-playing bots](https://github.com/Quuxplusone/Hanabi/).
In particular I'd like to talk about a very bot-friendly strategy due to
[Jeff Wu](https://github.com/WuTheFWasThat/hanabi.rs).

_Hanabi_ ([BoardGameGeek page](https://boardgamegeek.com/boardgame/98778/hanabi))
is a card game that's been around since about 2010. It is a _pure cooperative_
game, in which all the players win or lose together as a team; there is no
competitive aspect. Mechanically, it's sort of like [Klondike](https://en.wikipedia.org/wiki/Klondike_(solitaire))
meets [Indian poker](https://en.wikipedia.org/wiki/Blind_man%27s_bluff_(poker)):
the players are trying to build up foundation piles with the values 1 to 5
in each of the five colors (suits) in the deck, but with the twist that the
players' hands are held facing outward, so each player can see all the cards
available to play... except their own.
On your turn, you may 
expend one of the group's eight "clock tokens" in order to give a hint to another
player about the contents of their hand;
or (blindly) play a card from your hand to the piles;
or (blindly) discard a card in order to regain one clock token for the team.

The only kind of hint allowed is to choose either a _color_ or a _value_ visible
in the target player's hand, and indicate to them exactly which of their cards
are of that color or value — and, by process of elimination, which cards must
*not* have that color, or that value. For example, a valid hint might be to say,
"Bob, your leftmost and rightmost cards are _green_."

Typically, a group of _Hanabi_ players will establish certain conventions in order
to complete their foundation piles and declare victory. Some conventions are
almost self-evident: _Don't discard if you could safely play a card instead._
_If you must discard, discard your oldest unknown card_ (because if nobody's
spending hints on it, it's likely worthless). Some conventions are non-trivial
but still highly intuitive for human beings: _If I'm giving you a hint about some
specific card, it's probably because I want you to play it right now._

(This reminds me of a thought-experiment I read a long time ago — perhaps by Douglas Hofstadter
— about something akin to the [categorical imperative](https://en.wikipedia.org/wiki/Categorical_imperative#The_First_Formulation:_Formula_of_Universality_and_the_Law_of_Nature)
but a bit less philosophical. In this experiment, you and a friend are both arriving
in, let's say, San Francisco; and you need to meet up together; but neither of you
have any means of communicating with each other. Where do you go, hoping that your
friend will also go there?

(Okay, that was a hard one. Suppose you had already agreed to meet at [City Hall](https://en.wikipedia.org/wiki/San_Francisco_City_Hall);
that's still a pretty big area that you could wander around for quite a while.
How would you maximize your chances of finding your friend? If you're like me,
you'd head straight for the main entrance and hang out right there. Why the
_main_ entrance in particular, as opposed to the side entrance or around back
or the second-floor restroom? _This convention is non-trivial but still
highly intuitive for human beings._)

Immanuel Kant on _Hanabi_:

> Act only according to that maxim whereby you can, at the same time, will that it
> should become a universal law.


## Hat-guessing and _Hanabi_

[Jeff Wu wrote a _Hanabi_ bot](https://github.com/WuTheFWasThat/hanabi.rs)
that uses a clever strategy to achieve perfect scores
about 90% of the time in a four-player game. [This BGG thread](https://boardgamegeek.com/thread/1551166/near-optimal-hanabi-strategy-extreme-amount-conven)
calls the strategy "hat-guessing," because of its similarity to a number of
[logic puzzles involving colored hats](https://en.wikipedia.org/wiki/Hat_puzzle).

> There are 10 prisoners. Each prisoner is assigned a hat with either one stripe, or no stripe.
> The prisoners are lined up single-file such that each can see the hats in front of him but
> not behind.
> Starting with the prisoner in the back of the line and moving forward, they must each,
> in turn, say "one" or "zero". If the number they say matches their own hat's number
> of stripes, they are released; if not, they are killed on the spot. A friendly guard
> warns them of this test beforehand and tells them that they can formulate
> a plan whereby 9 of the 10 prisoners will definitely survive. What is the plan?

Spoiler alert!

The prisoner at the back of the line has no information on his own hat at all. So,
he's going to be taking one for the team here. He counts up all the stripes he sees,
and loudly declares that sum (mod 2). So if he sees 3 stripes total, he'll say "One!"
(Followed, fifty percent of the time, by a gunshot.) The ninth prisoner looks forward
and counts 3 stripes, which is an odd number; so he knows that the prisoner
behind him (who shouted "One!") must have seen those same 3 stripes, and no fourth stripe
on his own hat. Therefore he confidently announces "Zero!" and is saved.
The next prisoner looks and sees only 2 stripes; he deduces that his own hat must
have one of the 3 stripes the previous two prisoners saw, confidently announces
"One!", and is saved. And so on down the line until 9.5 prisoners have been saved.

How does this translate into a bot-friendly _Hanabi_ strategy?

Let's say that each player wants to know whether their leftmost card is playable or not.
The player whose turn it is has no information on his own hand at all. So, he's going
to be taking one for the team here. He counts up all the playable leftmost cards he
sees, and loudly declares that sum (mod 2) in the form of a _hint_ — let's say, if
the sum (mod 2) is 0, he'll give a color hint, and if it's 1, he'll give a value hint.

    Alice's hand: 2r 3g 2y 1r
    Bob's hand:   4r 5w 1g 3y
    Carol's hand: 3y 1w 1b 5r
    Dan's hand:   2g 3r 3r 1y

    Current top cards: 1r 2y 1g 1b 1w

Alice sees the leftmost cards "4r, 3y, 2g." That's two playable cards. She gives
a color hint — doesn't matter to whom — let's say, "Dan, your middle two cards are red."

Bob sees the leftmost cards "3y, 2g" (and Alice's 2r, which doesn't count). That's
two playable cards, which agrees with the hint Alice gave. Bob realizes that his
leftmost card is *not* playable.

Carol sees the leftmost cards "2g, 4r" (and Alice's 2r, which doesn't count). That's
only one playable card, which disagrees with the hint Alice gave. Carol realizes
that her leftmost card *is* playable.

Dan sees "4r, 3y", which disagrees with Alice's hint, and realizes that his leftmost
card is playable as well.

Thus, with just one hint, Alice has managed to convey one bit of useful information
to *each* of the three other players! And of course she also told Dan quite a bit
about Dan's hand.

This convention is *not* practically useful for human beings, as far as I know —
we aren't that good at mental bit-twiddling — but it is great for computer players.
Jeff's contribution (besides a really clean and pleasant Rust implementation)
was the observation that we can squeeze a *lot* more bits of information into
Alice's hint, by formalizing and generalizing this idea!


## The generalized InfoBot strategy

All of the following steps use only the players' _common knowledge_, unless
otherwise stated.

- For each player, we devise a `HintStrategy`, which is an integer `k` along with
  a reversible mapping
  from the integers `[0..k)` to a set of hints that would (according to common
  knowledge) be legal for that player. In the simple strategy above, `k` was 2,
  and the mapping was "0: give a color hint. 1: give a value hint."

- Alice (whose turn it is) now has a set of `sum(k)` possible hints she could give,
  to unambiguously communicate any integer in `[0..sum(k))`. That is, if she wants
  to announce the number "0", she gives Bob a color hint. For "1", she gives Bob
  a value hint. For "2", she gives Carol a color hint. For "3", she gives Carol
  a value hint. For "4" and "5", she gives Dan hints. She can express any integer
  in `[0..6)`.

- For each player, using only common knowledge, we devise a prioritized list of
  `Questions` that that player
  would like to have answered. In the simple strategy above, each player only had
  one question: "Is my leftmost card playable? 0 for no, 1 for yes." But let's
  say that they'd also like to know whether their *rightmost* card was playable,
  too. That becomes the second `Question` in their list. Each player can have
  many questions, as long as the product of all the possible answers remains
  less than Alice's `sum(k)` — which in our example is `6`. (Remember, this is
  all based on common knowledge! Bob can compute his own `HintStrategy` and therefore
  his own `k` without any recourse to eyesight. He can also compute his own `Questions`,
  and also the `Questions` of anyone else at the table.)

- Alice (whose turn it is) considers each player's `Questions` and multiplies
  out the answers into one combined `Answer` per player. For example:

        Alice's hand: 2r 3g 2y 1r
        Bob's hand:   2r 5w 1g 3y  # this has changed from above
        Carol's hand: 3y 1w 1b 5r
        Dan's hand:   2g 3r 3r 1y

        Current top cards: 1r 2y 1g 1b 1w

  Alice's `sum(k)` is 6. Bob's leftmost and rightmost cards are both playable,
  so his `Answer` is 11<sub>2</sub> = `3`. Only Carol's leftmost card is playable,
  so her `Answer` is 10<sub>2</sub> = `2`. Dan's `Answer` is also `2`.

- Alice announces the sum of the answers (mod `k`). In this case, 3+2+2 is 7,
  which is 1 (mod 6), so Alice announces "1" by giving a value hint to Bob.

- Bob works out Carol's and Dan's `Answers` the same way Alice did and deduces
  that his own `Answer` must have been `3` = 11<sub>2</sub>, and thus that his
  leftmost and rightmost cards must be playable. He also makes note of Carol's
  and Dan's `Answers`, contributing to the store of common knowledge for
  future turns. Meanwhile, Carol and Dan are going through the same process.

This concludes the description of a generic hat-guessing hint system for _Hanabi_!

I call this "generic" because it is parameterized on `HintStrategy` and `Questions`.
We can improve our bot's performance by finding improvements to those two
parameters.

In his Rust implementation, Jeff Wu observed that we can _always_ increase `k`
to 3 per player, because with only three copies of each card in the deck and
four cards in hand, we are guaranteed to be able to give

- a color hint that touches the leftmost card,

- a value hint that touches the leftmost card, or

- a hint that _does not touch_ the leftmost card.

Furthermore, if it is common knowledge that a particular player's hand has
at least one color _besides_ the leftmost card's color, and at least one value
_besides_ the leftmost card's value, then we can increase `k` to 4 for that player!
Jeff's bot implements this improvement to `HintStrategy`, which means that his
equivalent of Alice is able to announce numbers ranging as high as `sum(k)=9` (worst case)
or `sum(k)=12` (common case) instead of our example's paltry `sum(k)=6`.

Jeff also improves on the selection of `Questions` for each player.

- The first kind of question he asks is, for each card, "is this card
  playable?" This consumes one bit of the `Answer` space for each card that
  isn't already known-unplayable. Also, we consider this kind of question
  only if this player has no already-known-playable card.

- The other kind of question he asks is, for each card, "what is its exact color
  and value?" But since there aren't usually enough bits of `Answer` space to
  get a clean answer, he buckets the possibilities into just as many buckets
  as there are possible `Answers` (plus one special bucket for any card that is
  "worthless" and can safely be discarded). So for example, if `sum(k)` is known
  to be `9`, we might say that answer "0" indicates "1w or 2r or 3y",
  and answer "1" indicates "2g or 2y", and answer "2" indicates "5w or 5r",
  and so on.

I've thought of other possible `Questions` that one could ask:

- "What is the index of my leftmost playable card?"

- "What is the index of my leftmost worthless card?"

And it ought to be possible to increase `k` even more, in common cases.
But in each case, either I haven't managed to implement my new idea correctly yet,
or I've implemented it only to find that it is outperformed by Jeff's original
`HintStrategy` and `Question` selections. His bot is really quite good!

The complete code in Rust can be seen [here](https://github.com/WuTheFWasThat/hanabi.rs/blob/master/src/strategies/information.rs),
and my C++ port [here](https://github.com/Quuxplusone/Hanabi/blob/master/InfoBot.cc).

Jeff adds that the following paper was helpful:

- ["How to Make the Perfect Fireworks Display: Two Strategies for Hanabi"](/blog/images/how-to-make-the-perfect-fireworks-display.pdf) (2015)

The following resources might also be interesting:

- ["Hanabi is NP-complete, Even for Cheaters who Look at Their Cards"](https://arxiv.org/abs/1603.01911) (Baffier, Chiu, Diez, et al., 2017)
- ["Playing Hanabi Near-Optimally"](http://www.mi.parisdescartes.fr/~bouzy/publications/bouzy-hanabi-2017.pdf) (Bouzy, 2017)
- [https://github.com/chikinn/hanabi](https://github.com/chikinn/hanabi) (several bots, in Python)

----

P.S. — I think I'd head to the cable car turntable on Powell Street.
Where did _you_ choose to meet up?
