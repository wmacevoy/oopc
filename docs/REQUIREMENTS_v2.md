# Requirements — v2

What must be observably true, and what is not being built. A change is a new version, never an edit
to this one. See
[SE1/R1–R11](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/04-requirements.md).
Supersedes [v1](REQUIREMENTS_v1.md); IDs keep their meaning across versions, and a merged one is
retired rather than reused.

Client: Warren MacEvoy · Date agreed: 2026-10-26 · *Tell us what is wrong with it.*

**What changed from v1:** the client confirmed v1 read back, and answered: this is both a working
library and a teaching instrument; wasm32 is a target; a change that improves portability without
hurting readability is wanted. Hence RD-1, BLD-2 and TY-4 are new, OM-2 no longer names byte counts,
OM-6 settles the two open questions, and OM-4 is folded into OM-3.

## Glossary

Unchanged from [v1](REQUIREMENTS_v1.md#glossary).

## The pain

Object orientation is taught as language magic. Hand-rolled C versions exist but are usually unsound
(function-pointer casts, mutable lazily built tables) and stop at C. This shows the whole mechanism
in a few hundred lines that are correct under the sanitizers, usable as a library, and visible
natively in three other languages.

## People this touches

Students reading the code (primary); the instructor who presents it; anyone who copies the pattern
into real code and inherits its footguns.

## Requirements

### Readability

- **RD-1** *(since v2)* `src/` and `include/` contain no conditional on target, compiler or word
  size. Portability is bought with plain C, not with `#if`.

### Object model

- **OM-1** *(since v1)* A derived object passes to any function taking its base with a plain pointer
  conversion, and every dispatcher called that way runs the derived override.
- **OM-2** *(changed v2)* An object is its vtable pointer plus its fields: `sizeof` each class equals
  that of a plain struct with the same members, on every target.
- **OM-3** *(changed v2)* No function pointer is ever cast — a mistyped slot is a compile error — and
  vtables are `const`, initialized at compile time.
- **OM-4** *(retired in v2: merged into OM-3)*
- **OM-5** *(since v1)* Deleting a derived object through its base finalizes the derived part, then
  the base, once each.
- **OM-6** *(changed v2)* A `SizedBottle` always holds `0 ≤ volume ≤ maxVolume`: `setVolume` clamps
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
- **BND-4** *(since v1)* The raw wasm build links no libc and is under 4 KB *(invented limit;
  2,996 bytes measured 2026-10-26)*.

### Tracing

- **TR-1** *(since v1)* Without `-DDEBUG` nothing is traced; a stale trace format string fails the
  build either way.
- **TR-2** *(since v1)* A `-DDEBUG` wasm build traces to the JS console.

### Inverted models

- **INV-1** *(since v1)* `cpp1/` and `cpp2/` export the C ABI of `src/*.c` (`cpp1/` less the
  `*_vftbl` symbols), and the unmodified C demo, C suite and Python suite pass against each.
- **INV-2** *(since v1)* `cpp2/oopcc.py` refuses input outside the flattenable subset.

### Build

- **BLD-1** *(changed v2)* No warnings under `-Wall -Wextra -Wpedantic` and no UBSan report on Linux
  x64, Linux arm64 and macOS arm64; the C and C++ suites also pass on 32-bit x86.
- **BLD-2** *(since v2)* wasm32 is a first-class target: the Emscripten and the raw build each pass
  the JS suite in CI.

## Inputs: absent, late, wrong, lying

Every call is synchronous, so nothing is late.

| Input | Absent | Wrong | Lying |
| ----- | ------ | ----- | ----- |
| `me` to a dispatcher | null: undefined | freed or uninitialized: undefined | not an object: undefined |
| pointer to `Class_cast` / `Type_of` | null → null | — | trusted: only the vtable's first word is read |
| `double` volume or max | — | negative → 0 (`SizedBottle`); NaN: out of contract | — |
| `malloc` in `Class_new` | null → returns null | — | — |

## Operating envelope

- **Targets:** 64-bit native, 32-bit native, wasm32. Not wasm64 yet (see Deferred).
- **Scale:** one two-class hierarchy. Not designing for deep hierarchies, multiple inheritance or
  interfaces.
- **Resources:** OM-2, CXX-1, BND-4.
- **Recovery, availability, interruption:** not applicable; a library, not a service.

## Security, law, and ethics

No data about people. The C ABI trusts its callers completely (the *lying* column) — acceptable for
teaching, and the first thing to fix for anyone copying it into production.

## Assumptions (K4s), read back to the client

- "The hand-written `src/*.c` stays the teaching artifact; it may be rewritten when that makes it
  shorter or clearer." — confirmed, 2026-10-26
- "wasm32 is a target, and portability that costs no readability is wanted." — confirmed,
  2026-10-26

## Scope decisions

- Single inheritance only — the case that maps 1:1 onto every host language.
- C++ polymorphism through references, never `delete` through a base pointer — a virtual destructor
  would add a second vtable pointer (CXX-2).
- NaN is not handled — the NaN-safe comparison costs a student a paragraph (OM-6).

## Non-goals

- Multiple inheritance, interfaces, generics.
- Generating `src/` from a schema; it is hand-written on purpose.
- A production object system: no validation beyond `Class_cast`, no thread-safety beyond `const`
  tables.

## Not yet answered

- Is 4 KB the right ceiling for BND-4? — client

## Deferred

- *Not yet:* `align` in `Type` — needed only for an over-aligned class. TY-4 means adding it no longer
  touches the JS binding.
- *Not yet:* wasm64 — the JS binding would need `BigInt` for pointers. Trigger: a host that wants
  more than 4 GB.
- *Not yet:* exceptions across the inverted models' entry points — trigger: any non-teaching use.
