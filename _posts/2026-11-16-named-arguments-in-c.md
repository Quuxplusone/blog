---
layout: post
title: "C's designated initializers give us named arguments"
date: 2026-11-16 00:01:00 +0000
tags:
  abi
  designated-initializers
  library-design
  pearls
  preprocessor
---

In my [previous post](/blog/2026/11/10/designated-or-lambda/)
I talked about C++'s "relatively nerfed" form of designated initializers.
This time I want to show you a neat thing you can do with C's full-power designated
initializers. I first saw this pattern in Ben Klemens' book [_21st Century C_](https://link.amazon/B01S8VI8a) (2014),
pages 226–228. ([Godbolt.](https://godbolt.org/z/sq9oT4ETr))

    struct IdealVolumeArgs {
      double atm;      // P
      double moles;    // n
      double kelvin;   // T
    };
    #define ideal_volume(...) ideal_volume_impl( \
      (struct IdealVolumeArgs){ \
        .atm=1, .moles=1, .kelvin=273.15, __VA_ARGS__ \
      })
    double ideal_volume_impl(struct IdealVolumeArgs args) {
      // The ideal gas law: PV = nRT
      return args.moles * 0.082057366 * args.kelvin / args.atm;
    }

    int main() {
      printf("1 mole of gas at STP occupies %g L\n",
        ideal_volume());
      printf("1 mole of gas at 100°C occupies %g L\n",
        ideal_volume(.kelvin = 373.15));
      printf("2 moles of gas at 100°C occupies %g L\n",
        ideal_volume(.kelvin = 373.15, .moles = 2));
      printf("2 moles of gas at 0°C and 2 atm occupies %g L\n",
        ideal_volume(.moles = 2, .atm = 2));
    }

This idiom gives us named, defaultable arguments with natural function-call syntax.
It relies on two features of C's designated initializers that C++20 didn't adopt:

- They can appear in any order: `(struct IdealVolumeArgs){ .atm=1, .moles=2 }` means
    the same thing as `(struct IdealVolumeArgs){ .moles=2, .atm=1 }`.

- The same designator can appear multiple times, and only the last one "sticks":
    `(struct IdealVolumeArgs){ .atm=1, .moles=2, .atm=3 }` means the same thing
    as `(struct IdealVolumeArgs){ .atm=3, .moles=2 }`.

> That second bullet means we're relying on something that would be bad style if it
> weren't intentional. Clang emits a <nobr><code>-Winitializer-overrides</code></nobr> diagnostic by default.
> GCC (more sensibly IMHO) emits <nobr><code>-Woverride-init</code></nobr> only with <nobr><code>-Wextra</code>.</nobr>

Once a function uses this idiom, adding a new named parameter to the function definition
is no longer an API break! For example, we might decide to
let the caller provide their own value for the ideal gas constant _R_. We simply add a new
field to the `Args` struct. All the old call-sites will still compile just fine, and have the
same behavior. (It's not an API break, but it _is_ an _ABI_ break: the callers needn't be
re-<i>written</i>, but they do need to be re-<i>compiled</i> against the new header.)

    struct IdealVolumeArgs {
      double atm;      // P
      double moles;    // n
      double R;        // R -- this new field changes ABI
      double kelvin;   // T
    };
    #define ideal_volume(...) ideal_volume_impl((struct IdealVolumeArgs){ \
      .atm=1, .moles=1, .R = 0.082057366, .kelvin=273.15, __VA_ARGS__ \
    })
    double ideal_volume_impl(struct IdealVolumeArgs args) {
      return args.moles * args.R * args.kelvin / args.atm;
    }

If you're writing a family of related functions, you can share the `Args` struct among
the whole family; for example, we might add a `.liters` field to our struct, rename it
`IdealGasArgs`, and define a whole family of functions using that bag of keyword arguments:

    #define ideal_pressure(...) ideal_pressure_impl((struct IdealGasArgs){ \
      .atm=NAN, .moles=1, .kelvin=273.15, .liters=1, __VA_ARGS__ })
    #define ideal_amount(...) ideal_amount_impl((struct IdealGasArgs){ \
      .atm=1, .moles=NAN, .kelvin=273.15, .liters=1, __VA_ARGS__ })
    #define ideal_temperature(...) ideal_temperature_impl((struct IdealGasArgs){ \
      .atm=1, .moles=1, .kelvin=NAN, .liters=1, __VA_ARGS__ })

You can also prepend any number of non-keyword arguments:

    typedef struct { int cx, cy, r; } CircleArgs;
    void sc_circle_impl(ScreenContext *, int opcode, CircleArgs);
    #define sc_erase_circle(sc, ...) sc_circle_impl(sc, 0, (CircleArgs){__VA_ARGS__})
    #define sc_draw_circle(sc, ...)  sc_circle_impl(sc, 1, (CircleArgs){__VA_ARGS__})
    #define sc_fill_circle(sc, ...)  sc_circle_impl(sc, 2, (CircleArgs){__VA_ARGS__})

## Usage in the wild

Should you go out and replace all your ordinary C code with this pattern? Absolutely not.
It's heavyweight, and "cute," compared to just writing an ordinary non-named-argument function like

    double ideal_volume(double atm, double moles, double kelvin) {
      return moles * 0.082057366 * kelvin / atm;
    }

which almost invariably is all you need. Still, a quick regex search of GitHub turns up
a few real-looking uses of the pattern. The first shareworthy use I found was in
[gab462/hof-flecs-raylib](https://github.com/gab462/hof-flecs-raylib/blob/5abf44930b4ddc92178749d82561ce3e5cc3a0cb/include/message.h#L97),
which uses it to write calls
[such as](https://github.com/gab462/hof-flecs-raylib/blob/5abf44930b4ddc92178749d82561ce3e5cc3a0cb/src/client/systems.c#L175)

    send_message(&globals.send_buf, msg, .from_id = globals.name);

The most high-profile project in which I found this pattern was
[EpicGames/raddebugger](https://github.com/EpicGames/raddebugger/blob/5b0d5f67a46af5fffa52322e95a116dabf393103/src/ui/ui_core.h#L955-L956):

    typedef struct UI_AnimParams UI_AnimParams;
    struct UI_AnimParams {
      F32 initial;
      F32 target;
      F32 rate;
      F32 epsilon;
      B32 reset;
    };
    internal F32 ui_anim_(UI_Key key, UI_AnimParams *params);
    #define ui_anim(key, target_val, ...) ui_anim_((key), &(UI_AnimParams){.target = (target_val), .rate = (ui_state->default_animation_rate), __VA_ARGS__})
    ~~~~
    F32 cursor_pixel_off__animated = ui_anim(ui_key_from_stringf(~~~~), cursor_pixel_off, .initial = cursor_pixel_off);
    ~~~~
    u->alive_t = ui_anim(ui_key_from_stringf(~~~~), 1.f, .rate = entity_alive_t_rate);
