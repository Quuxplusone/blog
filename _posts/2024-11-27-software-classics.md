---
layout: post
title: "_The Elements of Programming Style_ (1974)"
date: 2024-11-27 00:01:00 +0000
tags:
  blog-roundup
  c++-style
  litclub
  rant
  slogans
---

In October's _Overload_ magazine [Chris Oldwood muses](https://accu.org/journals/overload/32/183/oldwood/)
on Italo Calvino's definition of what makes a book a "classic"
(["Why Read the Classics?"](https://www.penguin.co.uk/articles/2023/10/why-we-read-classics-italo-calvino), 1986).
Calvino's essay suggests fourteen different ways of describing the elephant that is
"a classic," including:

> A classic is a book which with each re-reading offers as much of a sense
> of discovery as the first reading.

and:

> Classics are books which, the more we think we know them through hearsay,
> the more original, unexpected, and innovative we find them when we actually
> read them.

and:

> A classic is a work which persists as background noise even when a present
> that is totally incompatible with it holds sway.

Two of Oldwood's personal candidates for classichood, as described in his _Overload_ essay,
are Kent Beck's [_Test-Driven Development: By Example_](https://amzn.to/3Aa9qUT) (2002)
and David Astels' [_Test-Driven Development: A Practical Guide_](https://amzn.to/40qypOa) (2003).
I've never read either of those, so I can't say what I think of those choices — but
reading Oldwood's essay did remind me that for quite a while now I've been intending
to re-read two of my own candidates for classichood in the computer-programming field:
Kernighan and Ritchie's _The C Programming Language_ (1978) and
Kernighan and Plauger's [_The Elements of Programming Style_](https://amzn.to/4fm9rnF) (1974).
And so the other week I did sit down and re-read the latter.

It holds up.


## The Elements of Programming Style

Kernighan and Plauger explicitly modeled their book on that other "little book,"
[_The Elements of Style_](https://en.wikipedia.org/wiki/The_Elements_of_Style) (Strunk and White, 1959).
But the format differs slightly: S&W present first the rule and then the examples; K&P present
first the example and then the rule. For example, S&W begin their fifteenth item with the
general principle

> Express co-ordinate ideas in similar form.

and follow up with concrete examples, such as:

<table>
<tr><td>
Formerly, science was taught by the textbook method, while now the laboratory method is employed.
</td><td>
Formerly, science was taught by the textbook method; now it is taught by the laboratory method.
</td></tr>
</table>

"The left-hand version gives the impression that the writer is undecided or timid;
he seems unable or afraid to choose one form of expression and hold to it. The right-hand
version shows that the writer has at least made his choice and abided by it."

K&P, on the other hand, always begin with a concrete example — all their examples were
taken from actual published textbooks! — such as:

<table style="font-size:80%;">
<tr><td>Original</td><td>Improved</td></tr>
<tr><td><pre>IF A>B THEN DO;
LARGE=A;
GO TO CHECK;
END;
LARGE=B;
CHECK: IF LARGE>C THEN GO TO OUTPUT;
LARGE=C;
OUTPUT: ...</pre></td><td><pre>LARGE = A;
IF B>LARGE THEN LARGE = B;
IF C>LARGE THEN LARGE = C;</pre></td></tr>
</table>

and only then derive a general principle, such as:

> Say what you mean, simply and directly.

Whole swaths of _The Elements of Programming Style_ are informed by the
[structured programming](https://en.wikipedia.org/wiki/Structured_programming) wars of the ’60s —
recall that 1974 was only six years after Dijkstra's
["GO TO statement considered harmful"](https://homepages.cwi.nl/~storm/teaching/reader/Dijkstra68.pdf) (1968).
K&P counsel their reader to avoid Fortran's arithmetic `IF`, avoid `GOTO` statements in general,
modularize by proper use of subroutines, express multiway branches via `IF`/`ELSE IF`/`ELSE IF`,
and so on. But even in Chapter 3, "Structure," only four of the chapter's ten mantras have
to do with structured programming. That chapter also contains evergreen guidelines such as:

> Choose a data representation that makes the program simple.

> Write first in an easy-to-understand pseudo-language; then translate into
> whatever language you have to use.

> Write and test a big program in small pieces.

---

K&P's _The Elements of Programming Style_ is a good read for beginner and expert programmers alike.
But you know what? So is S&W's _The Elements of Style_ itself! Abelson and Sussman write in the
preface to [_Structure and Interpretation of Computer Programs_](https://amzn.to/3OpfX1s) (1984) that
"programs must be written for people to read, and only incidentally for machines to execute."
So, good advice to writers of English prose is often good advice to writers of computer programs.
The majority of Strunk and White's mantras are directly applicable to programming:

> Put statements in positive form.

> Use definite, specific, concrete language.

> Choose a suitable design and hold to it.

Even rules which are ostensibly about the vagaries of English syntax can be applied to programming,
if you look at them the right way. Consider S&W's rule
"Enclose parenthetic expressions between commas." Their discussion clarifies that this rule is
really about making sure the nesting of _syntax_ matches the nesting of _semantics_. Setting off a
clause with commas in English is like pulling a block of code out into a named function in C++:
it promises the reader that the code will continue basically to make sense even if you skim over that
inessential part for now.
"If the interruption to the flow of the sentence is but slight, the writer may safely omit the
commas" — that's manual inlining. "But whether the interruption be slight or
considerable, he must never omit one comma and leave the other" — that's talking about symmetry,
about the proper division of responsibilities, about RAII.

<table>
<tr><td><b>Fully inlined</b><br><br>Marjorie's husband Colonel Nelson paid us a visit yesterday.</td><td style="font-size:80%;"><pre>std::pair&lt;int, int> get_dims() {
  int i, j;
  FILE *fp = fopen("input.txt", "r");
  if (fscanf(fp, "%d%d", &i, &j) != 2) {
    abort();
  }
  fclose(fp);
  return {i, j};
}</pre></td></tr>
<tr><td><b>Consigned to a parenthetical clause/function</b><br><br>Marjorie's husband, Colonel Nelson, paid us a visit yesterday.</td><td style="font-size:80%;"><pre>void read_dims(FILE *fp, int *i, int *j) {
  if (fscanf(fp, "%d%d", i, j) != 2) {
    abort();
  }
}
std::pair&lt;int, int> get_dims() {
  int i, j;
  FILE *fp = fopen("input.txt", "r");
  read_dims(fp, &i, &j);
  fclose(fp);
  return {i, j};
}</pre></td></tr>
<tr><td><b>Improperly divided</b><br><br>Marjorie's husband, Colonel Nelson paid us a visit yesterday.</td><td style="font-size:80%;"><pre>void read_dims(FILE *fp, int *i, int *j) {
  if (fscanf(fp, "%d%d", i, j) != 2) {
    abort();
  }
  fclose(fp);
}
std::pair&lt;int, int> get_dims() {
  int i, j;
  FILE *fp = fopen("input.txt", "r");
  read_dims(fp, &i, &j);
  return {i, j};
}</pre></td></tr>
</table>

The third of these snippets is still _intelligible_, in the sense that it has the same semantics
as far as the compiler is concerned. The programmer, like the novice writer, might complain:
"You still know what it _means!_ What do these little syntactic nits matter, anyway?"
But either of the first two is _stylistically correct_ in a way that the third simply is not:
the third makes an educated reader stumble and look twice.


## Closing thoughts

Both of my programming-classic picks (K&P and K&R) are short books:
just 220 and 140 pages, respectively. I'd be hard-pressed to name a similarly short
(or good) programming book from the current century.
Classic fiction, on the other hand, tends long,
from the _Odyssey_ to _Don Quixote_ to _Oliver Twist_, while modern fiction condenses —
not just its sentences but its scope.

> A classic is the term given to any book which comes to represent the whole universe, a book on a par with ancient talismans.

(In the _Atlantic_ this month, Charles McGrath writes of Alan Hollingshead's
[_The Stranger's Child_](https://amzn.to/3AgIi6C) — a 600-page novel whose
title comes from [a 200-page poem](https://en.wikipedia.org/wiki/In_Memoriam_A.H.H.) —
that in this book "people age, in ways that novels seldom portray anymore."
Compare Bertram Brooker's essay ["Idolaters of Brevity"](https://www.jstor.org/stable/27534650) (1931):
"Literature, in America, seems doomed to be brief. With the newspapers and
short-story magazines by day, and the radio by night, all engulfing the
frantic production of our scurrying scribblers, there seems to be little
hope of America producing many works of grand proportions.")

Speaking of curmudgeonly takes on modern literature,
in [_Boom Town_](https://amzn.to/4eXvukI) (2022) Garrison Keillor writes:

> Dickens was a good-natured writer and so he's been on the back shelf for ages.
> He created heroes you'd enjoy having dinner with, which is practically unknown
> in modern fiction. These days the reader dines alone in a cave and the
> waiters are weird with guttural voices and the food tastes like someone
> spit in it and there is never dessert.

I quote that here because you never know when I'll want to recover it later.
As Socrates said to [Phaedrus](https://www.gutenberg.org/files/1636/1636-h/1636-h.htm)
(and which I _also_ record here before I forget where _it_ came from):

> In the garden of letters he will sow and plant, but only for the sake of
> recreation and amusement; he will write them down as memorials to be
> treasured against the forgetfulness of old age, by himself, or by any
> other old man who is treading the same path.

Compare Plutarch's [_Life of Alexander_ 7](https://penelope.uchicago.edu/Thayer/E/Roman/Texts/Plutarch/Lives/Alexander*/3.html#7):

> [Aristotle said] the doctrines of which he spoke were both published
> and not published; for in truth his treatise on metaphysics was of no use
> for those who would either teach or learn the science, but was written
> as a memorandum for those already trained therein.
