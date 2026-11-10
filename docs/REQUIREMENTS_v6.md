# Requirements — v6

What must be observably true, and what is not being built. A change in meaning is a new version; a
spelling or lint fix is not. See
[SE1/R1–R11](https://github.com/wmacevoy/software-engineering/blob/v1.7/guide/04-requirements.md).
Supersedes [v5](REQUIREMENTS_v5.md); IDs keep their meaning across versions, and a merged one is
retired rather than reused. Cite each requirement with the version that last changed it: `REQ6/BND-4`,
but still `REQ5/SHAPE-2` and `REQ2/OM-6`.

Client and proxy for adopters and students: Warren MacEvoy · Date agreed: 2026-11-09 · *Tell us what
is wrong with it.*

**What changed from v5:** the client dropped BND-4's size limit as arbitrary, and made "patch releases
only" after finishing a guideline: a new major version is allowed and unlikely. Directed by the
client, 2026-11-09.

**What changed from v4 (in v5):** a real adopter showed a third inverted route, the handle. SHAPE-2 names it, as
documentation only, with its costs listed under Exceptions; the deferred error channel records the
convention adopters use. Agreed by the client, 2026-11-09.

**What changed from v3 (in v4):** the client wrote a Mission and a Fundamental shape
([SE1/D9](https://github.com/wmacevoy/software-engineering/blob/v1.7/guide/05-design.md#d9)). The aim
is now a pattern to adopt; v2's "both a working library and a teaching instrument" is superseded, and
adopters become a named audience. `cpp1/` and `cpp2/` are one of two directions the shape is about,
and both are patterns to adopt. Hence INV-3, INV-4, BLD-3 and ADP-1 are new, and the deferred item on
exceptions has a new trigger. No existing requirement changed, so existing citations stay current.

## Glossary

The terms in [v1](REQUIREMENTS_v1.md#glossary), unchanged, and these:

| Term | Meaning |
| ---- | ------- |
| pattern | The technique shown here, as opposed to this code: what an adopter copies. |
| adopter | Someone who copies the pattern into their own code. |
| gateway | The C ABI, as the one way every other language reaches the hierarchy. |
| principle direction | A C API wrapped as a C++ API (`cxx::`). |
| inverted direction | A C++ API wrapped as a C API (`cpp1/`, `cpp2/`), which then serves as the gateway. |
| footgun | A misuse the pattern allows and nothing in it catches. |
| handle | An opaque C type owning a reference to a C++ object, so nothing of its layout crosses the gateway. |

## Mission

- **MISSION-1** *(since v4)* *(judged)* Show how single inheritance really works, in a few hundred
  lines a student can read, correct enough to use as a pattern to adopt. Judge: Warren MacEvoy, when
  presenting it. Veto: a sanitizer report or a failing suite on any target.

## Fundamental shape

Held by: Warren MacEvoy. The holder proposes a change; the client makes it, in a new version.
Mechanisms are in the design and [`CLAUDE.md`](../CLAUDE.md#architecture).

- **SHAPE-1** *(since v4)* *(judged)* The C ABI is the gateway. Every other language reaches the
  hierarchy through it, written only in that language, with no per-class glue. Judge: the holder, at
  each requirements version.
- **SHAPE-2** *(since v5)* *(judged)* C++ meets the gateway in two directions. *Principle:* a C API
  wrapped as a C++ API (`cxx::`), adding no bytes. *Inverted:* a C++ API wrapped as a C API (`cpp1/`,
  `cpp2/`), which then serves as the gateway to other languages. Both directions present the same
  ABI, and both inverted routes shown here are patterns to adopt. A third inverted route, the handle,
  is recommended where a host needs the gateway and not the inheritance; it is documented in
  ADP-1's document and not built here. `cpp2/` is the more legible, since its source
  is ordinary C++, and is adopted with caution: a generator can make its two sides disagree silently,
  and that drift is K3. An adopter of an inverted route should test both sides, as a guideline; in
  this repository's examples it is a requirement (INV-3). Judge: the holder, at each requirements
  version.
- **SHAPE-3** *(since v4)* *(judged)* Inheritance is the same in every layer: a derived class declares
  only what it adds, so adding a class repeats the same steps. Judge: the holder, at each requirements
  version.
- **SHAPE-4** *(since v4)* *(judged)* Nothing is hidden: an object is its vtable pointer plus its
  fields, and any extra cost has to be stated and measured. Judge: the holder, at each requirements
  version.
- **SHAPE-5** *(since v4)* *(judged)* Plain beats clever: portability comes from plain C, and the
  domain stays trivial so nothing distracts from the mechanism. Judge: the holder, at each
  requirements version.
- **Finished:** two classes, both C++ directions, all targets in the operating envelope green; after
  that, patch releases, as a guideline. A new major version is allowed and unlikely.
- **Foundation:** `SE1`
- **Says no to:** multiple inheritance, interfaces and generics (they do not map 1:1 onto every host);
  generating `src/` (hand-written on purpose); a virtual destructor in the wrapper (a second vtable
  pointer); extension modules (per-class glue); per-target `#if` (SHAPE-5); a more interesting domain
  (it distracts).
- **Exceptions:** the `__GNUC__` guard in `Trace.h`, which only turns on compiler checks (RD-1).
  `cpp1/` is C++17 and `cpp2/` needs libclang, because the inverted direction has C++ as its source;
  SHAPE-5's plain C holds for `src/` and `include/`. The handle route stops inheritance at the gateway
  (SHAPE-3) and costs a smart pointer and its control block (SHAPE-4); it is the case where real
  C++ holds what the prefix layout cannot.

## The pain

Object orientation is taught as language magic. Hand-rolled C versions exist but are usually unsound
(function-pointer casts, mutable lazily built tables) and stop at C. This shows the whole mechanism
in a few hundred lines that are correct under the sanitizers, sound enough to copy, and visible
natively in three other languages.

## People this touches

Students reading the code and adopters copying the pattern (both primary); the instructor who
presents it. An adopter inherits the footguns, so ADP-1 is written for them.

## Requirements

### Readability

- **RD-1** *(since v3)* `src/` and `include/` contain no conditional on target, compiler or word
  size that changes behavior. Portability is bought with plain C, not with `#if`. A conditional
  whose only job is to turn on a compiler's checks is allowed, and says so in a comment.

### Object model

- **OM-1** *(since v1)* A derived object passes to any function taking its base with a plain pointer
  conversion, and every dispatcher called that way runs the derived override.
- **OM-2** *(since v2)* An object is its vtable pointer plus its fields: `sizeof` each class equals
  that of a plain struct with the same members, on every target.
- **OM-3** *(since v2)* No function pointer is ever cast — a mistyped slot is a compile error — and
  vtables are `const`, initialized at compile time.
- **OM-4** *(retired in v2: merged into OM-3)*
- **OM-5** *(since v1)* Deleting a derived object through its base finalizes the derived part, then
  the base, once each.
- **OM-6** *(since v2)* A `SizedBottle` always holds `0 ≤ volume ≤ maxVolume`: `setVolume` clamps
  into that range; `setMaxVolume` stores a negative as 0 and lowers the volume if it now exceeds the
  max. A new one has volume 0 and max 1. NaN is outside the contract.

### Runtime types

- **TY-1** *(since v1)* Every object reports its dynamic class name and parent chain, with no storage
  beyond its vtable pointer.
- **TY-2** *(since v1)* `Class_cast(p)` returns `p` if `p` is-a `Class`, else null — including for
  null `p`.
- **TY-3** *(since v1)* A host can allocate `Type.size` bytes, `init` them, use the object, and
  `fini` it, with C never allocating.
- **TY-4** *(since v2)* A host can read a `Type`'s name, parent and size through functions, without
  knowing the struct's layout.

### C++ wrapper

- **CXX-1** *(since v1)* `sizeof` and `alignof` of `cxx::X` equal those of `::X` at every level,
  checked at compile time.
- **CXX-2** *(since v1)* The wrappers contain no C++ vtable pointer.
- **CXX-3** *(since v1)* Through `cxx::Bottle&`, a `cxx::SizedBottle` runs its own override.
- **CXX-4** *(since v1)* The wrapper compiles as C++11.

### Bindings

- **BND-1** *(since v1)* In Python and JavaScript a derived class declares only what it adds;
  inherited methods reach the C override. No per-class C code exists for either.
- **BND-2** *(since v1)* Both bindings expose the checked downcast (TY-2) and host-owned memory
  (TY-3).
- **BND-3** *(since v1)* The JS binding runs unchanged over an Emscripten module and over a raw
  `WebAssembly.Instance`.
- **BND-4** *(since v6)* The raw wasm build links no libc.

### Tracing

- **TR-1** *(since v1)* Without `-DDEBUG` nothing is traced; a stale trace format string fails the
  build either way.
- **TR-2** *(since v1)* A `-DDEBUG` wasm build traces to the JS console.

### Inverted models

- **INV-1** *(since v1)* `cpp1/` and `cpp2/` export the C ABI of `src/*.c` (`cpp1/` less the
  `*_vftbl` symbols), and the unmodified C demo, C suite and Python suite pass against each.
- **INV-2** *(since v1)* `cpp2/oopcc.py` refuses input outside the flattenable subset.
- **INV-3** *(since v4)* In this repository's examples, the same behavioral checks run against the
  C++ model and against the C generated from it, for OM-1, OM-5 and OM-6 at least, and a change in
  either side's behavior alone fails the build.
- **INV-4** *(since v4)* `make test-inverted` runs in CI, through pixi, on the native Linux and macOS
  targets, and passes under both GCC and clang.

### Adoption

- **ADP-1** *(since v4)* One document, linked from the README and written for adopters, lists every
  footgun, each with what goes wrong and how to avoid it: at least the two manual prefix invariants,
  the `Bottle_fini` recursion in a `fini` slot, no `delete` through a `cxx::Bottle*`, the C ABI
  trusting its callers, and a C++ exception reaching an `extern "C"` entry point.

### Build

- **BLD-1** *(since v2)* No warnings under `-Wall -Wextra -Wpedantic` and no UBSan report on Linux
  x64, Linux arm64 and macOS arm64; the C and C++ suites also pass on 32-bit x86.
- **BLD-2** *(since v2)* wasm32 is a first-class target: the Emscripten and the raw build each pass
  the JS suite in CI.
- **BLD-3** *(since v4)* `make test` needs only a C and C++ compiler, make and Python 3.9 or later, and
  everything in Python here runs on 3.9. Anything further
  the inverted models need comes from `pixi.toml`, and no lock file is committed, so it resolves on
  each supported system.

## Inputs: absent, late, wrong, lying

Every call is synchronous, so nothing is late.

| Input | Absent | Wrong | Lying |
| ----- | ------ | ----- | ----- |
| `me` to a dispatcher | null: undefined | freed or uninitialized: undefined | not an object: undefined |
| pointer to `Class_cast` / `Type_of` | null → null | — | trusted: only the vtable's first word is read |
| `double` volume or max | — | negative → 0 (`SizedBottle`); NaN: out of contract | — |
| `malloc` in `Class_new` | null → returns null | — | — |
| a C++ model to `cpp2/oopcc.py` | missing file: fails | outside the subset: refused (INV-2) | — |

## Operating envelope

- **Targets:** 64-bit native, 32-bit native, wasm32. Not wasm64 yet (see Deferred).
- **Scale:** one two-class hierarchy. Not designing for deep hierarchies, multiple inheritance or
  interfaces.
- **Resources:** OM-2, CXX-1.
- **Recovery, availability, interruption:** not applicable; a library, not a service.

## Security, law, and ethics

No data about people. The C ABI trusts its callers completely (the *lying* column) — acceptable for
a pattern shown to be understood, and the first thing an adopter must fix for production (ADP-1).

## Assumptions (K4s), read back to the client

- "The hand-written `src/*.c` stays the teaching artifact; it may be rewritten when that makes it
  shorter or clearer." — confirmed, 2026-10-26
- "wasm32 is a target, and portability that costs no readability is wanted." — confirmed,
  2026-10-26
- "When teaching and use as a library conflict, the pattern wins." — confirmed, 2026-11-09

## Scope decisions

- Single inheritance only — the case that maps 1:1 onto every host language.
- C++ polymorphism through references, never `delete` through a base pointer — a virtual destructor
  would add a second vtable pointer (CXX-2).
- NaN is not handled — the NaN-safe comparison costs a student a paragraph (OM-6).
- Testing both sides of an inverted route binds this repository's examples and is a guideline for an
  adopter (SHAPE-2, INV-3) — the examples are what an adopter trusts; their code is theirs.
- The handle route is documentation only — its objects would not satisfy OM-2 or `Type`, so it could
  not share the suites that keep the other routes honest (INV-1).
- The release job stays and is not promised as a product — it shows the build works; what is offered
  is the pattern.

## Non-goals

- Multiple inheritance, interfaces, generics.
- Generating `src/` from a schema; it is hand-written on purpose.
- A production object system: no validation beyond `Class_cast`, no thread-safety beyond `const`
  tables. An adopter adds these.

## Not yet answered

None.

## Deferred

- *Not yet:* `align` in `Type` — needed only for an over-aligned class. TY-4 means adding it no longer
  touches the JS binding.
- *Not yet:* wasm64 — the JS binding would need `BigInt` for pointers. Trigger: a host that wants
  more than 4 GB.
- *Not yet:* an error channel in this repository's examples — none has an operation that can fail, and
  ADP-1's document gives adopters the convention. Trigger: an example gains an operation that can
  fail.
- *Not yet:* the JS suite through the inverted models — it needs a wasm build of the C++, a new build
  step, and Python already shows the gateway works. Trigger: a host that can only reach the gateway
  through wasm.
