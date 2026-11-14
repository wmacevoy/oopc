# rust-inv — Rust is the source, the C ABI is the artifact

The rest of this repository runs one way: C is the source of truth, and
`rust/oopc.rs` is a thin Rust *view* of it. This directory runs the other way.
`bottle.rs` and `sized_bottle.rs` **are** the implementation, and the C ABI in
`include/Bottle.h` / `include/SizedBottle.h` is what they emit, symbol for
symbol, `Bottle_vftbl` and the `Bottle_vf_*` functions included.

Nothing else in the tree changes. `bin/cbottle-rust-inv` is the unmodified
`src/cmain.c` linked against this build, and the C suite, `python/oopc.py` and
`rust/oopc.rs` run against it unchanged (REQ7/INV-5).

```bash
pixi run make rust-inv test-rust-inv
```

## What the compiler keeps

`CLAUDE.md` lists what `src/*.c` maintains by hand. Read `sized_bottle.rs` next
to `src/SizedBottle.c`:

| kept by hand in `src/*.c` | here |
|---|---|
| `struct SizedBottleStruct` restates every base field, in order | `base: Bottle` is the first field of a `#[repr(C)]` struct |
| `SizedBottleVftbl` restates every `BottleVftbl` slot, in order | `base: BottleVftbl`, the same way |
| an inherited slot names `Bottle_vf_getVolume` again | `..Bottle_vftbl` copies every slot not overridden |
| a fini must chain with `Bottle_vf_fini`, never `Bottle_fini` | the slot is `drop_in_place`; dropping a `SizedBottle` drops its `base` |
| every override opens `SizedBottle *me = (SizedBottle*)base;` | **still by hand**, but once per class: `me` and `me_mut` |

The methods themselves are ordinary Rust (`impl SizedBottle`). An override calls
its base with `self.base.set_volume(value)`, a plain call. The dispatcher
`Bottle_setVolume` is not something a method reaches by accident, so the
runaway recursion `src/SizedBottle.c` warns about has no natural spelling.

## One object file, no Rust runtime

The build is one `rustc` command, with no cargo project and no crates:

```bash
rustc --edition 2021 --crate-type=lib --emit=obj -O -C panic=abort -o tmp/rust-inv.o rust-inv/oopc.rs
```

The crate is `#![no_std]`, and the result is an object file that links exactly
like `tmp/Bottle.o` and `tmp/SizedBottle.o`. Its only undefined symbols are
`malloc`, `free` and `Type_cast`; `src/Type.c` and `src/Trace.c` are used as they
are, not rewritten.

`-O` is part of that. It inlines the few `core` functions the code uses. If a
change makes the link fail on an undefined `core::…` symbol, the code now needs
something from Rust's libraries, and the object-file build no longer fits.

## What it does not do

- **It does not trace.** `TRACE` is a C macro over a variadic function; the C
  demo's output is identical without it.
- **It has no Rust-side demo.** The Rust suite in `rust/` runs against it
  through the C ABI instead: Rust, over C, over Rust.
- **It runs on the 64-bit native targets only.** Not 32-bit x86 or wasm32.
- **The `vftbl` field is a raw pointer, not a `&'static BottleVftbl`.** A
  derived object points at a longer table and casts back to it; a reference to
  the base table would not cover the rest.
- **Like the C, it trusts its callers.** Every exported function is `unsafe`.
