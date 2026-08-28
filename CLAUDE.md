# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

A demonstration of single-inheritance object orientation in plain C, wrapped so the *same* hierarchy is usable from C++ at **zero memory or dispatch overhead**. `Bottle` is the base, `SizedBottle` the derived class. The C layer is the single source of truth; the C++ layer in `include/*.hpp` is a view over it, never a reimplementation.

The measurable goal: `sizeof(cxx::X) == sizeof(::X)` for every class, at any hierarchy depth, with no C++ vtable pointer anywhere. `src/cppmain.cpp` prints these sizes and `static_assert`s enforce them.

## Build & run

```bash
make all        # bin/cbottle (C demo), bin/cppbottle (C++ demo)
make clean
./bin/cbottle
./bin/cppbottle
```

There is no test suite. The demos are verified by reading their trace output — every function prints its own entry, so the output *is* the proof that dispatch went where it should.

Useful checks:

```bash
make clean && make CFLAGS="-g -Iinclude -fsanitize=undefined -fno-sanitize-recover=all" all
```

Note `CXXFLAGS` derives from `CFLAGS`, so a `CFLAGS` override reaches C++ too — but keep `-Iinclude` in it. ASan does not initialize on this macOS setup; UBSan works.

## Architecture

### The C object model (`include/*.h`, `src/Bottle.c`, `src/SizedBottle.c`)

Inheritance is by **prefix layout**; upcast is a plain pointer cast. Two invariants make that work, both manual — nothing checks them:

1. `SizedBottleVftbl` repeats every `BottleVftbl` slot first, same order, before adding new ones.
2. `struct SizedBottleStruct` repeats every `struct BottleStruct` field first (`vftbl`, `volume`), same order, before adding new ones.

Reordering anything in `Bottle.h` requires mirroring it into `SizedBottle.h` in the same commit.

**Every vtable slot takes `Bottle *` — the root base type — at every level, however deep.** That is the invariant that keeps the pattern cast-free: a derived class inherits a slot by naming the same function pointer (`SizedBottle`'s table lists `Bottle_vf_getVolume` verbatim), and each implementation downcasts its argument internally, which is an ordinary data-pointer cast. Never retype a slot to the derived class — that reintroduces a function-pointer cast, which is UB (C11 6.5.2.2p9) and traps under CFI. A mistyped slot is now a compile error (`-Wincompatible-function-pointer-types`).

Three layers per class:

- **`Class_vf_method` implementations** — the real behavior. Non-`static` and declared in the header under a "protected" comment, so a derived class in another translation unit can name them, both to inherit a slot and to chain to the base version. Not for application code.
- **`Class_vftbl()`** — returns a `static const` table, initialized at compile time, so it lands in read-only data (`__DATA_CONST,__const`). No lazy fill-in, hence no data race and nothing writable to corrupt.
- **Public `Class_method(me, ...)`** — thin dispatchers that go through `me->vftbl->method(...)`. That indirection *is* the virtual call; calling `Bottle_setVolume` on an upcast `SizedBottle*` runs the derived clamping logic. Never bypass it by touching `me->volume` directly.

Lifecycle is split so objects can live in caller-supplied storage — the C++ wrapper depends on this:

- `Class_init` / `Class_fini` operate on existing memory. Derived `_init` calls base `_init` first, then **overwrites** `me->vftbl` with the derived table and sets its own fields.
- `Class_new` / `Class_delete` are `malloc`+`_init` and `_fini`+`free`. One allocation at one address, so `free` through an upcast base pointer is correct.

**Footgun:** a derived class's `fini` slot chains to the base with a direct, non-virtual call — `Bottle_vf_fini(base)`. Writing the natural-looking `Bottle_fini(me)` re-dispatches through the vtable, lands back in the same function, and recurses forever.

### The C++ wrapper (`include/Bottle.hpp`, `include/SizedBottle.hpp`, namespace `cxx`)

The wrapper hierarchy **mirrors** the C layout rather than duplicating it. This is the part that is easy to get wrong — an earlier version embedded a C struct at *every* level, which produced two disjoint objects per instance, made base methods operate on an uninitialized one (segfault on first virtual call), and inflated `sizeof(cxx::SizedBottle)` to 40 vs. 24.

The rules, enforced by `static_assert` on size and alignment in each header:

- **Only `cxx::Bottle` embeds a C struct** (`private: ::Bottle _me`). It is the storage for the whole hierarchy.
- **Derived wrappers embed no C struct.** They reserve exactly the bytes their C struct adds beyond their base's (`alignas(::SizedBottle) char extra[sizeof(::SizedBottle) - sizeof(::Bottle)]`).
- **Every level defines a public `me()`, returning its own C type** (both a `const` and non-`const` overload) — the interop hook for handing an object to the C API. The base returns `&_me`; each derived level returns a downcast of *its parent's* `me()` — `reinterpret_cast<::SizedBottle*>(Bottle::me())`. No level reaches past its immediate parent's storage, which is why `_me` stays private. A derived `me()` hides the base's; write `sb.Bottle::me()` for the base view. Methods are always written in terms of `me()`, never raw storage.
- **Derived constructors** are `: Bottle(Bottle::UNINITIALIZED)` then their own C `_init` on `me()`. The tag suppresses base initialization because the derived `_init` initializes the whole object, prefix included.
- **Derived destructors are empty.** `~cxx::Bottle` calls `::Bottle_fini`, which dispatches through the C vtable and runs the full chain. Finalizing in a derived destructor too would run it twice.

`(void*)obj.me() == (void*)&obj` holds at every level. For `cxx::Bottle` that is **standard-guaranteed** — it is a standard-layout class, so `[class.mem]` promises a pointer to it points at its first member. Derived wrappers hold data in two classes (base + reserved bytes) and are therefore *not* standard-layout, so the standard promises nothing about where the base subobject sits; the Itanium ABI (gcc/clang) and MSVC both put it at offset 0. The size `static_assert` is what actually pins this down: base subobject 16 + `extra` 8 means any base offset `O > 0` would make the object `≥ O + 24` bytes and fail the assert. `me()` never computes an offset from `this` regardless — it downcasts its parent's `me()`.

**Polymorphism works through `cxx::Bottle&`** — the override is selected by the C vtable, so there is no C++ vptr in the object. `~cxx::Bottle` is deliberately **non-virtual**; making it virtual would add a second vtable pointer on top of the C one and defeat the whole exercise. The cost: never `delete` a derived wrapper through a `cxx::Bottle*`. Use stack values and references, or heap-allocate through the C API (`Bottle_new`/`Bottle_delete`), which dispatches correctly.

Adding a class means: `include/X.h` (vtable slots + declarations mirroring the base prefix), `src/X.c` (static impls, vtable wiring, dispatchers), `include/X.hpp` (the five rules above). All hand-written; nothing is generated.

## Known issues in the C layer (not yet addressed)

- No runtime type identity, so no checked downcast. Adding `parent` and `name` as the first two vtable slots costs two words per *class* and zero per object.
- Every function `printf`s unconditionally. Fine for a demo, wrong for a library — guard with `#if defined(DEBUG)` before this is consumed as one.


The tree builds warning-free under `-Wall -Wextra -Wpedantic`; keep it that way.
