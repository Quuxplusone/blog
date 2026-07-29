---
layout: post
title: "What is 8÷2(2+2)?"
date: 2019-08-01 00:01:00 +0000
tags:
  math
  memes
---

Google's recommendation algorithm has been trying to get me interested in
[this clickbait](https://www.popularmechanics.com/science/math/a28569610/viral-math-problem-2019-solved/)
for a while. Most recently via Fox News, but via some other meme aggregator before that.
And I remember when "[8÷2(2+2)](https://knowyourmeme.com/memes/8222)" was spelled
"[6÷2(1+2)](https://www.reddit.com/r/AskEngineers/comments/5icxyd/whats_the_actual_correct_answer_for_6212/)" (2016),
and before that "[48÷2(9+3)](https://knowyourmeme.com/memes/48293)" (2011),
and before that... well, there's [nothing new under the sun](https://biblehub.com/ecclesiastes/1-9.htm).
This evening my uncle forwarded it to the math majors in the family,
so I figured I might as well put my response in a blog post and link to it.

The answer to a mathematician would be "1" — because $$2(2+2)$$ is $$2\cdot 4$$ is 8,
and then $$8\div 8$$ is 1.

However, there are two differences going on here between "blackboard math" and "computer-program math."
One is that computer programs like Excel don't use the $$\div$$ symbol for division; they use forward-slash.
Two is that most computer programs (definitely including Google Sheets; I can't speak for Excel itself)
don't treat expressions like `2x` or `2(x+1)` as multiplication.
You'd have to insert a multiplication operator, i.e., `2*x` or `2*(x+1)`.

So when you ask "what is $$8\div 2(2+2)$$", you're implicitly asking "what would a _human_ make
of this sequence of symbols," to which the answer is "eight divided by eight, i.e., one."

Where it gets its "confusion factor" from is that there are a lot of people out there who
I guess can't do math in their heads, so they try to cobble together a way to get the computer
to compute $$8\div 2(2+2)$$ for them... and then they have to deal with the two quirks above.
They'll likely deal with it by changing $$\div$$ to `/`, and inserting `*` between
the $$2$$ and the $$(2+2)$$. But when you _change_ the sequence of symbols, you _change_ the sequence's
meaning!

If you ask a computer programmer to evaluate `8/2*(2+2)`, they'll
say it's `(8/2)*(2+2)` is `4*4` is 16, because that's how the associativity and precedence
of the `/` and `*` operators works. (In most languages, anyway. See below!)
If you ask a mathematician to evaluate $$8/2\ast(2+2)$$, they'll probably ask what operation
is represented by $$x\ast y$$ — and when you say "multiplication," they might concur that
the answer is 16, but they'll encourage you to rewrite the expression in some clearer form,
such as $$\frac{8}{2}\cdot(2+2)$$, if that's really what you meant to express.

> [Communicating badly and then acting smug when you're misunderstood is not cleverness.](https://xkcd.com/169/)

---

By the way...

In the programming language [APL](https://en.wikipedia.org/wiki/APL_(programming_language)),
concatenation means literal _concatenation_; so
`8÷2(2+2)` is `8÷(2 4)` is `4 2` — that's a vector of two elements. Meanwhile, in APL,
`*` means exponentiation, not multiplication, and `/` means replication, not division;
so `8/2*(2+2)` is `(2 2 2 2 2 2 2 2)*4` is `16 16 16 16 16 16 16 16`.
You're welcome.

Wolfram Alpha — another computer program designed by mathematicians — takes the middle route.
[As of this writing](https://www.wolframalpha.com/input/?i=8÷2(2%2B2)), it interprets `8÷2(2+2)` as
$$\frac{8}{2}(2+2)$$ (that is, 16); but it explicitly shows you that it's rewriting the expression
as $$\frac{8}{2}(2+2)$$ in order to make sense of it, and (because it's interactive) gives you a
chance to rewrite the expression if that's not what you intended. Exactly the way a mathematician might!

---

See also:

* ["How old is Ann?"](/blog/2026/07/29/how-old-is-ann/) (2026-07-29)
