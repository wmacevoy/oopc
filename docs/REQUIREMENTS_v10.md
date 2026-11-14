# Requirements — v10

What must be observably true, and what is not being built. A change in meaning is a new version; a
spelling or lint fix is not. See
[SE1/R1–R11](https://github.com/wmacevoy/software-engineering/blob/v1.7/guide/04-requirements.md).
Supersedes [v9](REQUIREMENTS_v9.md); IDs keep their meaning across versions, and a merged one is
retired rather than reused. Cite each requirement with the version that last changed it: `REQ10/INV-6`,
but still `REQ7/SHAPE-2` and `REQ2/OM-6`.

Client and proxy for adopters and students: Warren MacEvoy · Date agreed: 2026-11-14 · *Tell us what
is wrong with it.*

**What changed from v9:** the client asked that a class written in one of C, C++ and Rust be
extendable in any of the three, shown by a test and not by a new example. INV-6 is new, with one
scope decision saying what stands for C++ and for Rust in that test. He then asked whether the proof
shows polymorphism, and for the proof in pieces: "for real problems it would be the only way to
win". PRF-5 and PRF-6 are new, with two glossary terms and one scope decision. Nothing else changed.
Drafted by the agent from the client's direction; agreed by the client, 2026-11-14.

**What changed from v8 (in v9):** the client asked for a proof layer: bounded reasoning by a model checker,
with plain finite numbers as its assumption. PRF-1 to PRF-4 are new. BLD-3 changed to say where the
checker comes from. Two scope decisions, one deferred item and two glossary terms follow.

Reading it, the client found "no virtual destructor" where he remembered having one. The pattern
has one and always did: the `fini` slot (OM-5). What is refused is the C++ keyword `virtual` on the
wrapper's destructor. The glossary now says what the virtual destructor is, and OM-5, *Says no to*
and the scope decision are reworded to say so; none changed in meaning, so `REQ1/OM-5` stays current.
The proof is optional, and a scope decision says why it is here anyway. Drafted by the agent from
the client's direction; agreed by the client with that change, 2026-11-14.

**What changed from v7 (in v8):** the client asked for a wasm build of the Rust, and for a self-test of the
class in the Rust constructor. BLD-5 is new: `rust-inv/` builds for wasm32 and passes the JS suite.
RS-5 is new: a Rust constructor refuses an object of another class. BLD-3 changed to say where the
wasm build's compiler comes from, and the operating envelope, the inputs table and one deferred item
follow. Nothing else changed. Directed and agreed by the client, 2026-11-14.

**What changed from v6 (in v7):** the client asked for Rust in both directions, and for directories named
language first. SHAPE-2 now speaks of a language, shown for C++ and Rust, and no longer of C++ alone;
*Finished* and *Exceptions* follow it. RS-1 to RS-4, INV-5 and BLD-4 are new; BLD-3 and ADP-1 changed.
The C++ wrapper moved from `include/` and `src/` to `cpp/`, `cpp1/` became `cpp-inv/`, and `cpp2/`
became `cpp-inv-gen/`. INV-1 to INV-4 and the other requirements that name those paths use the new
names and mean what they did, so their citations stay current. The C ABI did not change. Drafted by
the agent from the client's direction. Directed and agreed by the client, 2026-11-14, with one
assumption answered no: he wants a wasm build of the Rust, which is work for the next version
([QUEUE](QUEUE.md) #23).

**What changed from v5 (in v6):** the client dropped BND-4's size limit as arbitrary, and made "patch releases
only" after finishing a guideline: a new major version is allowed and unlikely. Directed by the
client 2026-11-09, agreed 2026-11-13.

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
| principle direction | The C API wrapped as another language's API (`cpp/`, `rust/`). |
| inverted direction | Another language's API wrapped as the C API (`cpp-inv/`, `cpp-inv-gen/`, `rust-inv/`), which then serves as the gateway. |
| view | In the Rust binding, a type that stands for a C object and owns no bytes: `&Bottle` is the C `Bottle *`. |
| virtual destructor | The `fini` slot: the first method slot of every vtable. `Class_delete` and `Class_fini` call through it, so finalizing through a base pointer runs the dynamic class's finalizer, which chains to its base's. |
| caller's contract | What holds through a base pointer whatever the object's class: the promises a client may rely on, weak enough for every subclass to keep. |
| extender's contract | What a derived class may rely on from its base: `Class_init` and the non-virtual `Class_vf_` functions, exactly. |
| bounded proof | A model checker's check of every execution up to a stated number of calls, for every input its assumptions allow. It says nothing past the bound, except where a step is shown to preserve an invariant from any state. |
| assumption (K4) | Something a proof, or one side of the gateway, takes as true and does not check. |
| owner | In the Rust binding, the one pointer that holds an object C allocated and deletes it when dropped. |
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
- **SHAPE-2** *(since v7)* *(judged)* A language meets the gateway in two directions, shown here for
  C++ and for Rust. *Principle:* the C API wrapped as that language's API (`cpp/`, `rust/`), with its
  cost stated: no bytes in C++; in Rust, no bytes in a view and one pointer in an owner. *Inverted:*
  that language's API wrapped as a C API (`cpp-inv/`, `cpp-inv-gen/`, `rust-inv/`), which then serves
  as the gateway to other languages. Both directions present the same ABI, and every route shown here
  is a pattern to adopt. A further inverted route, the handle, is recommended where a host needs the
  gateway and not the inheritance; it is documented in ADP-1's document and not built here.
  `cpp-inv-gen/` is the more legible C++ route, since its source is ordinary C++, and is adopted with
  caution: a generator can make its two sides disagree silently, and that drift is K3. An adopter of
  an inverted route should test both sides, as a guideline; in this repository's examples it is a
  requirement (INV-3). Rust has no generated route here: its automatic form is a procedural macro,
  described in ADP-1's document and not built. Judge: the holder, at each requirements version.
- **SHAPE-3** *(since v4)* *(judged)* Inheritance is the same in every layer: a derived class declares
  only what it adds, so adding a class repeats the same steps. Judge: the holder, at each requirements
  version.
- **SHAPE-4** *(since v4)* *(judged)* Nothing is hidden: an object is its vtable pointer plus its
  fields, and any extra cost has to be stated and measured. Judge: the holder, at each requirements
  version.
- **SHAPE-5** *(since v4)* *(judged)* Plain beats clever: portability comes from plain C, and the
  domain stays trivial so nothing distracts from the mechanism. Judge: the holder, at each
  requirements version.
- **Finished:** two classes, both directions in C++ and in Rust, all targets in the operating envelope green; after
  that, patch releases, as a guideline. A new major version is allowed and unlikely.
- **Foundation:** `SE1`
- **Says no to:** multiple inheritance, interfaces and generics (they do not map 1:1 onto every host);
  generating `src/` (hand-written on purpose); the C++ keyword `virtual` on the wrapper's destructor (a
  second vtable pointer; the destructor is already virtual, through the `fini` slot); extension modules (per-class glue); per-target `#if` (SHAPE-5); a more interesting domain
  (it distracts).
- **Exceptions:** the `__GNUC__` guard in `Trace.h`, which only turns on compiler checks (RD-1).
  `cpp-inv/` is C++17, `cpp-inv-gen/` needs libclang and the Rust examples need `rustc`, because each
  has a language other than C as its source; the proof needs a model checker; SHAPE-5's plain C holds for `src/` and `include/`. The
  Rust binding does not hold an object in place as the C++ wrapper does: C allocates it and Rust keeps
  one pointer (RS-1, RS-3). The handle route stops inheritance at the gateway
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

- **RD-1** *(since v3)* `src/`, `include/` and `cpp/` contain no conditional on target, compiler or word
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
- **OM-5** *(since v1)* The destructor is virtual: deleting a derived object through its base
  finalizes the derived part, then the base, once each.
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

### Rust binding

- **RS-1** *(since v7)* A Rust view of a class owns no bytes and an owner is one pointer, checked at
  compile time.
- **RS-2** *(since v7)* Through `&mut Bottle`, a `SizedBottle` runs its own override, and so does one
  held by an owner of the base.
- **RS-3** *(since v7)* Safe Rust cannot slice an object: after two `&mut Bottle` are swapped, one of
  them a `SizedBottle`, each object has the class and the volume it had.
- **RS-4** *(since v7)* In Rust a derived class declares only what it adds; inherited methods reach
  the C override. The binding is built by `rustc` alone: no per-class C code, no crate and no build
  script exist for it.

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
- **RS-5** *(since v8)* A Rust constructor checks that the object C built is of the class it declares,
  at the cost of one pointer comparison, and panics with both class names if it is not.

### Inverted models

- **INV-1** *(since v1)* `cpp-inv/` and `cpp-inv-gen/` export the C ABI of `src/*.c` (`cpp-inv/` less the
  `*_vftbl` symbols), and the unmodified C demo, C suite and Python suite pass against each.
- **INV-2** *(since v1)* `cpp-inv-gen/oopcc.py` refuses input outside the flattenable subset.
- **INV-3** *(since v4)* In this repository's examples, the same behavioral checks run against the
  C++ model and against the C generated from it, for OM-1, OM-5 and OM-6 at least, and a change in
  either side's behavior alone fails the build.
- **INV-4** *(since v4)* `make test-inverted` runs in CI, through pixi, on the native Linux and macOS
  targets, and passes under both GCC and clang.
- **INV-6** *(since v10)* A `Bottle` written in C, C++ or Rust can be extended through the C ABI by a
  `SizedBottle` written in any of the three: for each of the nine pairings the unmodified C suite
  passes, including its checks that a further class can derive from the result.
- **INV-5** *(since v7)* `rust-inv/` exports the C ABI of `src/*.c`, and the unmodified C demo, C
  suite, Python suite and Rust suite pass against it.

### Proof

- **PRF-1** *(since v9)* For an object of either class, a model checker shows that every sequence of
  up to four calls with finite arguments gives OM-6's exact results, OM-1's base behavior, TY-1's
  class and TY-2's casts, with no invalid pointer use, no access out of bounds, and nothing leaked
  after `Class_delete`, whether or not `malloc` fails.
- **PRF-2** *(since v9)* OM-6's invariant is shown with no bound on history: from any `SizedBottle`
  state that satisfies it, any one call with a finite argument leaves it satisfied.
- **PRF-3** *(since v9)* The proof's assumptions are written in the proof as assumptions and listed at
  its top, and there are three: numbers are finite, an execution is at most the stated number of
  calls, and the checker's model of C is the compiler's.
- **PRF-4** *(since v9)* The same proof passes on `src/` and on the C generated by `cpp-inv-gen/`, and
  runs in CI.
- **PRF-5** *(since v10)* `Bottle` has a caller's contract, written in the proof: `getVolume` is finite
  and changes nothing, setting the volume an object holds changes nothing, and no call changes its
  class. Each class is shown to keep it from any state, and a client is proved from the contract
  alone, for an object known only to keep it.
- **PRF-6** *(since v10)* `Bottle` has an extender's contract, written in the proof. Each `Bottle` in C
  is shown to meet it with no derived class present, and each `SizedBottle` in C is proved over the
  contract with no `Bottle` present, so no pairing of the two needs a proof of its own.

### Adoption

- **ADP-1** *(since v7)* One document, linked from the README and written for adopters, lists every
  footgun, each with what goes wrong and how to avoid it: at least the two manual prefix invariants,
  the `Bottle_fini` recursion in a `fini` slot, no `delete` through a `cxx::Bottle*`, the C ABI
  trusting its callers, a C++ exception reaching an `extern "C"` entry point, a Rust type that owns
  the object's bytes, and a Rust `extern` declaration that disagrees with the C header.

### Build

- **BLD-1** *(since v2)* No warnings under `-Wall -Wextra -Wpedantic` and no UBSan report on Linux
  x64, Linux arm64 and macOS arm64; the C and C++ suites also pass on 32-bit x86.
- **BLD-2** *(since v2)* wasm32 is a first-class target: the Emscripten and the raw build each pass
  the JS suite in CI.
- **BLD-3** *(since v9)* `make test` needs only a C and C++ compiler, make and Python 3.9 or later, and
  everything in Python here runs on 3.9. Anything further the inverted models and the Rust examples
  need comes from `pixi.toml`, and no lock file is committed, so it resolves on each supported system.
  The exceptions are the wasm C compiler and sysroot, which BLD-2's raw build already needs, and the
  model checker for PRF-1, which is not packaged there and which CI installs from its release.
- **BLD-4** *(since v7)* `make test-rust` runs in CI, through pixi, on the native Linux and macOS
  targets, beside `make test-inverted` (INV-4).
- **BLD-5** *(since v8)* `rust-inv/` builds for wasm32 with no libc and no Rust library linked, and the
  unmodified JS suite passes against that module in CI.

## Inputs: absent, late, wrong, lying

Every call is synchronous, so nothing is late.

| Input | Absent | Wrong | Lying |
| ----- | ------ | ----- | ----- |
| `me` to a dispatcher | null: undefined | freed or uninitialized: undefined | not an object: undefined |
| pointer to `Class_cast` / `Type_of` | null → null | — | trusted: only the vtable's first word is read |
| `double` volume or max | — | negative → 0 (`SizedBottle`); NaN: out of contract | — |
| `malloc` in `Class_new` | null → returns null | — | — |
| a C++ model to `cpp-inv-gen/oopcc.py` | missing file: fails | outside the subset: refused (INV-2) | — |
| `Class_new` to the Rust binding | null: panics, no owner is made | an object of another class: deleted, then panics (RS-5) | — |
| an `extern` declaration in `rust/oopc.rs` | missing: fails to link | disagrees with the header: undefined, nothing checks it (ADP-1) | — |

## Operating envelope

- **Targets:** 64-bit native, 32-bit native, wasm32. Not wasm64 yet (see Deferred). `rust-inv/` runs on
  64-bit native and wasm32; `rust/`, the binding, on 64-bit native only.
- **Scale:** one two-class hierarchy. Not designing for deep hierarchies, multiple inheritance or
  interfaces.
- **Resources:** OM-2, CXX-1, RS-1.
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
- "The caller's contract as written, three clauses, is the one meant; a stronger one that a
  `SizedBottle` could not keep is not wanted." — agreed with this version, 2026-11-14
- "For 'C++' in the nine pairings, the C generated from the C++ model is what was meant, and a
  test-only Rust `SizedBottle` is acceptable for the Rust row." — agreed with this version, 2026-11-14
- "Four calls is bound enough for PRF-1, given that PRF-2 has no bound." — agreed with this version,
  2026-11-14
- "A proof that tests cannot replace is still worth a tool that pixi does not supply." — confirmed,
  2026-11-14: optional, and very important for code an agent writes
- "A wasm build of the Rust means `rust-inv/` as a raw wasm module under the JS suite. `rust/`, the
  binding, is not built for wasm: its suite would need a WASI runtime. Neither is built for 32-bit
  x86." — confirmed, 2026-11-14: wasm is for `rust-inv/` only, one way; no Rust binding in wasm
- "A Rust owner that costs one pointer and a C allocation is acceptable, where the C++ wrapper costs
  nothing." — confirmed, 2026-11-14
- "`rust-inv/` need not trace, and needs no Rust-side demo: the suites of the other languages are its
  proof." — confirmed, 2026-11-14
- "One Rust inverted route is enough; the procedural macro is documentation only." — confirmed,
  2026-11-14

## Scope decisions

- Single inheritance only — the case that maps 1:1 onto every host language.
- The wrapper's destructor is not `virtual` in C++, so C++ polymorphism goes through references, never
  `delete` through a base pointer — the keyword would add a second vtable pointer (CXX-2). Nothing is
  lost in finalizing: `~cxx::Bottle` calls through the `fini` slot. What `delete` through a base gets
  wrong is the size it frees ([FINDINGS](FINDINGS.md) 2026-11-14).
- In INV-6's test, C++ is the C that `cpp-inv-gen/` generates from the C++ model, and the Rust
  `SizedBottle` is a test-only file that reaches its base by the ABI's names — `cpp-inv/` and
  `rust-inv/` each weld their `SizedBottle` to their own base, which is what lets the compiler keep
  the prefix and the fini chain for them, so neither can sit over a foreign `Bottle`. Extending
  across languages is done the way C does it: by name, with C's footguns.
- "A `Bottle` keeps what it is given" is not in the caller's contract — a `SizedBottle` clamps, so a
  client that relied on it would be wrong for a subclass. The whole-program proof claims it of a plain
  `Bottle` only (PRF-1); the caller's contract is what every class keeps (PRF-5).
- The proof is optional — the pattern stands without it and `make test` does not need it (BLD-3). It
  is here because code is now written faster than it is read: a proof lets a person know that
  something works, or is safe, by reading its claims and assumptions and not the code.
- NaN is not handled — the NaN-safe comparison costs a student a paragraph (OM-6). The proof shows NaN
  is the only number that breaks OM-6: with infinities allowed and NaN excluded it still passes
  ([FINDINGS](FINDINGS.md) 2026-11-14).
- The proof assumes finite numbers, which is more than it needs — the client asked for plain finite
  numbers in the assumptions (PRF-3), and "finite" is the word a reader checks without a footnote.
- The proof covers C: `src/` and the output of `cpp-inv-gen/`. `cpp-inv/` and `rust-inv/` are not
  proved — each needs a different checker, and the suites they share with `src/` still run (INV-1,
  INV-5).
- Testing both sides of an inverted route binds this repository's examples and is a guideline for an
  adopter (SHAPE-2, INV-3) — the examples are what an adopter trusts; their code is theirs.
- The handle route is documentation only — its objects would not satisfy OM-2 or `Type`, so it could
  not share the suites that keep the other routes honest (INV-1).
- Rust views own no bytes, and C allocates the object — a Rust struct the size of the C one lets safe
  code swap a base prefix between two objects, which was seen ([FINDINGS](FINDINGS.md) 2026-11-14).
  The cost is one pointer per owner (RS-1, RS-3).
- The Rust procedural-macro route is documentation only — it needs crates, and its output is Rust a
  student never sees (SHAPE-4).
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

- *Not yet:* the contracts checked for the Rust `Bottle` and `SizedBottle` — INV-6's Rust rows are
  tested, not proved; the extender's contract is ready for a Rust checker to use. Trigger: as for the
  item below.
- *Not yet:* the proof for `rust-inv/` and `cpp-inv/` — a Rust model checker needs a cargo project, and
  the C checker's C++ support is partial. Trigger: an adopter who takes one of those routes into
  code where a failure is expensive.
- *Not yet:* a canary in the vtable or in `Type`, such as a slot count — it would catch a reordered or
  missing slot, which nothing checks in hand-written C, but it changes `Type`'s layout, the one-way
  door. RS-5 does what the ABI allows today. Trigger: a slot-order bug in an adopter's code, or a new
  major version.
- *Not yet:* `align` in `Type` — needed only for an over-aligned class. TY-4 means adding it no longer
  touches the JS binding.
- *Not yet:* wasm64 — the JS binding would need `BigInt` for pointers. Trigger: a host that wants
  more than 4 GB.
- *Not yet:* an error channel in this repository's examples — none has an operation that can fail, and
  ADP-1's document gives adopters the convention. Trigger: an example gains an operation that can
  fail.
- *Not yet:* the JS suite through the C++ inverted models — it needs a wasm build of the C++, a new
  build step, and Python already shows the gateway works. `rust-inv/` has one (BLD-5). Trigger: a
  host that can only reach the gateway through wasm.
