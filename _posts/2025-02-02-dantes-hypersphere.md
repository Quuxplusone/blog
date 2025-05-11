---
layout: post
title: "Dante's hypersphere in THREE.js"
date: 2025-02-02 00:01:00 +0000
tags:
  dante
  help-wanted
  litclub
  math
  web
---

Mark A. Peterson's essay ["Dante and the 3-sphere"](https://www.mathinees-lacaniennes.net/images/stories/articles/dante.pdf)
(_Am. J. Phys._ 47(12), December 1979) suggests that when, in _Paradiso_ 27, Dante looks from the starry sphere upward in
the direction of the Empyrean and downward in the direction of Earth, and when he sees that nine concentric circles
of angels above mirror the nine concentric planetary spheres below, he is
"groping for a language to express an idea conceived intuitively and nonverbally" — namely, the idea of
"how a 3-sphere would look from its equator."

The surface of the Earth forms a 2-sphere — a two-dimensional space which (because it curves back on itself in three dimensions) is finite,
yet edgeless. You can make a 2-sphere by taking two flat disks, one for the north hemisphere and one for the south hemisphere;
aligning their edges and sewing them together all around; and then inflating the thing.

A 3-sphere is that shape's three-dimensional analogue — a three-dimensional space which (because it curves back on itself in four
dimensions) is finite, yet edgeless. You can make a 3-sphere by taking two solid balls, one for the terrestrial semiuniverse
and one for the celestial semiuniverse; aligning their boundaries and gluing them together all over; and then
inflating the thing in the fourth dimension. (I can picture all of these steps intuitively except for the final step,
which is important because it's what will make the whole thing have constant curvature.)

I wanted to see a little more concretely what Peterson (and perhaps Dante) were describing, so I tried my hand at
hacking it together in the WebGL rendering framework [THREE.js](https://threejs.org/examples/#webgl_camera).

Use the arrow keys to look around, WASD to move forward and backward and strafe left and right, and RF to strafe up and down.
I recommend starting by holding down the up-arrow until the terrestrial spheres fall away below and, after a little longer,
the celestial spheres come into view above. If keyboard focus keeps leaving the WebGL iframe,
try [this full-page version](/blog/code/2025-02-02-dantes-hypersphere.html). And my apologies to mobile users:
I don't think this works at all on mobile.

<iframe src="/blog/code/2025-02-02-dantes-hypersphere.html" width="100%" height="750px" onload="this.height = this.contentWindow.document.body.scrollHeight + 'px';">
</iframe>

What you're looking at here only somewhat resembles Peterson's vision. Rather than nine nested spheres in each
semiuniverse, I bothered to depict only the Earth, Moon, Mercury, Venus, and the Sun, and their reflections in the
upper semiuniverse. The two semiuniverses are separated by an infinitesimally thin translucent sky-blue plane.
This plane doesn't reflect anything in Peterson's model, but I thought it looked nice, especially given my model's
major flaw: I'm not mathematically agile enough to figure out how to "smooth" the curvature at the boundary between
the two semiuniverses.

In fact, I'm not even agile enough to handle the same kind of "gluing together at the boundary" that Peterson describes!
(Raycasting through the glued boundary was easy enough, but letting the _camera itself_ travel through the boundary
without glitching its rotation simply gave me too much trouble. Assistance welcome.) So what this visualization
actually depicts isn't a hypersphere but rather a kind of hyper-torus. In Euclidean terms, we have two balls
connected by portals at their boundary. In Peterson's vision as I understand it, when you exit the terrestrial ball at some point $$P$$,
you warp to the celestial ball at that same point $$P$$ but headed inward, with your heading reflected off the wall
of the sphere. In the visualization above, when you exit the terrestrial ball at $$P$$, you warp to _the antipode_
of $$P$$ in the celestial ball, with your heading unchanged.

This was my first time using THREE.js. Most of the code I shamelessly stole from
[Nabil Mansour's](https://nabilmansour.com/) excellent tutorial
["Ray Marching in Three.js"](https://medium.com/@nabilnymansour/ray-marching-in-three-js-66b03e3a6af2) (May 2024).
Other helpful resources included
[the GLSL docs on Shaderific.com](https://shaderific.com/glsl/geometric_functions.html) and
[David Eberly's geometrical tips on GeometricTools.com](https://www.geometrictools.com/Documentation/Documentation.html).

If you improve on this visualization, please let me know!
