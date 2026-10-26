# Requirements — v1

What must be observably true, and what is not being built. A change is a new version
(`REQUIREMENTS_v2.md`), never an edit to this one. See
[SE1/R1–R11](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/04-requirements.md).

Client: Warren MacEvoy (owner; the users are students reading the code) · Date sent: 2026-10-26 ·
*Tell us what is wrong with it.*

**Every requirement below is *(inferred)*** — read off the code, CLAUDE.md and the commit history on
2026-10-26, not agreed with the client
([SE1/W12](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/08-work.md#w12)). Each
loses the mark when the client confirms it.

## Glossary

| Term | Meaning |
| ---- | ------- |
| class | A C struct, a vtable struct, a `Type`, and the `Class_*` functions over them. |
| vtable (`vftbl`) | Per-class `static const` table of function pointers; first slot is the class's `Type`. |
| slot | One function pointer in a vtable; always typed on the root base (`Bottle *`). |
| prefix layout | A derived struct repeats its base's members first, in the same order. |
| dispatcher | Public `Class_method(me, …)`; calls through `me->vftbl`. The virtual call. |
| implementation | `Class_vf_method`; the behaviour behind a slot. "Protected": derived classes only. |
| `Type` | `{ name, parent, size }`, one per class; reached from an object via its vtable. |
| checked downcast | `Class_cast(p)`: `p` if its dynamic type is-a `Class`, else null. |
| `init` / `fini` | Construct / finalize in caller-supplied storage. |
| `new` / `delete` | `malloc` + `init` / `fini` + `free`. |
| wrapper | The C++ `cxx::` class over a C class. |
| binding | The Python (`python/oopc.py`) or JavaScript (`js/oopc.mjs`) class over a C class. |
| host | The language runtime calling the C ABI: C++, Python, JavaScript. |
| inverted model | `cpp1/` or `cpp2/`: C++ as the source, emitting the same C ABI. |
| trace | `TRACE(...)` output; compiled out without `-DDEBUG`. Not the demo's `printf`s. |

## The pain

Students learning object orientation see it as language magic. Hand-rolled C OOP examples exist, but
they are usually unsound (function-pointer casts, mutable lazily-built tables) and stop at C. This
repository shows the whole mechanism in a few hundred lines, correct under the sanitizers, and shows
the same objects living natively in three other languages.

## People this touches

- **Students** reading the code — the primary users. They need it short, and every rule explained.
- **The instructor** who presents and maintains it.
- **Anyone copying the pattern** into real code, who will inherit its footguns.

## Requirements

### Object model (C)

- **OM-1** *(since v1)* A derived object can be passed to any function taking its base type with a
  plain pointer conversion, and every public method called that way runs the derived override.
- **OM-2** *(since v1)* `sizeof(Bottle) == 16` and `sizeof(SizedBottle) == 24` on 64-bit targets:
  one vtable pointer per object, nothing else.
- **OM-3** *(since v1)* No function pointer is ever cast; a slot implemented with the wrong type is
  a compile error.
- **OM-4** *(since v1)* Vtables live in read-only memory and require no runtime initialization.
- **OM-5** *(since v1)* `Bottle_delete` on a derived object finalizes the derived part, then the
  base part, exactly once each.
- **OM-6** *(since v1)* `SizedBottle` clamps every volume set through any dispatcher to
  `[0, maxVolume]`; a negative `maxVolume` is stored as 0; a new `SizedBottle` has `maxVolume` 1.

### Runtime types

- **TY-1** *(since v1)* Every object reports its dynamic class name and parent chain, with no
  per-object storage beyond the vtable pointer.
- **TY-2** *(since v1)* `Class_cast(p)` returns `p` when `p`'s dynamic type is `Class` or derives
  from it, and null otherwise, including when `p` is null.
- **TY-3** *(since v1)* `Type.size` is enough for a host to allocate an object, `init` it, and
  `fini` it, with C never allocating.

### C++ wrapper

- **CXX-1** *(since v1)* `sizeof(cxx::X) == sizeof(::X)` and alignments match, at every level,
  checked at compile time.
- **CXX-2** *(since v1)* The wrappers contain no C++ vtable pointer.
- **CXX-3** *(since v1)* Calling through `cxx::Bottle&` on a `cxx::SizedBottle` runs the derived
  override; destruction finalizes exactly once.
- **CXX-4** *(since v1)* The wrapper compiles as C++11.

### Bindings

- **BND-1** *(since v1)* In Python and JavaScript, a derived class declares only the methods it adds;
  inherited methods reach the C override. No per-class C code exists for either.
- **BND-2** *(since v1)* Each binding exposes the checked downcast (TY-2) and host-owned memory
  (TY-3).
- **BND-3** *(since v1)* The JS binding runs unchanged over an Emscripten module and over a raw
  `WebAssembly.Instance`.
- **BND-4** *(since v1)* The raw wasm build needs no libc and no Emscripten, and is under 4 KB
  *(invented: the code measured ~2.9 KB; the limit is a guess)*.

### Tracing

- **TR-1** *(since v1)* Without `-DDEBUG` no trace is emitted and no trace code is linked; with a
  stale format string, the build fails either way.
- **TR-2** *(since v1)* A `-DDEBUG` wasm build's traces appear on the JS console.

### Inverted models

- **INV-1** *(since v1)* `cpp1/` and `cpp2/` each export the C ABI of `src/*.c` (less the
  `*_vftbl` symbols in `cpp1/`), such that the unmodified `src/cmain.c` produces byte-identical
  output and the unmodified Python suite passes.
- **INV-2** *(since v1)* `cpp2/oopcc.py` refuses any input outside the flattenable subset, rather
  than emitting C that means something else.

### Build

- **BLD-1** *(since v1)* The tree builds with no warnings under `-Wall -Wextra -Wpedantic`, and with
  no UBSan report, on Linux x64, Linux arm64 and macOS arm64.

## Inputs: absent, late, wrong, lying

The inputs are the pointers and doubles a host passes across the C ABI. Nothing is late: every call
is synchronous.

| Input | Absent | Late | Wrong | Lying |
| ----- | ------ | ---- | ----- | ----- |
| `me` pointer to a dispatcher | null: crash (undefined) | — | freed or uninitialized: undefined | non-object: undefined; only `Class_cast` checks, and only the vtable's first word |
| pointer to `Class_cast` / `Type_of` | null → null | — | — | as above: it trusts the first word |
| `double` volume | — | — | negative → clamped to 0 by `SizedBottle`, stored by `Bottle`; NaN → stored by both (see Not yet answered) | — |
| `malloc` in `Class_new` | null → returns null | — | — | — |

## Operating envelope

- **Scale:** one demo hierarchy, two classes. Not designing for deep hierarchies, multiple
  inheritance or interfaces.
- **Speed:** a virtual call costs one indirection; not otherwise measured.
- **Resources:** OM-2, CXX-1, BND-4.
- **Recovery / availability / interruption:** not applicable; a library, not a service.

## Security, law, and ethics

No data about people. The C ABI trusts its callers completely (see the *lying* column); that is
acceptable for a teaching demo and is the first thing anyone copying it into production must
address. Licensed as in `README.md`.

## Assumptions (K4s), read back to the client

- "We're assuming the hand-written `src/*.c` stays the teaching artifact, and `cpp1/`/`cpp2/` never
  replace it. True?" — _unanswered_
- "We're assuming 64-bit targets only for OM-2's numbers; wasm32 sizes are not promised. True?" —
  _unanswered_

## Scope decisions

- Single inheritance only — it is the case that maps 1:1 onto every host language.
- C++ polymorphism goes through references, not `delete` on a base pointer — a virtual destructor
  would add a second vtable pointer (CXX-2).

## Non-goals

- Not building multiple inheritance, interfaces, or generics.
- Not generating the C from a schema or IDL; `src/` is hand-written on purpose.
- Not a production object system: no thread-safety beyond const tables, no input validation beyond
  `Class_cast`.

## Not yet answered

- Should `setMaxVolume` below the current volume re-clamp the volume? Today it does not, so
  `getVolume() > getMaxVolume()` is reachable. — client
- Should NaN be rejected or clamped by `SizedBottle`? Today it is stored. — client

## Deferred

- *Not yet:* `align` in `Type` — needed only for an over-aligned class; changes the `Type` layout the
  JS binding reads by offset.
- *Not yet:* exceptions across the inverted models' entry points — triggered by any non-teaching use.
