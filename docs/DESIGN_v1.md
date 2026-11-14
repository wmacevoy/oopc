# Design — v1

How the requirements in [REQUIREMENTS_v10](REQUIREMENTS_v10.md) are met, and what was rejected. The
mechanism itself — layout rules, the five wrapper rules, the fini footgun — is in
[`CLAUDE.md`](../CLAUDE.md#architecture), which stays the one place it is written
([SE1/D6](https://github.com/wmacevoy/software-engineering/blob/v1.7/guide/05-design.md#d6)). This
file records structure and *why*.

Rebuilt from the code and history on 2026-10-26
([SE1/W12](https://github.com/wmacevoy/software-engineering/blob/v1.7/guide/08-work.md#w12)).
Reasons cite the commit that stated them; reasons with no source are marked *(inferred)*.

## Components and the dependency rule

| Component | Layer | Kind | Contract (one line) | Holds state? |
| --------- | ----- | ---- | ------------------- | ------------ |
| `include/Type.h`, `src/Type.c` | 0 | C | name/parent/size per class; `Type_of`, `Type_isa`, `Type_cast`; field accessors for hosts | const only |
| `include/Trace.h`, `src/Trace.c` | 0 | C | `TRACE(...)`, `Trace_to(sink)` | the sink pointer |
| `Bottle.{h,c}` | 1 | C | root class | per object |
| `SizedBottle.{h,c}` | 2 | C | derived class | per object |
| `cpp/*.hpp` (`cxx::`) | 3 | C++11 | zero-overhead wrapper | none of its own |
| `python/oopc.py` | 3 | Python | ctypes binding | none of its own |
| `js/oopc.mjs`, `js/load.mjs` | 3 | JS | wasm binding; loader | none of its own |
| `rust/oopc.rs` | 3 | Rust | views and owners over the C ABI | one pointer per owner |
| `src/cmain.c`, `cpp/cppmain.cpp` | 4 | demo | prints the mechanism | — |
| `cpp-inv/` | 1–2 alt | C++17 | emits the layer-1/2 ABI from headers | per object |
| `cpp-inv-gen/` | 1–2 alt | Python + libclang | emits `cpp-inv-gen/gen/*.{h,c}` from `cpp-inv-gen/model/` | per object |
| `rust-inv/` | 1–2 alt | Rust, `#![no_std]` | emits the layer-1/2 ABI as one object file | per object |
| `proof/oopc_proof.c` | 4 | C for a model checker | claims about layers 0–2, and the assumptions they rest on | — |

A component depends only on lower layers. Nothing checks this
([QUEUE](QUEUE.md) #6).

## State

| State | Owner (authoritative) | Copies | Max staleness | While wrong, the user sees |
| ----- | --------------------- | ------ | ------------- | -------------------------- |
| object fields (`volume`, `maxVolume`) | the C object | none — every wrapper and binding reads through the C functions | — | — |
| vtable, `Type` | `static const` in the class's `.c` | none | — | — |
| the struct layouts | `include/*.h` | `cxx::` reserved bytes; `cpp-inv/`, `cpp-inv-gen/gen/`, `rust-inv/`; Python's ctypes `Type` | until a rebuild | `static_assert` fails (C++); the C suite fails (inverted models) |
| the function signatures | `include/*.h` | Python's ctypes declarations; the `extern` block in `rust/oopc.rs` | until edited by hand | nothing: a wrong one is undefined behavior (ADOPTING) |
| trace sink | `src/Trace.c` | none | — | — |

## Essential complexity

- Virtual dispatch with one pointer per object — REQ2/OM-1, REQ2/OM-2.
- Base-typed slots, so no function-pointer cast — REQ2/OM-3.
- A per-class `Type` reachable from the vtable — REQ2/TY-1, REQ2/TY-2.
- `init`/`fini` separate from allocation — REQ2/TY-3, REQ2/CXX-1.
- One wrapper or binding class per C class, mirroring it — REQ2/CXX-1, REQ2/BND-1.

Everything else (the tracing, the inverted models) serves teaching, not a requirement on the objects.

## One-way doors

- **The exported C ABI** — symbol names and `Type`'s layout. Every binding, both inverted models, and
  the released artifacts depend on it
  ([SE1/D11](https://github.com/wmacevoy/software-engineering/blob/v1.7/guide/05-design.md#d11)).
  Reviewed by: _nobody recorded_.

## Decisions

- **Slots are typed on the root base, implementations downcast** — REQ2/OM-3 — a derived
  table can name the base implementation verbatim, so no function-pointer cast (UB, traps under CFI),
  and the table can be `static const` — rejected: per-class slot types with a cast — source:
  `fb6fba8`.
- **Only `cxx::Bottle` embeds a C struct; derived wrappers reserve bytes** — REQ2/CXX-1 — one object,
  one address — rejected: embedding a C struct at every level (two disjoint objects, segfault, 40
  bytes) — source: `61862af`.
- **`~cxx::Bottle` is non-virtual** — REQ2/CXX-2 — a virtual destructor adds a second vtable pointer
  — rejected: virtual destructor; cost: no `delete` through a base pointer — source: `61862af`.
- **`Type` reached through the vtable, not a per-object tag** — REQ2/TY-1 — zero bytes per object —
  source: `e64839e`.
- **Every class exposes `Class_getType()` as well as `Class_type`** — REQ2/BND-1 — a function is the
  one thing ctypes, raw wasm and Emscripten all reach the same way — source: `e64839e`.
- **`TRACE` expands to `if (0) Trace(...)`, not nothing** — REQ2/TR-1 — arguments stay type-checked
  — source: `e64839e`.
- **The fini slot chains with `Bottle_vf_fini`, not `Bottle_fini`** — REQ2/OM-5 — the dispatcher
  would recurse forever — source: `fb6fba8`.
- **Hosts read `Type` through `Type_name`/`Type_parent`/`Type_size`** — REQ2/TY-4, REQ2/BLD-2 — the
  struct is 12 bytes on wasm32 and 24 on 64-bit, so offsets in JS were correct on one target only;
  three one-line functions make that C's business — rejected: computing offsets from pointer size in
  JS (still couples JS to the layout) — source: client, 2026-10-26. Python keeps its ctypes
  `Structure`, which is layout-correct on any platform.
- **`setMaxVolume` lowers the volume directly, not through the dispatcher** — REQ2/OM-6 — one
  readable line, and the generator translates the `cpp-inv-gen` model's version to the same line —
  source: client, 2026-10-26.
- **Every suite checks values and names its requirement** — REQ2/BLD-1 — `bin/cbottle` and
  `bin/cppbottle` once stood in for C and C++ tests and could not fail on a wrong answer
  ([FINDINGS](FINDINGS.md) 2026-10-26) — source: this adoption.
- **Two inverted C++ models rather than one** — REQ2/INV-1 — `cpp-inv/` is the one to use (no toolchain),
  `cpp-inv-gen/` the one to read (its output diffs against `src/`) — source: `README.md` *(not yet
  committed)*.
- **`cpp-inv/` uses no `virtual`** — REQ2/INV-1 — an authored vtable has a layout C can read; a
  compiler's does not — source: `cpp-inv/README.md` *(not yet committed)*.

- **C++ lives in `cpp/`, and directories are named language first** — REQ7/SHAPE-2 — `include/` and
  `src/` are plain C only, C++ sits beside `python/` and `js/` as one more language at the gateway,
  and a listing groups each language's routes (`cpp`, `cpp-inv`, `cpp-inv-gen`, `rust`, `rust-inv`)
  — rejected: `invcpp`, `autoinvcpp` (scatter in a listing) — source: client, 2026-11-14.
- **A Rust class is a zero-sized view; `Owned<T>` holds the pointer** — REQ7/RS-1, REQ7/RS-3 — a
  view with no bytes cannot be swapped or replaced by safe code — rejected: a `#[repr(C)]` struct the
  size of the C object, in place, as the C++ wrapper does (safe `mem::swap` slices it;
  [FINDINGS](FINDINGS.md) 2026-11-14); cost: one pointer and a C allocation per owner — source: a trial,
  2026-11-14; the client confirmed the cost the same day (REQUIREMENTS_v7, Assumptions).
- **`Owned<T>` drops through `Bottle_delete`** — REQ7/RS-2 — the C dispatcher finalizes the dynamic
  class, so an owner of the base may hold a derived object; the C++ wrapper cannot offer that without
  a second vtable pointer — source: `rust/oopc.rs`.
- **`rust-inv/` is hand-written, with no macro** — REQ7/INV-5, REQ4/SHAPE-5 — each slot is one plain
  line, and the downcast is written once per class — rejected: `macro_rules!` thunks (shorter, harder
  to read beside `src/`), a procedural macro (crates; output unseen) — source: this session
  *(inferred: the client asked for the route, not the mechanism)*.
- **`rust-inv/` builds to one object file** — REQ7/INV-5, REQ9/BLD-3 — it links like a C object on
  every platform with the Makefile's existing rules — rejected: a `staticlib` (needs a per-platform
  whole-archive flag for the shared library, and bundles `core`), a `cdylib` (does not export the C
  objects linked into it) — source: this session's trial. The same choice gives the wasm build
  (REQ8/BLD-5): the object is linked by the clang that builds `src/Type.c` for wasm, exactly as the
  raw wasm build links its C.

- **Extension across languages is by name, and is a test** — REQ10/INV-6 — nine links of the C suite
  need no new example; the Rust `SizedBottle` over a foreign base is test-only and transcribes
  `src/SizedBottle.c` — rejected: making `rust-inv/sized_bottle.rs` or `cpp-inv/` reach their base
  through the ABI (it gives up what those routes are for: the compiler keeping the prefix and the
  fini chain) — source: client, 2026-11-14 ("not an example, just a test"); the choice of what stands
  for C++ and Rust is the agent's *(inferred)*.
- **The proof is a harness in C for a bounded model checker** — REQ9/PRF-1, REQ9/PRF-3 — it reads
  like the C suite, needs no annotation in `src/`, runs in seconds, and gives a counterexample;
  its assumptions are three lines — rejected: contracts in `src/` for a deductive verifier (no bound,
  but the annotations would outweigh the code a student came to read), a proof assistant (a second
  model of the code to keep in step) — source: client, 2026-11-14 ("bounded reasoning", "plain finite
  numbers only in the K4s"); the choice of CBMC is the agent's *(inferred)*.
- **The proof is cut by two contracts, a caller's and an extender's** — REQ10/PRF-5, REQ10/PRF-6 —
  a client is proved once for every subclass, and a derived class once for every base; pairings are
  never proved, so the cost grows by addition and not by multiplication — rejected: one contract per
  class (the caller's must be weak enough for a clamping subclass, the extender's must be exact, and
  one text cannot be both) — source: client, 2026-11-14 ("if bottle is correct, I only need to
  show…"; "the only way to win"); the three clauses of the caller's contract are the agent's
  *(inferred)*.
- **The invariant also gets an inductive step** — REQ9/PRF-2 — any state that satisfies it, one call:
  that removes the bound for the one claim where history matters — source: this session.

## Deferred

- *Not yet:* a layering check (see Components) — trigger: a third class or a second contributor.
