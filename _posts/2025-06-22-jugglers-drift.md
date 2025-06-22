---
layout: post
title: "Animating moving juggling patterns"
date: 2025-06-22 00:01:00 +0000
tags:
  juggling
  web
---

About 15 years ago I was active with the [Santa Barbara Jugglers Association](http://www.sbjuggle.org/html/home.html)
(which still exists and still meets up by UCSB in Isla Vista). They're particularly known for
club-passing in _moving patterns_. We're talking things like [Havana](https://vimeo.com/348768026) and
[Shooting Star](https://www.youtube.com/watch?v=SMTtJAJZaGo). (I don't think either of those patterns came out
of Isla Vista, mind you. I don't know where either of them _did_ come from. The
[Madison Area Jugglers' Pattern Book](https://madjugglers.com/majpatternbook/) credits Shooting Star to
"Bryan Olson in 1993.")

Anyway, back then I had the idea of creating a juggling simulator for moving patterns. I was vaguely familiar
with Wolfgang Westerboer's [JoePass!](http://koelnvention.de/w/?page_id=151) but felt it could be improved,
in ways I no longer remember; possibly I was hoping to be able to animate takeouts, wally-walks, and other
such extremely idiosyncratic motions. Also I was enamored of [METAFONT](https://en.wikipedia.org/wiki/Metafont),
and wanted to use an algebraic language to define the trajectories of individual jugglers, such that you
could say something like "This is basically a triangle, but spline it up a bit and take the leads into account
before you animate it." Well, I got as far as
[a very partial specification](https://github.com/Quuxplusone/JugglersDrift/blob/422873a/doc/index.html)
of the input language; but then, in search of instant gratification, pivoted to the idea that the user
would just provide "key frames" of an animation, and the software would fill in between them. I got _that_
working, and then let it lie for about 13 years.

> The one indubitably successful piece of the project was the naming: I called the program "Juggler's Drift,"
> stealing an existing term for the tendency of any moving pattern to drift around the gymnasium floor instead
> of staying where it started.

The other day, Matthew Thornley posted to the SBJA mailing list an interesting five-count weave pattern
that they'd invented in Isla Vista circa June 2025. I figured this was as good an excuse as any to dust off
Juggler's Drift and add five-count weave to the [list of patterns I've animated](https://quuxplusone.github.io/JugglersDrift/).

To animate a pattern in Juggler's Drift, you need a set of key frames: where is everyone standing and who's
passing to whom, on each beat of the pattern. (Or every other beat, or whatever; as long as the frames are
a constant number of beats apart.) For example, for the Isla Vista Weave ("IV W"), we could supply these five
key frames:

![Four key frames](/blog/images/2025-06-22-keyframes.png)

Notice that frame #5 is the same as frame #1 with the colors permuted; this avoids our needing to draw out
the pattern's whole cycle. In this pattern, each frame represents a right-hand throw; we needn't draw the
left-hand throws, since they're all selfs. In each frame we draw a colored circle for each juggler, and a
heavy black line between two jugglers if they're exchanging clubs. (The program also supports lines with
arrowheads, for oogle patterns. There's no iconography for backdrops, doubles, Jims, missing clubs, takeouts,
etc. etc., although I would certainly welcome improvements of that nature.) Finally, a frame can include
"tracery" in light gray, which the program ignores. Anyway, we take these five key frames and feed them into
Juggler's Drift as follows:

    ./png2js --join=0 [12345].png

It spits out a JavaScript representation of the pattern:

    var PatternLengthInBeats = 40;
    var JugglerSize = 25;
    var Jugglers = [
     { color: '#0000FF',
          ts: [0, 2, 4, 6, 8, 10, 12, ~~~~ ],
           x: [357.0, 381.0, 443.0, ~~~~ ],
           y: [207.0, 116.0, 46.0, ~~~~ ],
          fa: [-0.4, 0.3, 1.1, 2.1, ~~~~ ] },
     ~~~~
    };
    var Passes = [
      { start: 0, end: 1.3, from: 0, fromhand: 'r', to: 0, tohand: 'l' },
      { start: 0, end: 1.3, from: 1, fromhand: 'r', to: 1, tohand: 'l' },
      { start: 0, end: 1.3, from: 2, fromhand: 'r', to: 4, tohand: 'l' },
      ~~~~
    };

which can then be fed into a bit of HTML-and-JavaScript (distributed alongside `png2js`) that knows how to animate that
representation. The result looks something like this. (Again, this is the famous "IV W".)

<iframe src="https://quuxplusone.github.io/JugglersDrift/IVW/index.html" width="100%" height="300px" onload="this.height = this.contentWindow.document.body.scrollHeight + 'px';">
[Click here](https://quuxplusone.github.io/JugglersDrift/IVW/)
</iframe>

For this month's five-count weave, the finished animation looks like this:

<iframe src="https://quuxplusone.github.io/JugglersDrift/FiveCountWeave/index.html" width="100%" height="300px" onload="this.height = this.contentWindow.document.body.scrollHeight + 'px';">
[Click here](https://quuxplusone.github.io/JugglersDrift/FiveCountWeave/)
</iframe>

Click through to see [the other patterns I've animated](https://quuxplusone.github.io/JugglersDrift/).

Finally, to create key frames, I very highly recommend [JSPaint](https://jspaint.app/), Isaiah Odhner's pixel-for-pixel
recreation of Microsoft Paint in the browser. It's truly one of the wonders of the modern web.
