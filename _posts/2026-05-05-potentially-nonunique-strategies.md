---
layout: post
title: "ELF's ways to combine potentially non-unique objects"
date: 2026-05-05 00:01:00 +0000
tags:
  abi
  elf
  inline-functions
  language-design
  proposal
  rant
  sufficiently-smart-compiler
  templates
excerpt: |
  Previously [I wrote](/blog/2026/04/24/define-static-array/):

  > [Template parameter objects of array type] are permitted to overlap or be
  > coalesced, just like `initializer_list`s and string literals. Clang trunk
  > isn't smart enough to coalesce potentially non-unique objects [but]
  > GCC, once it implements `define_static_array`, will presumably make them the same.

  Well, GCC 16 has an experimental implementation of `define_static_array`
  (compile with `g++ -std=c++26 -freflection`), and it does _not_ coalesce
  template parameter objects of array type in the way I expected. Digging deeper
  into why not, I learned that there are at least three ways compilers and linkers
  (on ELF — that is, non-Windows — platforms) conspire to "merge"
  potentially non-unique objects:

  * Merging at the compiler level (for `initializer_list` backing arrays)
  * Sections with `SHF_MERGE` (for string literals and backing arrays)
  * Sections with `SHF_GROUP`, a.k.a. COMDAT sections (for inline variables)
---

Previously [I wrote](/blog/2026/04/24/define-static-array/):

> [Template parameter objects of array type] are permitted to overlap or be
> coalesced, just like `initializer_list`s and string literals. Clang trunk
> isn't smart enough to coalesce potentially non-unique objects [but]
> GCC, once it implements `define_static_array`, will presumably make them the same.

Well, GCC 16 has an experimental implementation of `define_static_array`
(compile with `g++ -std=c++26 -freflection`), and it does _not_ coalesce
template parameter objects of array type in the way I expected. Digging deeper
into why not, I learned that there are at least three ways compilers and linkers
(on ELF — that is, non-Windows — platforms) conspire to "merge"
potentially non-unique objects:

* Merging at the compiler level (for `initializer_list` backing arrays)
* Sections with `SHF_MERGE` (for string literals and backing arrays)
* Sections with `SHF_GROUP`, a.k.a. COMDAT sections (for inline variables)

Sadly, no combination of these facilities *quite* achieves
ideal behavior for `define_static_array`. Let's take a look.

## The compiler can merge similar data

GCC itself merges similar `initializer_list` backing arrays. For example
([Godbolt](https://godbolt.org/z/EnzxoM9ch)):

    void f(std::initializer_list<int>);
    int main() {
      f({1,2,3}); // C.0.0
      f({1,2,3}); // C.1.1
    }

turns into the assembly directives

      .section .rodata.cst16,"aM",@progbits,16
      .align 8
      .type C.0.0, @object
      .size C.0.0, 12
    C.0.0:
      .long 1
      .long 2
      .long 3
      .zero 4
      .set C.1.1,C.0.0

The symbols `C.0.0` and `C.1.1` are set to the same memory address, because GCC
itself can see that the two `initializer_list` objects should have the same backing
array.

This is the most powerful approach, but at the same time the least elegant, because
it requires ad-hoc "smarts" built directly into the compiler. For example, we
could imagine GCC generating code that merges one list into the tail of another:

    void f(std::initializer_list<int>);
    int main() {
      f({1,2,3}); // C.0.0
      f({2,3});   // C.2.2
    }

    C.0.0:
      .long 1
      .long 2
      .long 3
      .zero 4
      .set C.2.2,C.0.0 + 4

but in fact GCC doesn't generate that code, because nobody has taught GCC that specific
trick. Nor will GCC 16 merge the backing arrays of `{1,2,3}` and `{1u,2u,3u}` using
this technique, again because it hasn't been taught to.

Merging things at the compiler level also, by definition, works only within a single
translation unit (a single .cpp file). If you want to merge things between different TUs,
you'll need help from the linker. Which brings us to...

## `SHF_MERGE` sections

I said GCC 16 wouldn't merge `{1,2,3}` and `{1u,2u,3u}` in the compiler. But if you
try this program ([Godbolt](https://godbolt.org/z/MqxsPE98z)), you'll see that the
backing arrays are indeed merged at runtime — the same pointer value is printed twice:

    template<class... Ts>
    void f(std::initializer_list<Ts>... ils) {
      (printf("%p\n", (const void*)ils.begin()) , ...);
    }
    int main() {
      f({1,2,3},     // C.0.0
        {1u,2u,3u}); // C.1.1
    }

The same pointer value is printed twice, despite that we can see GCC emitting two
different objects into the assembly file:

      .section .rodata.cst16,"aM",@progbits,16
      .align 8
      .type C.0.0, @object
      .size C.0.0, 12
    C.0.0:
      .long 1
      .long 2
      .long 3
      .zero 4
      .align 8
      .type C.1.1, @object
      .size C.1.1, 12
    C.1.1:
      .long 1
      .long 2
      .long 3
      .zero 4

The trick here is in the `.section` directive, which creates an ELF section in the
object file with the name `.rodata.cst16`, the section flag `SHF_MERGE` (that's the `M`
in `"aM"`), and a `sh_entsize` of `16` bytes. After concatenating every object file's
`.rodata.cst16` sections as usual, the linker is permitted (but not required) to treat
the contents of this section as an array of 16-byte elements, and to deduplicate any
identical elements it finds. Since the 16-byte region starting at `C.1.1` matches the
16-byte region starting at `C.0.0`, the linker is allowed to eliminate the 16 bytes
at `C.1.1` and point the label `C.1.1` at `C.0.0` (or vice versa).

The `SHF_MERGE` trick works across TUs, and even across types. For example, GCC 14+
makes the following _four_ initializer lists share a single backing array
at runtime by putting them all into `.rodata.cst16` sections with the
`SHF_MERGE` flag set. This works even if the lists appear in different TUs!
([Godbolt.](https://godbolt.org/z/19ch8oYK9))

    {1,2,3,4}
    {1u,2u,3u,4u}
    {0x200000001, 0x400000003} // given little-endian int64
    {4.2439915824e-314, 8.4879831654e-314} // ditto

The major optimization-related downside of the `SHF_MERGE` approach — as it is sketched in the
[System V ABI document](https://refspecs.linuxbase.org/elf/gabi4+/ch4.sheader.html#:~:text=The%20data%20in%20the%20section%20may%20be%20merged)
(2001) and as it is implemented in GNU `ld` as far as I know — is that you can't use it to merge data
elements of different sizes or alignments. GCC 16 won't merge `{1,2}` with `{1,2,3}`
because GCC puts the former in section `.rodata.cst8` and the latter in `.rodata.cst16`.
(GCC 14 and 15 put the latter in plain old unmergeable `.rodata` instead, because
its size — 12 bytes — isn't precisely 16 bytes. GCC 16 fixed that.) Basically, GCC has to
precommit every data element to a specific "bucket"; the linker will consider merging
it _only_ with other elements in its own bucket.

And `SHF_MERGE` cannot merge parts of elements; it can merge only full elements.
So while you might think a "sufficiently smart linker" could merge `{2,3}` across the
conjunction of `{1,2}` and `{3,4}`, the `SHF_MERGE` algorithm by itself will never do that.
Some users might even rely on that guarantee (somehow), so I don't imagine that
any linker will ever gain the smarts to do that.

## `SHF_MERGE | SHF_STRINGS`

When an ELF section specifies both `SHF_MERGE` and `SHF_STRINGS`, then instead of chunking
the section into elements of size `sh_entsize` bytes, the linker chunks the section into
variable-length elements each of which is a null-terminated C-style string. It then
deduplicates those strings.

As in the previous subsection, it seems that no linker will merge
`"ello"` into the tail of `"Hello"`; the `SHF_MERGE|SHF_STRINGS` algorithm alone
will merge only full elements. (In this case, full null-terminated strings.)

The `SHF_MERGE|SHF_STRINGS` algorithm finds the "elements" of the section by simplemindedly
scanning for null bytes; no additional metadata is involved. Therefore, such a section must
never contain strings with embedded null bytes. Try the following on your machine:

    cat >x.c <<EOF
    #include <stdio.h>
    int main() {
      puts("p");
      puts(&"qxp"[2]);
      puts("r");
    }
    EOF
    gcc -S x.c -o - | sed 's/qxp/q\\0p/' > x.s
    gcc x.s
    ./a.out

The assembly file `x.s` should end up containing something like this:

      .section .rodata.str1.1,"aMS",@progbits
    .LC0:
      .string "p"
    .LC1:
      .string "q\0p"  # Danger!
    .LC2:
      .string "r"

and when executed, will print not `p p r` but rather `p r r`, because the `SHF_MERGE`
algorithm understands `"q\0p"` as `"q" "p"` and eliminates the second `"p"` as a duplicate.
Therefore, a C++ compiler must go out of its way never to store a string literal containing
embedded null bytes into such a section.

    const char *p1 = "hello world"; // literal in .rodata.str1.1 (SHF_MERGE)
    const char *p2 = "hell\0world"; // literal in .rodata (not SHF_MERGE)

In theory, a compiler could place the backing array for an `initializer_list<char>`
into a mergeable string section, and potentially merge the backing arrays for
`{'x','y','z','\0'}` and `"xyz"`. In practice, neither GCC nor Clang does this (yet).

---

The big disadvantage of `SHF_MERGE` merging from the C++ compiler's point of view —
the thing that makes it unsuitable for certain use-cases such as merging the duplicate definitions
of template parameter objects — is that it is completely optional. It's legal for a
dumb linker to just ignore the `SHF_MERGE` flag. It would be unwise to rely on `SHF_MERGE`
to take care of merging objects that the C++ standard _requires_ us to merge, such as the
duplicate definitions of inline variables or template parameter objects.

And, of course, it would be wrong to place an inline variable into an `SHF_MERGE` section
anyway, because the standard (and common sense) _forbids_ us to merge unrelated inline
variables just because they happen to have the same value!

    inline constexpr char a[] = "hello world";
    inline constexpr char b[] = "hello world";

Here `&a == &b` is guaranteed to be false; an implementation that made it true would be
non-conforming. (Even `gcc -fmerge-all-constants` will make it true, non-conformingly,
only if you remove the `inline` keyword in both places.)

To handle inline variables, which are deduplicated according to their _symbol names_
rather than their _data contents_, we need the next approach, which is...

## `SHF_GROUP`, a.k.a. COMDAT sections

For the past twenty-some years, C and C++ compilers have traditionally compiled
inline functions, inline variables, and implicit instantiations of function and variable
templates into what are called "COMDAT sections." This feature came late to ELF;
the name "COMDAT" [apparently comes](https://itanium-cxx-abi.github.io/cxx-abi/abi/prop-72-comdat.html)
from Windows NT. For more than you ever wanted to know about COMDAT, see
["COMDAT and section group"](https://maskray.me/blog/2021-07-25-comdat-and-section-group)
(Fangrui Song, July 2021).

ELF's version of COMDAT was basically designed to do _exactly_ what a C++ compiler needs
in order to implement inline functions. The compiler can take C++ code such as

    inline int f() {
      static int i = 42;
      return ++i;
    }

and turn it into a whole group of sections (text, data, rodata, whatever else it needs) —
basically a whole mini object file of its own — something like this:

      .section .text._Z1fv,"axG",@progbits,fgroup,comdat
      .globl _Z1fv
    _Z1fv:
      movl _ZZ1fvE1i(%rip), %eax
      addl $1, %eax
      movl %eax, _ZZ1fvE1i(%rip)
      ret

      .section .data._ZZ1fvE1i,"awG",@progbits,fgroup,comdat
      .globl _ZZ1fvE1i
    _ZZ1fvE1i:
      .long 42

The assembler emits an ELF section of type `SHT_GROUP` representing the group of sections
with "group identifier" `fgroup`. The linker, at link time, will pick one object file's
`fgroup` group and throw away the rest.

> Now, I simplified that codegen quite a bit. Really, GCC doesn't make such a human-friendly
> section group; it dumps each section into its own _individual_ section group (so in this
> example there will be two different group identifiers, not just one); and GCC also marks
> both `f` and `i` as `.weak` symbols rather than global. I'm not sure why GCC does these
> things; I conjecture "intermediate codegen targeting an object format less powerful than ELF"
> and "compatibility with very old ELF linkers lacking `SHT_GROUP` support" respectively,
> but I don't know. Email and tell me!

COMDAT sections are exactly what you need to implement the definitions of (1) inline functions;
(2) implicit instantiations of function templates; (3) the static local variables of
inline functions and implicitly instantiated function templates; (4) implicit instantiations
of variable templates; (5) inline variables and static inline data members of classes;
and probably a few more things I'm forgetting. All of these are entities with _names_,
and C++ requires them to be properly deduplicated by name: `&myInlineFunc` must have the same pointer
value no matter what translation unit you're in.

Another kind of entity we talked about the other day in
["Things `define_static_array` can't do"](/blog/2026/04/24/define-static-array/) (2026-04-24):
template parameter objects of class type. Code like this:

    template<A t>
    const A *f() { return &t; }

will produce a template parameter object "variable" in its own COMDAT section,
like this ([Godbolt](https://godbolt.org/z/Pb9WdKezh)):

      .section .rodata._ZTAXtl1AEE,"aG",@progbits,_ZTAXtl1AEE,comdat
      .weak _ZTAXtl1AEE
    _ZTAXtl1AEE:
      .zero 1

That's exactly the same strategy the compiler would use for a simple inline variable like

    inline const A v;

Now, when the linker deduplicates COMDAT sections, it looks at the group identifier (a symbol name)
to decide if two sections are "duplicates" or not. It doesn't care whether they have the same
bytewise contents. That makes sense, because usually the text sections corresponding to
instantiations of the same inline function in different TUs _won't_ be byte-for-byte identical
(especially if the two TUs were compiled with different optimization levels). For inline functions,
that's exactly what we need: deduping by name, not by contents.

----

You might imagine abusing COMDAT sections to do content-addressed deduplication à la `SHF_MERGE`.
We just have to put each element in its own individual section group, with a
group identifier based on (the hash of) its contents. For example, instead of

      .section .rodata.str1.1,"aMS",@progbits
    .LC0:
      .string "hello"
    .LC1:
      .string "world"
    .LC2:
      .string "hello"

we could emit

      .section .rodata.str1.hello,"aG",@progbits,group_hello,comdat
      .globl str_hello
    str_hello:
      .string "hello"
      .section .rodata.str1.world,"aG",@progbits,group_world,comdat
      .globl str_world
    str_world:
      .string "world"

But that would be vastly increase the linker's burden — not just processing all those tiny
sections, but keeping track of all those new global symbols. (We couldn't use local, internal-linkage
symbols anymore, because the linker wouldn't be helping us to repoint one symbol's relocations
at another.) And the compiler would have to dedupe _within_ the TU: we could no longer
refer to `"hello"` by local symbols like `.LC0` and `.LC2`, but at the same time we couldn't emit the
global symbol `str_hello` twice in the same TU.

> By the way, making symbol names that incorporate hashes of user-provided data is just asking for trouble.
> See ["Hash-colliding string literals on MSVC"](/blog/2022/12/31/mid-snow-and-ice/) (2022-12-31).
> A hash collision is disastrous; and when someone discovers how to generate hash collisions and starts
> writing blog posts like the above, you can't switch out your hash function for a better one because
> that would break ABI.

So it's very good that we have different, essentially custom-fitted, tools for deduping inline functions
versus string literals.

- Duplicate inline functions _must_ be merged (we forbid false negatives);
    non-duplicates _must not_ be merged (we forbid false positives). Dupe-ness is decided by symbol name;
    contents can be different. Duplication need be detected only across TUs; duplication within a single TU
    is impossible. Use COMDAT sections.

- Duplicate strings can be left unmerged (we permit false negatives), although we still forbid false positives.
    Dupe-ness is decided by bytewise contents. Duplication should be detected both within and across TUs.
    Use `SHF_MERGE`.

Templates and inline variables can also use COMDAT. A downside is that we'd like to merge the
definitions of e.g. `f<char*>` and `f<void*>` when they're identical (if nothing in the program compares their
addresses), but if dupe-ness is decided by symbol name and those two have different symbols, we're out
of luck. MSVC has a thing called ["Identical Comdat Folding"](https://learn.microsoft.com/en-us/cpp/build/reference/opt-optimizations)
that helps with that. (MSVC's ICF is a non-conforming optimization, because it doesn't check the parenthetical above.
This can [break code](https://developercommunity.visualstudio.com/t/Safe-Identical-COMDAT-folding-ICF/10888506):
[Godbolt](https://godbolt.org/z/5TM15Wzcx)).

Initializer-list backing arrays can use `SHF_MERGE` too. A downside is that we'd _really_ like to
merge `{2,3,4}` into the tail of `{1,2,3,4}`, and the `SHF_MERGE` algorithm will never do that.
At least the compiler can do that, if someone teaches it to. That compiler optimization "composes"
properly with `SHF_MERGE`. The compiler could even merge `{1,2}`, `{3,4}`, and `{2,3}` into a single
16-byte element `{1,2,3,4}` with three embedded labels, which could then merge with `{1u,2u,3u,4u}`
at link time:

      .section .rodata.cst16,"aM",@progbits,16
    .C.0.0:
      .long 1
    .C.1.1:
      .long 2
    .C.2.2:
      .long 3
      .long 4

Of course it would be better if the linker could work this out itself; the linker has more
information than the compiler and therefore can do a better job of merging elements. It would also
be nice if we could do something like `SHF_MERGE` for literals larger than 32 bytes, and/or literals
of lengths that aren't powers of two. [GCC bug #119153](https://gcc.gnu.org/bugzilla/show_bug.cgi?id=119153)
is related.

Contrariwise, C++26 `std::define_static_array` seemingly cannot use `SHF_MERGE`, because it cannot tolerate
false negatives: C++26 at present _requires_ us to merge all copies of `define_static_array(std::array{1,2,3})`
into the same place in memory. That's too bad, because forcing it to use COMDAT (name-addressed)
instead of `SHF_MERGE` (data-addressed) means we cannot take advantage of the latitude C++26 gives us to
merge `define_static_array(array{1,2,3})` with `define_static_array(array{1u,2u,3u})`.

> Well, we could merge `define_static_array(array{1,2,3})` with `define_static_array(array{1u,2u,3u})`
> if we used group identifiers based on a hash of the data! When we tried that on string data
> [above](#you-might-imagine-abusing-comdat)
> we paid a huge performance penalty as well as a correctness penalty. Here we'd pay only
> the correctness penalty. It's still not worth it, IMHO.

## Conclusion

Figuring out the best way to "compress" static data using only the tools we've got — single-TU ad-hoc compiler
smarts, data-addressed `SHF_MERGE`, and name-addressed COMDAT sections — is a very hard problem. The implementation
won't be optimum in every case.

Maybe we could give future linkers even better tools. For example, now that
"potentially non-unique object" is a term of art in C++, maybe we could just dump all PNU objects' initializers into
a single section (`.rodata.pnu`?) and in one more special section (`.pnu_symtab`, storing just a list of indices
into the real `.symtab`?) specify their starts and sizes — I think that's all the information the linker needs
in order to overlap them any way it sees fit and repair `.symtab` accordingly.

Something like that might already exist. If it does, I'd certainly like to hear about it.
And if it doesn't, I'd certainly like someone to build it!
