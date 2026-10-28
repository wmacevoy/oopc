# Design — v1

How the requirements in [REQUIREMENTS_v3](REQUIREMENTS_v3.md) are met, and what was rejected. The
mechanism itself — layout rules, the five wrapper rules, the fini footgun — is in
[`CLAUDE.md`](../CLAUDE.md#architecture), which stays the one place it is written
([SE1/D6](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/05-design.md#d6)). This
file records structure and *why*.

Rebuilt from the code and history on 2026-10-26
([SE1/W12](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/08-work.md#w12)).
Reasons cite the commit that stated them; reasons with no source are marked *(inferred)*.

## Components and the dependency rule

| Component | Layer | Kind | Contract (one line) | Holds state? |
| --------- | ----- | ---- | ------------------- | ------------ |
| `include/Type.h`, `src/Type.c` | 0 | C | name/parent/size per class; `Type_of`, `Type_isa`, `Type_cast`; field accessors for hosts | const only |
| `include/Trace.h`, `src/Trace.c` | 0 | C | `TRACE(...)`, `Trace_to(sink)` | the sink pointer |
| `Bottle.{h,c}` | 1 | C | root class | per object |
| `SizedBottle.{h,c}` | 2 | C | derived class | per object |
| `include/*.hpp` (`cxx::`) | 3 | C++11 | zero-overhead wrapper | none of its own |
| `python/oopc.py` | 3 | Python | ctypes binding | none of its own |
| `js/oopc.mjs`, `js/load.mjs` | 3 | JS | wasm binding; loader | none of its own |
| `src/cmain.c`, `src/cppmain.cpp` | 4 | demo | prints the mechanism | — |
| `cpp1/` | 1–2 alt | C++17 | emits the layer-1/2 ABI from headers | per object |
| `cpp2/` | 1–2 alt | Python + libclang | emits `cpp2/gen/*.{h,c}` from `cpp2/model/` | per object |

A component depends only on lower layers. Nothing checks this
([QUEUE](QUEUE.md) #6).

## State

| State | Owner (authoritative) | Copies | Max staleness | While wrong, the user sees |
| ----- | --------------------- | ------ | ------------- | -------------------------- |
| object fields (`volume`, `maxVolume`) | the C object | none — every wrapper and binding reads through the C functions | — | — |
| vtable, `Type` | `static const` in the class's `.c` | none | — | — |
| the struct layouts | `include/*.h` | `cxx::` reserved bytes; `cpp1/`, `cpp2/gen/`; Python's ctypes `Type` | until a rebuild | `static_assert` fails (C++); the C suite fails (inverted models) |
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
  ([SE1/D11](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/05-design.md#d11)).
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
  readable line, and the generator translates the `cpp2` model's version to the same line —
  source: client, 2026-10-26.
- **Every suite checks values and names its requirement** — REQ2/BLD-1 — `bin/cbottle` and
  `bin/cppbottle` once stood in for C and C++ tests and could not fail on a wrong answer
  ([FINDINGS](FINDINGS.md) 2026-10-26) — source: this adoption.
- **Two inverted models rather than one** — REQ2/INV-1 — `cpp1/` is the one to use (no toolchain),
  `cpp2/` the one to read (its output diffs against `src/`) — source: `README.md` *(not yet
  committed)*.
- **`cpp1/` uses no `virtual`** — REQ2/INV-1 — an authored vtable has a layout C can read; a
  compiler's does not — source: `cpp1/README.md` *(not yet committed)*.

## Deferred

- *Not yet:* a layering check (see Components) — trigger: a third class or a second contributor.
