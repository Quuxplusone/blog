---
layout: post
title: "_Adventure:_ Is there light in the cobble crawl?"
date: 2026-04-30 00:01:00 +0000
tags:
  adventure
  digital-antiquaria
  war-stories
excerpt: |
  The original _Colossal Cave Adventure_ consists basically of a
  Fortran source file and a textual data file. These files would
  often travel from one installation to another via paper printouts:
  printed out at one site, typed in by hand at another.

  The lines of WOOD0350's Fortran source (intentionally or not)
  never exceed 80 columns regardless of your tab stop.
  But the data file fits within 80 columns only with a tab stop of four.
  With an eight-space tab stop, four lines of the data file exceed 80 columns:
---

The original _Colossal Cave Adventure_ consists basically of a
Fortran source file and a textual data file. These files would
often travel from one installation to another via paper printouts:
printed out at one site, typed in by hand at another.

The lines of WOOD0350's Fortran source (intentionally or not)
never exceed 80 columns regardless of your tab stop:

    $ detab -4 advent.for | awk '{print length}' | sort -n | uniq -c | tail -4
      48 77
      52 78
      35 79
       3 80
    $ detab -8 advent.for | awk '{print length}' | sort -n | uniq -c | tail -4
      58 77
      62 78
      39 79
       3 80

But the data file fits within 80 columns only with a tab stop of four.
With an eight-space tab stop, four lines of the data file exceed 80 columns:

    $ detab -4 advent.dat | awk '{print length}' | sort -n | uniq -c | tail -4
      55 71
      59 72
      67 73
      69 74
    $ detab -8 advent.dat | awk '{print length}' | sort -n | uniq -c | tail -4
      59 76
      66 77
      69 78
       4 82

These are the four offending lines. The first comes from section 3 (travel table)
and the rest from section 9 (bit flags).

    108     95556   43      45      46      47      48      49      50      29      30
    0       1       2       3       4       5       6       7       8       9       10
    7       42      43      44      45      46      47      48      49      50      51
    7       52      53      54      55      56      80      81      82      86      87

If your version of _Adventure_ has suffered truncation at the 80th column
at any point in its pedigree, you'd expect to see
(1) going DOWN from Witt's End is impossible; (2) the Cobble Crawl
does not have light; (3) entering maze room 51 or 87 resets the maze
hint counter.

I first became aware of this possibility in December 2025, when Mike Willegal
sent me a fan-fold printout of HORV0350 (a
[SEL32](https://en.wikipedia.org/wiki/Systems_Engineering_Laboratories)/[RTM](http://mnembler.com/ragooman/computers_mini_history.html)
port of WOOD0350 most recently touched by Ned Horvath) that he'd kept since March 1979.
In that fan-fold printout, all four of the lines above are truncated and
thus missing the last number.

![Three of the truncated lines](/blog/images/2026-04-30-horv0350.jpg)

Now, I have no evidence that HORV0350's own data file was actually truncated.
But anyone retyping the game from that printout could easily have assumed that
the Cobble Crawl was meant to be dark, and that Witt's End was meant
to have no DOWN exit.

I see evidence that such truncation really did happen between LONG0501 and
ANON0501/OSKA0501.

- ANON0501 reflows line [(1)](https://github.com/Quuxplusone/Advent/blob/master/ANON0501/adv.data.2#L353-L354),
    preserving the exits from Witt's End,
    but truncates [(2)](https://github.com/Quuxplusone/Advent/blob/master/ANON0501/adv.data.3#L742)
    and [(3)](https://github.com/Quuxplusone/Advent/blob/master/ANON0501/adv.data.3#L759-L760).
- OSKA0501 reflows [(1)](https://github.com/Quuxplusone/Advent/blob/master/OSKA0501/src/adventure.text#L1583-L1584)
    but truncates [(2)](https://github.com/Quuxplusone/Advent/blob/master/OSKA0501/src/adventure.text#L3218)
    and [(3)](https://github.com/Quuxplusone/Advent/blob/master/OSKA0501/src/adventure.text#L3235-L3236).
- MCDO0551 reflows [(1)](https://github.com/Quuxplusone/Advent/blob/master/MCDO0551/ADVDAT#L1606-L1607)
    but truncates [(2)](https://github.com/Quuxplusone/Advent/blob/master/MCDO0551/ADVDAT#L3172)
    and [(3)](https://github.com/Quuxplusone/Advent/blob/master/MCDO0551/ADVDAT#L3191-L3192).
- ROBE0665 reflows all three.

My decompilation of [the recovered LONG0751's](/blog/2025/12/29/long0751/) data file indicates
that it did not truncate any of these lines; and my playtesting of the recovered LONG0501
indicates that it did not truncate (1) or (2) either. (It's inconvenient to test (3) by playtesting.)

These observations are consistent with the hypothesis that first LONG0501 reflowed (1) but left (2) and (3)
untouched; then ANON0501/OSKA0501 and MCDO0551 descended from truncated paper printouts of LONG0501
(or via a now-lost common ancestor that was itself descended from LONG0501 by truncation).
Meanwhile LONG0751 and ROBE0665 descended, independently, from non-truncated copies of LONG0501.
