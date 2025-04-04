---
layout: post
title: 'Phrase origin: Why do we "call" functions?'
date: 2025-04-04 00:01:00 +0000
tags:
  digital-antiquaria
  etymology
excerpt: |
  On [StackExchange](https://softwareengineering.stackexchange.com/questions/253694/where-did-the-notion-of-calling-a-function-come-from),
  someone asks why programmers talk about "calling" a function. Several possible allusions spring to mind:

  * Calling a function is like calling on a friend — we go, we stay a while, we come back.
  * Calling a function is like calling for a servant — a summoning to perform a task.
  * Calling a function is like making a phone call — we ask a question and get an answer from outside ourselves.

  The true answer seems to be the middle one — "calling" as in "calling up, summoning" —
  but indirectly, originating in the notion of "calling for" a subroutine out of a library of subroutines
  in the same way that we'd "call for" a book out of a closed-stack library of books.
---

On [StackExchange](https://softwareengineering.stackexchange.com/questions/253694/where-did-the-notion-of-calling-a-function-come-from),
someone asks why programmers talk about "calling" a function. Several possible allusions spring to mind:

* Calling a function is like calling on a friend — we go, we stay a while, we come back.
* Calling a function is like calling for a servant — a summoning to perform a task.
* Calling a function is like making a phone call — we ask a question and get an answer from outside ourselves.

The true answer seems to be the middle one — "calling" as in "calling up, summoning" —
but indirectly, originating in the notion of "calling for" a subroutine out of a library of subroutines
in the same way that we'd "call for" a book out of a closed-stack library of books.

---

The OED's first citation for _call number_ in the library-science sense comes from
[Melvil Dewey](https://en.wikipedia.org/wiki/Melvil_Dewey) (yes, [that Dewey](https://en.wikipedia.org/wiki/Dewey_Decimal_Classification))
in 1876. The OED defines it as:

> A mark, esp. a number, on a library book, or listed in a library's catalogue,
> indicating the book's location in the library; a book's press mark or [shelf mark](https://en.wikipedia.org/wiki/Shelfmark).

I see librarians using the term "call-number" in [_The Library Journal_ <b>13</b>.9](https://archive.org/details/libraryjournalch1318dewe/page/309/mode/1up?q=%22call+number%22)
(1888) as if it was very well established already by that point:

> Mr. Davidson read a letter from Mr. A.W. Tyler [...] enclosing sample of the new <b>call blank</b> used at the
> Plainfield (N.J.) P. L., giving more room for the signature and address of the applicant. [...]
> "In connection with Mr. Tyler's new <b>call slip</b> [...] I always feel outraged when I make up a long list
> of <b>call numbers</b> in order to make sure of a book, and then the librarian keeps the list, and the next
> time I have it all to do over again."

According to [_The Organization of Information_](https://books.google.com/books/?id=uhPHEAAAQBAJ&pg=PA624) 4th ed. (Joudrey & Taylor, 2017):

> <b>Call number.</b> A notation on a resource that matches the same notation in the metadata description and
> is used to identify and locate the item; it often consists of a classification notation and a cutter number,
> and it may also include a workmark and/or a date. It is the number used to "call" for an item in a closed-stack
> library; thus the source of the name "call number."
>
> <b>Cutter number.</b> A designation with the purpose of alphabetizing all works that have exactly the same classification notation.
> Named for [Charles Ammi Cutter](https://en.wikipedia.org/wiki/Charles_Ammi_Cutter), who devised such a scheme, but spelled
> with a small _c_ when referring to another such table that is not Cutter's own.

---

John W. Mauchly's article ["Preparation of problems for EDVAC-type machines"](https://archive.org/details/originsofdigital0000rand/page/365/mode/1up) (1947)
uses the English word "call" only twice, yet this seems to be an important early attestation of the word
in the context of a "library" of computer subroutines:

> Important questions for the users of a machine are: How easily can reference be made to any of the subroutines?
> How hard is it to initiate a subroutine? What conditions can be used to terminate a subroutine? And with what
> facility can control revert to any part of the original sequence or some further sequence [...] Facilities
> for conditional and other transfers to subroutines, transfers to still further subroutines, and transfers
> back again, are certain to be used frequently.
>
> [...] the position in the memory at which arguments are placed can be standardized, so that whenever a subroutine
> is <b>called in</b> to perform a calculation, the subroutine will automatically know that the argument which is to
> be used is at a specified place.
>
> [...] Some of them might be written out in a handbook and transferred to the coding of the problem as needed,
> but those of any complexity presumably ought to be in a <b>library</b> — that is, a set of magnetic tapes in which
> previously coded problems of permanent value are stored.
>
> [...] One of the problems which must be met in this case is the method of withdrawal from the library
> and of compilation in the proper sequence for the particular problem. [...] It is possible [...] to evolve
> a coding instruction for placing the subroutines in the memory at places known to the machine, and in such
> a way that they may easily be <b>called into use</b> [...] all one needs to do is make brief reference to them
> by number, as they are indicated in the coding.

The manual for the ["MANIAC II assembly routine"](https://archive.org/details/bitsavers_lanlMANIAC_2287632/page/n31/mode/2up?q=%22call+number%22) (January 1956)
follows Mauchly's sketch pretty closely. MANIAC II has a paper-tape "library" of subroutines which
can be summoned up (by the assembler) to become part of a fully assembled program, and in fact each item
in the "library" has an identifying "call number," just like every book in a real library has a call number:

> The assembly routine for Maniac II is designed to translate descriptive code into absolute code. [...]
> The bulk of the descriptive tape consists of a series of instructions, separated, by control words,
> into numbered groups called _boxes_ [because flowcharts: today we'd say "basic blocks"]. The allowed
> box numbers are `01` through `EF`[. ...] If the address [in an instruction's address field] is `FXXX`,
> then the instruction must be a transfer, and the transfer is to the subroutine whose <b>call number</b>
> is `XXX`. The most common subroutines are on the same magnetic tape as the assembly routine, and are
> brought in automatically. For other subroutines, the assembly routine stops to allow the appropriate
> paper tapes to be put into the photoreader.

Notice that the actual instruction (or "order") in MANIAC II is still known as "`TC`," "transfer control,"
and the program's runtime behavior is known as a _transfer of control_, not yet as a _call_.
The _calling_ here not the runtime behavior but rather the calling-up of the coded subroutine (at assembly time)
to become part of the fully assembled program.

[Fortran II](https://archive.org/details/bitsavers_ibm704C286_3406140/page/n19) (1958; also
[here](https://ed-thelen.org/LaFarr/IBM-FORTRAN-II-704-C28-6000-2-c-1958.pdf))
introduced `CALL` and `RETURN` statements, with this description:

> The additional facilities of FORTRAN II effectively enable the programmer to expand the language
> of the system indefinitely. [...] Each [CALL statement] will constitute <b>a call for</b> the defining subprogram,
> which may carry out a procedure of any length or complexity [...]
>
> [The CALL] statement causes transfer of control to the subroutine NAME and presents the subroutine
> with the arguments, if any, enclosed in parentheses. [...] A subroutine introduced by a SUBROUTINE
> statement is <b>called into</b> the main program by a CALL statement specifying the name of the subroutine.
> For example, the subroutine introduced by
>
>     SUBROUTINE MATMPY (A, N, M, B, L, C)
>
> could be <b>called into</b> the main program by the statement
>
>     CALL MATMPY (X, 5, 10, Y, 7, Z).

Notice that Fortran II still describes the runtime behavior as "transfer of control,"
but as the computer language becomes higher-level the English starts to blur and conflate
the runtime transfer-of-control behavior with the assembly- or link-time "calling-in" behavior.

In Robert I. Sarbacher's [_Encyclopedic dictionary of electronics and nuclear engineering_](https://archive.org/details/encyclopedicdict00sarb/page/215/mode/1up) (1959),
the entry for <b>Subroutine</b> doesn't use the word "call," but Sarbacher does seem to be reflecting
a mental model somewhere inside the union of Mauchly's definition and Fortran II's.

> <b>Call in.</b> In computer programming, the transfer of control of a computer from a
> main routine to a subroutine that has been inserted into the sequence of calculating
> operations to perform a subsidiary operation.
>
> <b>Call number.</b> In computer programming, a set of characters used to identify
> a subroutine. They may include information related to the operands, or may be used
> to generate the subroutine.
>
> <b>Call word.</b> In computer programming, a call number exactly the length of one word.

Notice that Sarbacher defines "call in" as the runtime transfer of control itself;
that's different from how the Fortran II manual used the term. _Maybe_ Sarbacher was
accurately reflecting an actual shift in colloquial meaning that had already taken place
between 1958 and 1959 — but personally I think he might simply have goofed it.
(Sarbacher was a highly trained physicist, but not a computer guy, as far as I can tell.)

["JOVIAL: A Description of the Language"](https://archive.org/details/bitsavers_sdcjovialj0342JOVIALLangDescrFeb60_3650695/page/n56/mode/1up) (February 1960) says:

> A <b>procedure call</b> [today we'd say "call site"] is the link from the main program
> to a procedure. It is the only place from which a procedure may be entered.
>
> An *input parameter* [today we'd say "argument"] is an arithmetic expression specified in the <b>procedure call</b>
> which represents a value on which the procedure is to operate[.] A *dummy input parameter* is an item specified in
> the procedure declaration which represents a value to be used by the procedure as an input parameter.
>
> One or more <b>Procedure Calls</b> (<b>of</b> other procedures) may appear within a procedure.
> At present, only four "levels" of <b>calls</b> may exist.

That JOVIAL manual mentions not only the "procedure call" (the syntax for transferring control to a procedure declaration)
but also the "`SWITCH` call" (the syntax for transferring control to a switch-case label). That is, JOVIAL (1960) has
fully adopted the _noun_ "call" to mean "the syntactic indicator of a runtime transfer of control."
However, JOVIAL never uses "to call" as a verb.

Backtracking a few months, here's Perlis & Samelson's
["Preliminary Report—International Algebraic Language"](https://archive.org/details/algol-preliminary/page/n8/mode/1up) (_CACM_ <b>2</b>(6), June 1959):

> A _procedure_ statement serves to initiate (<b>call for</b>) the execution of a _procedure_,
> which is a closed and self-contained process [...] The procedure declaration defining the <b>called</b> procedure contains,
> in its heading, a string of symbols identical in form to the procedure statement, and the formal parameters [...]
> give complete information concerning the admissibility of parameters used in any <b>procedure call</b>[.]

Peter Naur's ["Algol 60 Report"](https://archive.org/details/algol-60-report/page/n9/mode/1up) (May 1960) avoids the verb "call,"
but in a new development casually uses the noun "call" to mean "the period during which the procedure itself is working" —
not the transfer of control but the period _between_ the transfers in and out:

> A procedure statement serves to invoke (<b>call for</b>) the execution of a procedure body. [...]
> [When passing an array parameter, if] the formal parameter is called by value the local array created
> <b>during the call</b> will have the same subscript bounds as the actual array.

Finally, ["Burroughs Algebraic Compiler: A representation of ALGOL for use with the Burroughs 220 data-processing system"](https://archive.org/details/bitsavers_burroughse11DBALGOLJan61_5661201/page/n24/mode/1up) (1961)
attests a single (definitionary) instance of the preposition-less verb "call":

> The `ENTER` statement is used to initiate the execution of a subroutine (<i>to <b>call</b></i> a subroutine).

The usage in ["Advanced Computer Programming: A case study of a classroom assembly program"](https://archive.org/details/bitsavers_mitctssAdv_8918831/page/n15/mode/2up?q=%22call%22) (Corbató, Poduska, & Saltzer, 1963)
is entirely modern: "It is still convenient for pass one to <b>call</b> a subroutine to store the cards"; "In order to <b>call</b> `EVAL`,
it is necessary to save away temporary results"; "the subroutine which <b>calls</b> `PASS1` and `PASS2`"; etc.

Therefore my guesses at the moment are:

- Fortran II (1958) rapidly popularized the phrasing "to call X" for the temporary transfer of control to X,
    because "`CALL X`" is literally what you write in a Fortran II program when you want to transfer control
    to the procedure named `X`.

- Fortran's own choice of the "`CALL`" mnemonic was <b>an original neologism,</b> inspired by the pre-existing use
    of "call (in/up)" as seen in the Mauchly and MANIAC II quotations but introducing wrinkles that had never been
    seen anywhere before Fortran.

- By 1959, Algol had picked up "call" from Fortran. Algol's "procedure statements" produced _calls_ at runtime;
    a procedure could _be called_; during _the call_ the procedure would perform its work.

- <b>By 1961,</b> we see the first uses of the exact phrase "to call X."
