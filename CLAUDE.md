# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

This repository follows the [software-engineering guide](https://github.com/wmacevoy/software-engineering/tree/v1.1).
Read [`AGENTS.md`](AGENTS.md) (the guide's working agreement, copied as-is) before acting; this file
wins where they disagree. The working set is in `docs/`: [`REQUIREMENTS_v3`](docs/REQUIREMENTS_v3.md)
(versioned, never edited in place — cite each with the version that last changed it, e.g. `REQ2/OM-6`), [`DESIGN_v1`](docs/DESIGN_v1.md)
(structure and *why*; the mechanism stays below), [`QUEUE`](docs/QUEUE.md), and
[`FINDINGS`](docs/FINDINGS.md).

## What this project is

A teaching demo of single-inheritance object orientation in plain C, wrapped so the *same* hierarchy appears natively in C++, Python and JavaScript with **no per-class glue in any of them**. `Bottle` is the base, `SizedBottle` the derived class; the domain is deliberately trivial so nothing distracts from the mechanism.

The C ABI is the interface. Every binding is written in its host language only:

| layer | files | costs |
|---|---|---|
| C | `include/*.h`, `src/*.c` | one vtable pointer per object, one vtable + one Type per class |
| C++ | `include/*.hpp` | zero extra bytes, no C++ vtable pointer |
| Python | `python/oopc.py` | ctypes; no extension module |
| JavaScript | `js/oopc.mjs` | wasm exports; no glue module |

Two measurable goals, both enforced by `static_assert` rather than by convention: `sizeof(cxx::X) == sizeof(::X)` at any depth, and no C++ vtable pointer anywhere.

## Build & test

```bash
make all          # bin/cbottle, bin/cppbottle, bin/liboopc.<dylib|so>
make test         # C, C++, Python  (test-c / test-cpp / test-python individually)
make clean
```

Every language has an assertion-based suite named `test_oopc` (`src/test_oopc.c`, `src/test_oopc.cpp`, `python/test_oopc.py`, `js/test_oopc.mjs`), and every check's label names the requirement it checks (`REQ2/OM-6`). `bin/cbottle` and `bin/cppbottle` are demos, run only to see they don't crash. A new check should be seen failing once before you trust it (SE1/T3). When you break and restore a file to do that, `make clean` afterwards: make's one-second timestamps can leave a stale object that still holds the break.

WebAssembly needs Emscripten, which CI installs — `make wasm && make test-js`. There is also a toolchain-free build: `make wasm-raw WASM_CC=<clang> WASI_SYSROOT=<sysroot>` targets wasm32 with the wasi-libc *headers* but links no libc, leaving `malloc`/`free` as imports the host supplies (see `js/load.mjs`). ~3.1 KB, runs in node and the browser with no shim, and `js/test_oopc.mjs` runs against whichever build is present.

Useful checks — CI runs all of these:

```bash
make clean && make CFLAGS="-g -Iinclude -fPIC -Wall -Wextra -Wpedantic -Werror" all
make clean && make CFLAGS="-g -Iinclude -fPIC -fsanitize=undefined -fno-sanitize-recover=all" all
make clean && make CFLAGS="-g -Iinclude -fPIC -DDEBUG" all      # tracing on
```

Note `CXXFLAGS` derives from `CFLAGS`, so a `CFLAGS` override reaches C++ too — but keep `-Iinclude -fPIC` in it. ASan does not initialize on this macOS setup; UBSan works.

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

`(void*)obj.me() == (void*)&obj` holds at every level. For `cxx::Bottle` that is **standard-guaranteed** — it is a standard-layout class, so `[class.mem]` promises a pointer to it points at its first member. Derived wrappers hold data in two classes (base + reserved bytes) and are therefore *not* standard-layout, so the standard promises nothing about where the base subobject sits; the Itanium ABI (gcc/clang) and MSVC both put it at offset 0. The size `static_assert` is what actually pins this down: base subobject plus `extra` equals `sizeof(::SizedBottle)`, so any base offset `O > 0` would make the object `O` bytes too large and fail the assert. `me()` never computes an offset from `this` regardless — it downcasts its parent's `me()`.

**Polymorphism works through `cxx::Bottle&`** — the override is selected by the C vtable, so there is no C++ vptr in the object. `~cxx::Bottle` is deliberately **non-virtual**; making it virtual would add a second vtable pointer on top of the C one and defeat the whole exercise. The cost: never `delete` a derived wrapper through a `cxx::Bottle*`. Use stack values and references, or heap-allocate through the C API (`Bottle_new`/`Bottle_delete`), which dispatches correctly.

Adding a class means: `include/X.h` (vtable slots + declarations mirroring the base prefix), `src/X.c` (static impls, vtable wiring, dispatchers), `include/X.hpp` (the five rules above). All hand-written; nothing is generated.

### The inverted models (`cpp1/`, `cpp2/`)

Two experiments that run the repository backwards: **C++ is the source and the C
ABI is the artifact**. Both emit the *same* symbols as `src/*.c`, so nothing
downstream changes — `bin/cbottle1` and `bin/cbottle2` are the unmodified
`src/cmain.c` linked against them, and `python/oopc.py` runs against
`bin/liboopc1` / `bin/liboopc2` through the `OOPC_LIB` environment override.
Each has its own README; read those before changing either.

The point of both: the four invariants listed above under *The C object model*
— mirrored vtable slots, mirrored struct fields, the hand-written downcast in
every override, and the `Bottle_fini`/`Bottle_vf_fini` recursion footgun — are
maintained by the compiler (`cpp1/`) or by the generator (`cpp2/`) instead of by
a human.

- **`cpp1/`** — header-only, C++17. `oopc::Slot` generates the slot thunks
  (a checked `static_cast` where the C writes `(SizedBottle*)base`),
  `oopc::Destroy` makes the fini slot call the most-derived destructor so C++
  chains automatically, and `oopc::Export` makes each `extern "C"` entry point
  one expression. **No `virtual` keyword anywhere**, deliberately: the vtable is
  authored so its layout is ours, `constexpr`, and readable from C. The object
  struct is *inherited* (compiler-computed casts); the vtable struct is *nested*
  (standard-layout, so the prefix is guaranteed rather than ABI-dependent), and
  a derived table is built by copying the base's and patching the overrides.
  Class constants live in `oopc::Class<C>` specializations *after* the class
  body, so constructors must be defined out of line.
  Deliberate ABI delta: `Bottle_vftbl`/`SizedBottle_vftbl` are not exported.

- **`cpp2/`** — `cpp2/oopcc.py`, a libclang generator. `cpp2/model/*.hpp` is
  ordinary polymorphic C++ (real `virtual`) that knows nothing about oopc;
  the generator emits `cpp2/gen/*.{h,c}` in the exact shape of `src/Bottle.c`.
  It refuses anything outside the flattenable subset rather than emitting C that
  quietly means something else — `cpp2/test_oopcc.py` has 12 refusal cases and
  4 acceptance cases, and every new subset rule needs a refusal test with it.
  Body translation is cursor-based, not textual; the qualified/unqualified call
  distinction is what decides `Base_vf_f` versus the dispatcher.

`make cpp1 cpp2 test-inverted`. `CXX17FLAGS` is separate from `CXXFLAGS` because
the `cxx::` wrapper stays at C++11. `bin/cpp2bottle` is built *without*
`-Iinclude`, since `include/SizedBottle.hpp` and `cpp2/model/SizedBottle.hpp`
share a basename and are different things.

### Runtime types (`include/Type.h`)

`Type { name, parent, size }` — three words per class, zero per object. Objects carry no type tag; they reach their `Type` through the vtable's first slot, so `Type_of` is one double indirection. `Type_isa` walks the parent chain; `Class_cast` is the checked downcast that makes the foreign bindings safe.

`size` is what lets a **host** allocate an object without knowing C's `sizeof`. With the `Class_init`/`Class_fini` split it means Python and JS can own the memory and C never needs an allocator — the C, Python and JS suites all exercise that path.

Every class exposes its Type two ways, deliberately: the `Class_type` object (for a derived class's static initializer), and `Class_getType()` (because a plain function is the one thing every FFI can reach — ctypes, raw wasm exports and Emscripten all differ on data symbols). For the same reason a host reads a `Type` through `Type_name`/`Type_parent`/`Type_size`, never by offset: the struct is 12 bytes on wasm32 and 24 on 64-bit targets. (Python's ctypes `Structure` is the exception; ctypes computes the layout for whatever platform it runs on.)

Portability is a requirement (REQ3/RD-1, REQ2/BLD-1, REQ2/BLD-2): 64-bit native, 32-bit x86 (where `Bottle` is 12 bytes, not 16) and wasm32 are all targets, but `src/` and `include/` may not contain a per-target `#if` that changes behavior (the `__GNUC__` guard in `Trace.h` only turns on checks, and says so). Never write a size as a number; compare with `sizeof`.

### Tracing (`include/Trace.h`)

`TRACE(...)` compiles to nothing without `-DDEBUG`, but expands to `if (0) Trace(...)` rather than nothing at all, so arguments stay type-checked and a stale format string is a compile error. Demo `printf`s in `cmain.c`/`cppmain.cpp` are *not* traces and stay unconditional.

`Trace_to()` redirects the sink at runtime. Under Emscripten stdout is already `console.log`, so a wasm `-DDEBUG` build traces to the JS console unaided. These writes go to fd 1 and bypass `sys.stdout`, so under CPython they interleave out of order and under Jupyter they miss the cell entirely.

### Bindings (`python/oopc.py`, `js/oopc.mjs`)

Both follow the same shape, and it is the whole point: **the host language's single inheritance maps 1:1 onto the C hierarchy.** A derived wrapper declares only the methods it *adds*. Inherited methods call the *base* C functions, which dispatch through the C vtable into the derived override. In JS, `SizedBottle` needs no constructor at all — `Bottle`'s uses `new.target._C` to pick the right C constructor.

Neither binding has per-class C code. Adding a class means adding a C class and a ~10-line subclass in each host language.

`js/oopc.mjs` is module-agnostic: it accepts either an Emscripten `Module` (functions at `Module._Name`) or a raw `WebAssembly.Instance`'s exports. Loading lives in `js/load.mjs`.

## CI

`.github/workflows/build-test.yml` — a native matrix (linux x64/arm64, macOS arm64) running the warning-free build, the test suite, UBSan and a tracing build; a wasm job that installs emsdk, builds, runs the JS suite, and asserts that `-DDEBUG` traces actually reach the JS console; a raw-wasm job (pinned wasi-sdk, no Emscripten) running the JS suite; an i386 job running the C and C++ suites at `-m32`; and a release job on `v*` tags. Artifacts upload per platform.

## Known issues

Tracked in [`docs/QUEUE.md`](docs/QUEUE.md). The one to know before trusting a green run:
`make test-c` and `make test-cpp` pass whenever the demos exit 0 — they assert no values, so only the
Python and JS suites catch a wrong answer (QUEUE #1). The wasm path is exercised in CI only; locally,
only `wasm-raw` builds without an Emscripten install.

The tree builds warning-free under `-Wall -Wextra -Wpedantic`; CI enforces it with `-Werror`.
