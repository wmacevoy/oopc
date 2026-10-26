# cpp-1 — C++ is the source, the C ABI is emitted by a header

The rest of this repository runs one way: C is the source of truth and
`include/*.hpp` is a thin C++ *view* of it. This directory runs the other way.
`cpp1/include/oopc/*.hpp` **is** the implementation, written in C++, and the C
ABI in `include/Bottle.h` / `include/SizedBottle.h` is what it emits.

Nothing else in the tree changes. `bin/cbottle1` is the unmodified `src/cmain.c`
linked against this build; `python/oopc.py` runs against it unchanged.

```bash
make cpp1 test-cpp1
```

## Why invert

`CLAUDE.md` lists four things `src/*.c` maintains by hand, with nothing
checking them. All four stop being human problems here:

| kept by hand in `src/*.c` | kept by the compiler in `cpp1/` |
|---|---|
| `SizedBottleVftbl` restates every `BottleVftbl` slot, in order | `Bottle::Vtbl base;` — one line, one time |
| `struct SizedBottleStruct` restates every base field, in order | `: public Bottle` |
| every override opens `SizedBottle *me = (SizedBottle*)base;` | `oopc::Slot` does a checked `static_cast` |
| a fini must chain with `Bottle_vf_fini`, never `Bottle_fini` | `~SizedBottle` is empty; C++ chains |

The fourth is worth dwelling on. `src/SizedBottle.c` carries a warning that
writing the natural-looking `Bottle_fini(me)` recurses forever. Here the fini
slot calls the *most-derived destructor* (`oopc::Destroy`), and the chain is the
language's job. The wrong version is not expressible.

A fifth falls out for free: **members with non-trivial destructors just work.** A
C `fini` has to release them field by field.

## What it does not use

There is no `virtual` keyword anywhere in the model. That is deliberate, and it
is the central design decision.

Using real `virtual` would let the compiler emit the vtable — and the compiler's
vtable is private. Under Itanium two words sit *below* the address point
(offset-to-top at `[-2]`, `type_info*` at `[-1]`), slot indices are not
contractual, a virtual destructor silently consumes **two** adjacent slots
(complete and deleting), MSVC differs, and the entry points come out
name-mangled where ctypes and `js/exports.json` cannot reach them.

So the vtable is authored, and its layout is ours: `constexpr`, hence
constant-initialized into read-only memory — no lazy fill-in, no data race, and
nothing writable to corrupt. Exactly the properties `src/Bottle.c` gets from a
`static const` initializer, kept rather than traded away.

The cost is that `virtual` becomes a convention: a public dispatcher plus a
`vf_` implementation, which is the Non-Virtual Interface idiom and is also
precisely what the C already does, named the same way.

## Nest the vtable, inherit the object

The asymmetry is on purpose.

**Object: inherit.** `class SizedBottle : public Bottle` gives compiler-computed
`static_cast` in both directions — correct even on an ABI that did not put the
base at offset 0.

**Vtable: nest.** `struct Vtbl { Bottle::Vtbl base; ... };` keeps every level
standard-layout, so "a derived table starts with the base table" is guaranteed
by `[class.mem]` rather than by ABI, and `reinterpret_cast` between a table and
its prefix is defined via pointer-interconvertibility at any depth.

Nesting also buys the one thing the C cannot express — building a derived table
by **copying the base and patching the overrides**, which is what a vtable
actually is:

```cpp
oopc1::SizedBottle::Vtbl v = { Class<oopc1::Bottle>::table, ...new slots... };
v.base.setVolume = OOPC_SLOT(oopc1::Bottle, &oopc1::SizedBottle::vf_setVolume);
```

`getVolume` is inherited by **not being mentioned**. `src/SizedBottle.c` must
restate `Bottle_vf_getVolume` verbatim, in the right position.

## Allocation vs. initialization

`Class_init(p)` is `::new (p) Class()` and `Class_fini(p)` runs the destructor
chain; `Class_new` is `malloc` plus the former. `Type::size` stays `sizeof(T)`
as a `constexpr`. The host-owned-memory path in `python/test_oopc.py` — allocate
a `ctypes` buffer, call `SizedBottle_init` on it — works against this build
unchanged.

One subtlety worth banking: `SizedBottle_init` in C calls `Bottle_init` and
*then* overwrites `me->vftbl`, so a virtual call made during base construction
reaches the base override — which is exactly what C++ mandates. The constructors
here do the same (`bind<SizedBottle>()` after `Bottle()`). Threading the final
table down through the base constructor instead would look like an optimization
and would silently invert that rule.

## What is still hand-written

`cpp1/src/exports.cpp`: one line per entry point. It cannot be shorter — C has
no reflection and neither does C++17, so nothing can enumerate a class's methods
and mint C names. C++26 static reflection (P2996) can enumerate members but
still does not hand you arbitrary `extern "C"` symbols, so some form of that list
survives. Against `src/Bottle.c`, which writes each method three times plus two
mirrored declarations, one line is the win.

## Deliberate ABI delta

`Bottle_vftbl` and `SizedBottle_vftbl` are declared by the C headers and are
**not** defined here. They exist so a derived class written in C, in another
translation unit, can name base slots in a static initializer; in the inverted
model that job belongs to C++ inheritance, and exporting a second copy of a
table would be a lie. Every other symbol is defined, including `Bottle_type` and
`SizedBottle_type` — `Type_isa` compares `Type` addresses, so the symbol the
header publishes and the one the vtable points at must be the same object.

## Standard-layout, honestly

`oopc1::Bottle` is standard-layout, so `(void*)this == (void*)&_vptr` is
guaranteed. `oopc1::SizedBottle` holds data in two classes and therefore is not,
so the standard says nothing about where the base subobject sits — the same gap
`include/SizedBottle.hpp` already documents, closed the same way: base 16 plus
`_maxVolume` 8 means a base at any offset `O > 0` would make the object at least
`O + 24` bytes and fail the `static_assert`.

C is no better off. The prefix-cast idiom `src/SizedBottle.c` rests on is only
formally blessed for a common initial sequence seen through a union. The
difference is that the casts *here* are compiler-computed and stay correct
regardless; only the C and host views of the bytes depend on offset 0.

## Verified

`make test-cpp1` checks, in order:

- `bin/cbottle` and `bin/cbottle1` produce **byte-identical** output — the same
  C program, one linked against `src/*.c`, one against this;
- `bin/cpp1bottle` uses the classes natively (stack objects, RAII, polymorphism
  through `oopc1::Bottle&`) and reports `sizeof` equal to the C structs;
- `python/oopc.py`, unmodified, passes its whole suite against
  `bin/liboopc1.<dylib|so>` via `OOPC_LIB`, including the host-owned-memory path.

Clean under `-Wall -Wextra -Wpedantic -Werror` and under UBSan.

## Trace deltas under `-DDEBUG`

Three, all structural, all expected:

1. **No `Bottle_init` line inside `SizedBottle_init`.** C's `SizedBottle_init`
   traces on entry and then calls `Bottle_init`; a C++ constructor has no place
   to trace before its base constructor runs, so the base's line is the one that
   goes missing.
2. **An extra `Bottle vf setVolume` line.** `SizedBottle::vf_setVolume` chains to
   `Bottle::vf_setVolume` — a qualified, therefore non-virtual, call. The C
   version writes `me->volume` directly instead.
3. **An extra `Bottle_fini` line on delete.** `Bottle_delete` here goes through
   the `fini()` dispatcher; `src/Bottle.c` calls `me->vftbl->fini(me)` directly
   while `SizedBottle_delete` calls `SizedBottle_fini`. The C is inconsistent
   between the two; this build is not.

## Requires

C++17 — `auto` non-type template parameters (the `oopc::Slot` and `oopc::Export`
partial specializations) and inline variables. `CXXFLAGS` for the existing
`cxx::` wrapper stays at C++11; `CXX17FLAGS` is separate.
