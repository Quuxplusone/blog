---
layout: post
title: "Scott Kim's rotational ambigrams for the Celebration of Mind"
date: 2020-10-18 00:01:00 +0000
tags:
  celebration-of-mind
  hofstadter
  math
  pretty-pictures
  typography
  web
---

The "Celebration of Mind" is held annually on or around Martin Gardner's birthday (October 21st).
This year, the 10th annual CoM is [being held virtually](https://www.gathering4gardner.org/g4gs-celebration-of-mind-2020/)
all week long, with Zoom webinars at noon, 4pm, and 8pm daily from Saturday 2020-10-17 until Friday 2020-10-23.

> The annual October "Celebration of Mind" is not to be confused with the
> March/April ["Gathering 4 Gardner"](https://www.gathering4gardner.org/category/g4gn-recaps/)
> held biennially since G4G2 in 1996.
> I admit I have only just learned that these are different!

I just attended Scott Kim's webinar on "Musical Illusions," in which he demonstrated how to play the
[Shepard tone](https://www.youtube.com/watch?v=boJD_gTLavA) on the piano, and also how one person can
harmonize "Frère Jacques" by simultaneously humming one part and whistling another. (This was quite
impressive. I may be playing a bit of telephone here, but my understanding is that Kim basically taught
himself to hum-whistle in 24 hours after Douglas Hofstadter dared him to.) Incidentally, this
same pair of parlor tricks is mentioned in a footnote to [_Musical Illusions and Phantom Words_](https://amzn.to/3k8Mrwc) (2019),
by Diana Deutsch, who was also in the webinar audience.

For more Shepard-tone craziness, check out [this animation](https://www.youtube.com/watch?v=u9VMfdG873E&ab_channel=VladimirBulatov)
by Vladimir Bulatov (video) and Daniel Repasky (audio).

----

Scott Kim is also known for creating "ambigrams," as in his 1981 book [_Inversions_](https://amzn.to/358YFOZ).
For the first four "Celebration of Mind" events, in 2010, 2011, 2012, and 2013, Kim created amazing
invertible designs. Hover or tap to rotate each image:

|:----------------------------------------------------------------------------------------------------:|:----------------------------------------------------------------------------------------------------:|
| ![Scott Kim's logo for CoM #1 (2010)](/blog/images/2020-10-18-ambigram-2010.jpg){: .rotate-on-hover} | ![Scott Kim's logo for CoM #2 (2011)](/blog/images/2020-10-18-ambigram-2011.jpg){: .rotate-on-hover} |
| ![Scott Kim's logo for CoM #3 (2012)](/blog/images/2020-10-18-ambigram-2012.jpg){: .rotate-on-hover} | ![Scott Kim's logo for CoM #4 (2013)](/blog/images/2020-10-18-ambigram-2013.jpg){: .rotate-on-hover} |

Scott Kim also created an invertible logo for the eighth biennial "Gathering for Gardner" (2008):

![Scott Kim's logo for G4G #8 (2008)](/blog/images/2020-10-18-ambigram-2008.png){: .rotate-on-hover}

Images from [martin-gardner.org](http://martin-gardner.org/TributeGraphics.html) and
[Butler University](https://digitalcommons.butler.edu/faculty_images/23/).

<script>
    window.addEventListener('load', function () {
        for (var e of document.getElementsByClassName('rotate-on-hover')) {
            e.addEventListener('click', function () { this.classList.toggle('inverted'); });
        }
    });
</script>
