---
layout: post
title: "Type-erasure, trivial relocation, and lethal sirens"
date: 2025-05-01 00:01:00 +0000
tags:
  proposal
  relocatability
  triviality
  type-erasure
  wg21
---

Here's a simple and trivially relocatable `MoveOnlyCallable` type. This is like
[`std::move_only_function<int(int) const>`](https://en.cppreference.com/w/cpp/utility/functional/move_only_function),
except that the "take `int` and return `int`" part is hard-coded, for simplicity.
([Godbolt.](https://godbolt.org/z/aheGT4ssT))

    class [[trivially_relocatable]] MoveOnlyCallable {
    public:
      static constexpr int CAPACITY = 16;
      static constexpr int ALIGNMENT = 8;

      template<class T, class DT = std::decay_t<T>>
      static constexpr bool use_small_storage() {
        return sizeof(DT) <= CAPACITY &&
               alignof(DT) <= ALIGNMENT &&
               std::is_trivially_relocatable_v<DT>;
      }

      explicit MoveOnlyCallable() noexcept {
        s.call_ = call_empty;
        s.destroy_ = destroy_empty;
      }

      template<class T, class DT = std::decay_t<T>>
        requires (!std::is_same_v<DT, MoveOnlyCallable>)
      MoveOnlyCallable(T&& t) {
        if constexpr (use_small_storage<DT>()) {
          ::new ((void*)s.data_) DT(std::forward<T>(t));
          s.call_ = [](const void *data, int arg) -> int {
            const DT& dt = *static_cast<const DT*>(data);
            return dt(arg);
          };
          s.destroy_ = [](void *data) { static_cast<DT*>(data)->~DT(); };
        } else {
          *reinterpret_cast<DT**>(s.data_) = new DT(std::forward<T>(t));
          s.call_ = [](const void *data, int arg) -> int {
            const DT& dt = **static_cast<DT* const*>(data);
            return dt(arg);
          };
          s.destroy_ = [](void *data) { delete *static_cast<DT**>(data); };
        }
      }

      MoveOnlyCallable(MoveOnlyCallable&& rhs) noexcept : s(rhs.s) {
        rhs.s.call_ = call_empty;
        rhs.s.destroy_ = destroy_empty;
      }
      MoveOnlyCallable& operator=(MoveOnlyCallable&& rhs) noexcept {
        auto copy = std::move(rhs);
        copy.swap(*this);
        return *this;
      }
      ~MoveOnlyCallable() {
        s.destroy_(s.data_);
      }
      int operator()(int arg) const {
        return s.call_(s.data_, arg);
      }
      void swap(MoveOnlyCallable& rhs) noexcept {
        std::swap(s, rhs.s);
      }

    private:
      static int call_empty(const void*, int) { return -999; }
      static void destroy_empty(void*) {}

      struct {
        int (*call_)(const void *data, int arg);
        void (*destroy_)(void *data);
        alignas(ALIGNMENT) char data_[CAPACITY];
      } s;
    };

This class type can declare itself unconditionally `[[trivially_relocatable]]`
because it takes care to use its "small storage" buffer `s.data_` only for callables
that are themselves trivially relocatable (and which fit in the buffer): that's
the purpose of its `use_small_storage<DT>()` discriminator.

In `MoveOnlyCallable`'s "empty state," the `call_` and `destroy_` members are
set to no-ops. `MoveOnlyCallable`’s move-constructor first relocates the callable from
source to destination, and then resets the source into this empty state.
Since the `data_` array only ever holds a trivially relocatable `DT` or a trivially copyable `DT*`,
this relocation can always be accomplished via `memcpy`. In fact, we don't even need an
explicit `memcpy` in the code: the member-initializer `s(rhs.s)` accomplishes the same thing.

`MoveOnlyCallable` relies on the fact that trivially relocatable types
can be relocated bytewise. It doesn't have to store a third function pointer for the
`relocate_` operation, because the relocation operation is always the same — always trivial.
This is a special kind of type erasure — where we arrange to store only types that meet some
checkable precondition, in order to avoid the need for a type-specific function pointer.
`string_view` does the same thing: it deliberately restricts itself to storing only
_contiguous ranges_ of `char`, so that its iterators can use pointer arithmetic instead of
needing to remember (at runtime, via a stored function pointer)
how the wrapped range's `operator++` works.

Suppose `MoveOnlyCallable` is asked to store an instance of callable type `Alpha`:

    struct Alpha {
      std::unique_ptr<int> p_ = std::make_unique<int>(42);
      int operator()(int x) const { return x + *p_; }
    };

Since `sizeof(Alpha) <= CAPACITY` and `alignof(Alpha) <= ALIGNMENT` and `std::is_trivially_relocatable_v<Alpha>`,
an instance of `Alpha` will be constructed directly in the small storage buffer `data_`.
Contrariwise:

    struct Beta {
      std::set<int> set_ = {42};
      int operator()(int x) const { return x + *set_.begin(); }
    };

Since `!std::is_trivially_relocatable_v<Beta>`, an instance of `Beta` will be constructed
on the heap and we'll store a `Beta*` in `data_`.

## C++26 intends to break `MoveOnlyCallable`

Unfortunately for C++ users, there are two competing models of "trivial relocation."
There's the "P1144" model everyone uses in practice, and then there's the "P2786" model
that was voted into C++26 in Hagenberg in February despite
[loud technical objections from the userbase](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2024/p3236r1.html)
about problems which [remain unaddressed](https://quuxplusone.github.io/draft/d1144-object-relocation.html#intro).

`MoveOnlyCallable` hits one or two of those interrelated technical objections:

- C++26's `is_trivially_relocatable` returns `true` for some types whose move-and-destroy
    operation is _not_ tantamount to memcpy, but in fact requires some "fixup."

- _Therefore_, even if you have checked that a type satisfies C++26's `is_trivially_relocatable`
    trait, it's still a bug to relocate it bytewise (as `MoveOnlyCallable` does). In the P2786 model,
    you cannot relocate an arbitrary (type-unknown) object; you must know its dynamic type
    statically, so that the compiler can generate type-specific codegen to move and destroy it.

- _Therefore_, `MoveOnlyCallable`'s type-erasure technique is not implementable in C++26.

Consider the following two Rule-of-Zero types ([Godbolt](https://godbolt.org/z/Er4Wjd4d7)):

    struct Gamma {
      void *p_;
      int y_ = 1;
      int operator()(int x) const { return x + y_; }
    };

    struct Delta {
      int y_;
      virtual int y() const { return y_; }
      int operator()(int x) const { return x + y(); }
    };

Consider the codegen required to correctly (that is, safely) relocate each of these types.
For `Gamma` the codegen is trivial (bytewise); for `Delta` it's not.

{% include 2025-05-01-table1.html %}

Here `_ZTVDelta` is the vtable for the `Delta` class. And — what's that `pacdza` instruction in the ARM code?
That's ARM64e [pointer authentication](https://clang.llvm.org/docs/PointerAuthentication.html),
a relatively new feature which you can find already used
[in the `setjmp` implementation on M1 Macs](https://stackoverflow.com/questions/78827888/which-registers-are-stored-in-jmp-buf-in-apple-silicon-arm64-environment#:~:text=pointer%20authentication).
On ARMv8.3-a platforms, the `Delta` object's vptr must be _cryptographically signed_ in order for the destination object to make use of it.

> Ptrauth _can_ involve ["peppering"](https://en.wikipedia.org/wiki/Pepper_(cryptography))
> with the address of the pointer itself, such that even memcpy'ing the vptr from one object to another
> of the same dynamic type would cause the program to abort at runtime. However, by default the "pepper" is zero
> (see: `pacdza` rather than `pacda`). This suffices to defeat [vptr-smashing attacks](https://phrack.org/issues/56/8).
> Still, the signature _is_ "salted" with the dynamic rather than static type of the object, such that
> memcpy'ing the vptr from a derived object into an object of the base type, or into an object of a different
> derived type, will cause the program to abort at runtime.
>
> Clang trunk also exposes this functionality directly, via custom type-qualifiers. ([Godbolt.](https://godbolt.org/z/4bqEcThW6))

{% include 2025-05-01-table2.html %}

Here's how the two competing relocation models handle this kind of situation:

- P1144: `is_trivially_relocatable` should be true if and only if the relocation operation is bytewise and
    can be type-erased into a simple byte copy.
    Relocating `Delta` is non-trivial (just like move-constructing it). Therefore, `is_trivially_relocatable_v<Delta>`
    should be `false`. `MoveOnlyCallable` can detect that `Delta` does not meet its checkable precondition, store
    it on the heap instead of in the small-storage buffer, and the code as presented Just Works.

- P2786/C++26: `is_trivially_relocatable` can be true at other times, too. In this model, relocating a
    "trivially relocatable" object can do pretty much anything — `MoveOnlyCallable` cannot assume that
    a simple byte-copy will safely relocate any "trivially relocatable" object.

Our `MoveOnlyCallable` is valid C++23 as written except for its use of `std::is_trivially_relocatable_v`;
we can substitute `absl::is_trivially_relocatable` or
[any other third-party P1144 implementation](https://docs.google.com/presentation/d/1kB5INtwPaZ_yrVUTgmPa1obiW0L_jOXKzw41mBcGzGg/edit#slide=id.g274cf5db546_0_243)
there.
Suppose WG21 sticks to its decision not to forward P1144 nor even to discuss it in committee.
Will we be able to switch from `absl::is_trivially_relocatable` to `std::is_trivially_relocatable` in C++26?
Certainly not without making some tradeoff. There are two ways the author of `MoveOnlyCallable` might
address that tradeoff:

<b>(A: Keep correctness and performance, but don't use P2786)</b> The author of `MoveOnlyCallable` decides to preserve the outcome that
the small storage is always bytewise copyable, so that class `MoveOnlyCallable` will remain trivially
relocatable itself. C++26 offers no type-trait for this property (P2786 `std::is_trivially_relocatable`
does not suffice), so he'll just keep using `absl::is_trivially_relocatable`, `amc::is_trivially_relocatable`,
or whatever he's using today. These third-party traits
([here's a list](https://docs.google.com/presentation/d/1kB5INtwPaZ_yrVUTgmPa1obiW0L_jOXKzw41mBcGzGg/edit#slide=id.g274cf5db546_0_243))
generally fall back to `std::is_trivially_copyable` in the
absence of compiler support for P1144, and will continue to do so even if C++26 ships P2786, since the third-party
users themselves generally fall into this category for whom P2786 isn't safe to use.

<b>(B: Use P2786, but lose performance)</b> The author of `MoveOnlyCallable` decides to preserve the outcome that
every suitably sized `is_trivially_relocatable` object is stored in the small storage.
`DT`'s relocation behavior can always be
expressed in terms of the generic algorithm [`std::trivially_relocate`](https://isocpp.org/files/papers/P2786R13.html#definitions-for-relocation-functions-obj.lifetime);
but that algorithm has different codegen depending on the internal details of the type being relocated.
In P2786-land, we make these changes to `MoveOnlyCallable` ([Godbolt](https://godbolt.org/z/Ycv5TbKfh)):

- <b>(B1)</b> It will store an additional pointer `this->s.relocate_` holding a pointer to a type-specific
    function defined in terms of `std::trivially_relocate`. (So its size will increase.)

- <b>(B2)</b> It will _not_ be marked `trivially_relocatable_if_eligible replaceable_if_eligible`
    (thank heaven for small mercies). In fact, if you _did_ mark it thus, you'd have a serious safety bug,
    because `std::trivially_relocate<MoveOnlyFunction>` would relocate `data_` according to its static type
    (array of char) and not according to the dynamic type of the `DT` stored within.

- <b>(B3)</b> Its `swap` member function must be written longhand in terms of three calls to
    `s.relocate_`, rather than delegating to a simple (bytewise) `std::swap`.

There's a third option: Use P2786, but lose correctness (e.g. by failing to navigate pitfalls B2 and B3).
I worry that some authors will make that choice without realizing it.

## Can't I just...?

Suppose I maintain a library such as Abseil or Folly that today relies on P1144-style relocation;
or I'm writing a new library that uses that style because it's the safest and most common style,
widely used in industry. Can't I just define my own trait as a _combination_ of C++26 traits?

    namespace fantasy_absl {
      template<class T>
      inline constexpr bool is_trivially_relocatable_v = // attempt an Abseil-friendly definition in C++26
        std::is_trivially_relocatable_v<T> &&            // start with P2786's definition
        std::is_replaceable_v<T> &&                      // exclude types like tuple<int&>
        !std::is_polymorphic_v<T>;                       // exclude types like Delta
    }

This definition successfully rejects `Delta`, because `Delta` is polymorphic.
But it fails to compose through Rule-of-Zero types.

    struct Carrier {
      Delta d_;
    };

`Delta` is P2786-trivially-relocatable and P2786-replaceable; it is excluded from
`fantasy_absl::is_trivially_relocatable_v` only because it is polymorphic. `Carrier`
remains unsafe to bytewise-copy (because it contains a `Delta` member), but `Carrier`
*itself* is not polymorphic, so it is *not* excluded and
`fantasy_absl::is_trivially_relocatable_v<Carrier>` remains `true`.
Boom goes the dynamite.

The only winning move is not to play. P2786's `is_trivially_relocatable` is not
even a _building block_ toward correctness/safety. In order to write correct and performant
code, the author of `MoveOnlyCallable` must ignore the siren songs of
`trivially_relocatable_if_eligible` and `std::trivially_relocate`:

- Adding `trivially_relocatable_if_eligible` introduces a correctness bug (B2).

- Using `std::trivially_relocate` sacrifices both code size and data size (B1).

Now, there are no users of P2786-style relocation in the wild today.
However, after C++26 is released, I do expect to see some authors lured toward
these facilities based purely on the sound of their "trivial" names...
and being wrecked on dangerous rocks.

## Now that you've read this far...

I intend to bring a revision of P1144 in the May mailing with (for the first time)
a number of coauthors, including some of the signatories of P3236. Since P2786 has
been merged into the Working Draft, this new version of P1144 will (for the first time)
be expressed as
[a diff against](https://quuxplusone.github.io/draft/d1144-object-relocation.html#scope)
what is now P2786-in-the-Working-Draft, showing exactly what parts of the C++26 wording
must change in order to make what remains both correct and performant.

If you'd like to be listed among the supporters of P1144R13, please
[send me an email](mailto:arthur.j.odwyer@gmail.com).

----

See also:

* ["Constexpr `Optional` and trivial relocation"](/blog/2025/05/08/siren-song-2-optional/) (2025-05-08)
