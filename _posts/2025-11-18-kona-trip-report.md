---
layout: post
title: "How my papers did at Kona"
date: 2025-11-18 00:01:00 +0000
tags:
  memes
  relocatability
  sg14
  triviality
  wg21
---

The Kona WG21 meeting was two weeks ago. This was the (first) big "resolve NB comments for C++26" meeting.
Last month [I wrote](/blog/2025/10/12/nb-comments/) about some of the NB comments, and whether they might
actually lead to removal of the offending features. Well, big news: P2786's non-trivial "trivial" relocation
was successfully removed in Kona, satisfying the 6+ NB comments that asked for that removal!
This is good news for everyone who uses trivial relocation today, and I'm very happy that we finally got
enough "adults in the room" (read: library implementors) to make the right call. Of course we'll probably
do it all over again for C++29. See below.

The other big news is that Contracts — with 10+ NB comments asking to remove it — was _not_ removed.
Unsurprisingly, `<hive>`, `<linalg>`, and `optional`'s `begin`/`end` also survived Kona.

Two small cleanup papers of mine were adopted into C++26 in Kona, and one rejected again:

## Adopted: P3016 harmonizing [iterator.range]

[P3016R6](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3016r6.html) ends up doing a bunch of useful
things. First, it finally adds the ability to say `il.data()` and `il.empty()` on an `initializer_list` object.

Second, it simplifies the overload sets of `begin` and `end`, which will reduce the amount of error-spam
you get when you misuse a ranged `for` loop — perhaps by as much as half!

<table class="smaller">
<tr><th>Before</th><th>After</th></tr><tr>
<td><div style="max-width: min(45vw,350px); overflow-x: scroll;"><pre>error: invalid range expression of type 'std::regex'; no viable 'begin' function available
note: candidate template ignored: could not match '_Tp[_Np]' against 'std::regex' (aka 'basic_regex&lt;char>')
   23 | constexpr _Tp* begin(_Tp (&amp;__array)[_Np]) noexcept {
note: candidate template ignored: substitution failure [with _Cp = std::regex]: no member named 'begin' in 'std::regex'
   35 | constexpr auto begin(_Cp&amp; __c) -> decltype(__c.begin()) {
note: candidate template ignored: substitution failure [with _Cp = std::regex]: no member named 'begin' in 'std::regex'
   40 | constexpr auto begin(const _Cp&amp; __c) -> decltype(__c.begin()) {
note: candidate template ignored: could not match 'initializer_list' against 'basic_regex'
   89 | constexpr const _Ep* begin(initializer_list&lt;_Ep> __il) noexcept {
note: candidate template ignored: could not match 'valarray' against 'basic_regex'
 3277 | _Tp* begin(valarray&lt;_Tp>&amp; __v) {
note: candidate template ignored: could not match 'valarray' against 'basic_regex'
 3282 | const _Tp* begin(const valarray&lt;_Tp>&amp; __v) {
</pre></div></td>
<td><div style="max-width: min(45vw,350px); overflow-x: scroll;"><pre>error: invalid range expression of type 'std::regex'; no viable 'begin' function available
note: candidate template ignored: could not match '_Tp[_Np]' against 'std::regex' (aka 'basic_regex&lt;char>')
   23 | constexpr _Tp* begin(_Tp (&amp;__array)[_Np]) noexcept {
note: candidate template ignored: substitution failure [with _Cp = std::regex]: no member named 'begin' in 'std::regex'
   35 | constexpr auto begin(_Cp&amp; __c) -> decltype(__c.begin()) {
note: candidate template ignored: substitution failure [with _Cp = std::regex]: no member named 'begin' in 'std::regex'
   40 | constexpr auto begin(const _Cp&amp; __c) -> decltype(__c.begin()) {
&nbsp;
&nbsp;
&nbsp;
&nbsp;
&nbsp;
&nbsp;
</pre></div></td></tr>
</table>

Third, it harmonizes the noexcept-specifications of all ten [[iterator.range]](https://eel.is/c++draft/iterator.range) functions.
In C++23, given a `std::vector<int> v`, you'd find that `v.end()` was guaranteed to be noexcept but `std::end(v)` wasn't
(and indeed is non-noexcept, on libc++, in November 2025). Given `int a[10]`, you'd find that `std::end(a)` was guaranteed noexcept
but `std::rend(a)` wasn't. And so on. It was just a mess of ad-hoc overloads. P3016 adds the proper conditional noexcept-specs
to all ten of these overload sets.

## Adopted: P3612 harmonizing proxy-reference types

[P3612R1](https://wg21.link/p3612) "Harmonize proxy-reference operations" cleans up another mess of ad-hoc definitions
in ancient parts of the codebase: `vector<bool>::reference` and `bitset<N>::reference`. The former was made const-assignable
by [P2214](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2022/p2214r2.html) in C++23, but the latter was left alone,
because who cares about `bitset`? Meanwhile neither type had a proper ADL `swap`: some STL vendors provided one, some didn't,
and the paper standard itself specified a bizarre _static member_ `swap` that is now, thankfully, moved to [depr].

## No joy for P3160 allocator-aware `inplace_vector`

In February I wrote:

> LEWG rejected [P3160R2](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3160r2.html) (Halpern & O'Dwyer 2024).
> I expect it to come back as a national body comment, since I cannot imagine us shipping a standard
> container without the ability to hold allocator-aware elements. That is, `inplace_vector::insert` needs to call
> the element type's allocator-extended constructor if it exists; but without knowledge of the allocator, it can't.
> See ["Boost.Interprocess and `sg14::inplace_vector`"](/blog/2024/08/23/boost-interprocess-tutorial/) (2024-08-23).
>
> This rejection was a surprise to me, because my impression in St Louis had been that we had all agreed to forward
> [P0843](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p0843r14.html)'s non-allocator-aware container
> _only because_ we knew allocator-awareness was coming right behind — all we lacked
> was the wording, and we collegially "didn't want to hold up" P0843 on that account. Coming back in Hagenberg
> with wording, only to find out it had been a bait-and-switch, was a rude (if perfectly legal) awakening.

Well, there _was_ an NB comment about `inplace_vector`. LEWG discussed the comment. I was heartened by the number
of knowledgeable people who spoke up and said, "Yes, technically, Arthur is correct: this is important, it's functionality
that the STL needs in order to function correctly, and if we ship it now we cannot fix it later at all."
(The number was two.) Sadly, those speakers also ended with "...But, procedurally speaking, we cannot fix it now,
because fixing it would be a _design issue_ and we promised to stop doing _design_ two meetings ago. So even though
he's right, we can't do it." In short, the NB comment didn't help.

`sg14::inplace_vector` will continue providing allocator-awareness. It would be interesting for some STL vendor to
volunteer to maintain an allocator-aware `inplace_vector`, since the only difference between that and what's in the
paper standard is one extra defaulted template parameter.

## Snafu on trailing commas

Not my paper, but I see from [Jan Schultke's writeup on /r/cpp](https://old.reddit.com/r/cpp/comments/1oyltrq/progress_report_for_my_proposals_at_kona_2025/)
that [P3776R1 "More trailing commas,"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3776r1.html)
which I _thought_ had had extremely strong consensus given its utility to diff-viewers and code-generators,
apparently somehow received perfect non-consensus (9–8–4–8–9) in EWG, causing it to be rejected from _C++29_
(in the midst of all the otherwise C++26 activity at this meeting). That's odd news.

## P2786 versus P1144

I'm still hopeful that one day EWG will discuss [P1144](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p1144r13.html),
or (more likely) will discuss a proposal morally equivalent to it but with different authorship.

Louis Dionne and Giuseppe D'Angelo will likely continue to pursue the P1144-lite
[P3516 "Uninitialized algorithms for relocation,"](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/p3516r0.html)
which in the presence of P2786 was forwarded to LWG in time for C++26 but hadn't made the CD. Now that P2786 is gone,
P3516 is gone too: that's acceptable collateral damage. However, P3516's library algorithms
do work fine even in the absence of triviality: they're specified in terms of
ordinary, possibly-non-trivial relocation (i.e. move and destroy), which the STL vendor can optimize
"behind the scenes," without any core-language intervention, just like they already do for
`std::copy` and `std::uninitialized_move`.

The NB-comment repository (which records the disposition of individual comments)
was taken private during this meeting and likely will remain so; the text of NB comments
remains for posterity in [N5028 "SoV and Collated Comments."](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2025/n5028.pdf)
For ease of reference in the future, here (copied from N5028) is the full text of US 46-085,
which explains _some_ of the reasons P2786 was bad for C++26:

> So-called "trivial" relocation is currently permitted to do arbitrarily more than a bitwise copy of the
> object representation. This has at least four bad effects:
>
> (1) It means that `std::is_trivially_relocatable` returns `true` for types that are not in fact "trivially
> relocatable" by the industry definition. Google Gemini, when asked "what is trivially relocatable type in c++", responds:
>
> > A trivially relocatable type in C++ is a type whose objects can be moved from one memory
> > location to another by simply copying their raw bytes (e.g., using memcpy) without needing to
> > call the move constructor at the new location and the destructor at the old location. This allows for
> > significant performance optimizations [...] Key characteristics and implications of trivially
> > relocatable types include:
> >
> > * Bitwise Copying: The core idea is that the object's internal state remains valid and
> >     consistent after a bitwise copy to a new address. [...]
> > * Safety and Correctness: The concept ensures that relocating an object via a bitwise copy is a
> >     safe and correct operation, preserving the object's invariants and avoiding undefined behavior.
>
> It would be unfortunate if C++26 standardizes a meaning for "is_trivially_relocatable" that is
> inconsistent with this widely understood and well-supported definition. Programmers (and LLMs)
> *will* assume that trivially relocatable types can in fact be memcpy'ed, realloc'ed, mmap'ed with
> safety. C++ should provide them that safety, not undercut them.
>
> (2) "Trivial" means "bitwise" in every other situation (e.g. "trivially copy constructible" always
> means "as if by memcpy," never anything more expensive: e.g. polymorphic types are never
> trivially copy constructible; ptrauth-qualified scalar types are never trivially copy
> constructible). Trying to make "trivial" mean something different from "bitwise" in the solitary
> case of relocation is inconsistent with the rest of C++.
>
> (2) It means that if you put two trivially relocatable types together in a union, the union itself might
> not be trivially relocatable. This makes trivial relocatability non-composable.
>
> (3) It introduces new implementation-defined behavior in 11.2, which could be made well-defined if we adopt the proposed change.
>
> (4) It introduces new undefined behavior in 20.2.6, which could be made well-defined if we adopt the proposed change.
> Namely, `std::relocate(static_cast<Base*>(derivedSrc), static_cast<Base*>(derivedSrc)+1, baseDst)` is
> well-formed on paper, but physically results in a "radioactive" baseDst object with the vptr of a
> Derived but the data members of a Base. 20.2.6/10 and /17 add preconditions (UB) on all
> functions that use relocation, in order to make sure the Standard doesn't accidentally claim that
> the "radioactive" behavior complies with the abstract machine. The proposed change makes
> `std::trivially_relocate(static_cast<Base*>(derivedSrc), static_cast<Base*>(derivedSrc)+1, baseDst)` ill-formed instead of UB,
> and makes `std::relocate(static_cast<Base*>(derivedSrc), static_cast<Base*>(derivedSrc)+1, baseDst)` well-defined instead of UB.

And here's US 44-082, which was the first discussed, and therefore the honored
[host](https://github.com/cplusplus/nbballot/issues/658) of the actual removal vote:

> The trivial relocation language feature currently does not allow for the ability to mark a type as
> being conditionally trivially relocatable. The suggested workaround in the paper is that users
> manually have to add conditionally-non-relocatable base classes or members to achieve this.
>
> That workaround is atrocious, and means that we're adding a language feature in C++26 that
> has worse ergonomics than a library feature that we could implement in C++26. That's not an
> acceptable situation to be in.

The vote was 27–17–2–4–6 in favor of removal. One thing that helped was the library authors saying
"I can't use this."

![](/blog/images/2025-11-18-qt-winkler.jpg){: .meme}

An even bigger help was the number of Standard Library vendors saying "I've looked at this spec now,
and _nobody_ will be able to use this. This is horribly messy."

![](/blog/images/2025-11-18-unfortunatly-for-you.jpg){: .meme}

The above meme also works if you replace "LEWG" with "Bloomberg" and "many hours" with "years."
Concepts of a plan were presented to rescue the feature — maybe by introducing a whole new user-defined
relocation operation which might retroactively motivate P2786's `std::relocate` function — but for once
LEWG was not buying. Perhaps they recalled this line from [P3236](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3236r1.html):

![](/blog/images/2025-11-18-mourinho.jpg){: .meme .float-right}

> P2786 is deliberately incomplete. This makes it hard to reason about. It is a chess problem with multiple potential "lines" of development.

Kudos to EWG and LEWG for realizing that when you belatedly discover a whole camel inside your tent, the
right action is not to debate whether to remove it nose-first or tail-first;
not to listen to the person who says it will all make sense when you see the elephant and kangaroo
they're bringing next; but simply to get. it. out.

----

In short, this meeting was largely about either fixing the problems identified with C++26, or
stonily refusing to fix them but at least making few things any worse.
C++26 may remain confusing news for working programmers, but Kona was certainly good news for C++26.
