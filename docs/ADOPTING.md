# Adopting the pattern

For someone copying this pattern into their own code. It lists what the pattern lets you get wrong
(REQ4/ADP-1) and how to design an API that stays portable across hosts. What each piece does is in
the [README](../README.md); this page is what to watch for once the code is yours.

This repository offers a pattern, not a production object system: validation beyond `Class_cast` and
thread-safety beyond `const` tables are yours to add.

## Choosing a route

| Route | Your source | The compiler or generator keeps | You keep |
| ----- | ----------- | ------------------------------- | -------- |
| Hand-written C (`src/`) | C | function-pointer types | every footgun below |
| Wrapper over C (`include/*.hpp`) | C, then C++ | sizes, by `static_assert` | the C footguns, plus the wrapper's |
| Inverted, header-only (`cpp1/`) | C++17, no `virtual` | prefixes, casts, the fini chain | exceptions, trusting callers |
| Inverted, generated (`cpp2/`) | ordinary C++ | prefixes, casts, the fini chain | exceptions, trusting callers, **drift** |

`cpp2/` is the most legible route, because its source is ordinary C++ and the C reads like the
hand-written version. **Adopt it with caution:** a generator can make its two sides disagree
silently, and nothing about the output will look wrong.

## Footguns

### The two prefix invariants (hand-written C)

- A derived vtable struct repeats every base slot first, in the same order.
- A derived object struct repeats every base field first, in the same order.

*What goes wrong:* nothing checks either one. A reordered slot calls the wrong function; a reordered
field reads the wrong bytes.
*Avoid it:* change the base and every derived header in the same commit, or use an inverted route.

### Retyping a slot to the derived class

*What goes wrong:* a slot typed on the derived class forces a function-pointer cast, which is
undefined behavior (C11 6.5.2.2p9) and traps under CFI.
*Avoid it:* every slot takes the root base pointer at every depth, and each implementation downcasts
its argument inside.

### The `Bottle_fini` recursion in a `fini` slot

*What goes wrong:* a derived `fini` that calls the public `Bottle_fini(me)` dispatches through the
vtable, lands back in itself, and recurses until the stack runs out.
*Avoid it:* chain with the direct, non-virtual call: `Bottle_vf_fini(base)`.

### Finalizing in a derived C++ wrapper

*What goes wrong:* `~cxx::Bottle` already runs the whole chain through the C vtable, so finalizing in
a derived destructor runs it twice.
*Avoid it:* derived wrapper destructors are empty.

### `delete` through a `cxx::Bottle*`

*What goes wrong:* `~cxx::Bottle` is deliberately non-virtual, because a virtual one would add a
second vtable pointer. Deleting a derived wrapper through a base pointer is undefined behavior.
*Avoid it:* use stack values and references, or allocate through the C API (`Bottle_new` and
`Bottle_delete`), which dispatches correctly.

### The C ABI trusts its callers

*What goes wrong:* a null, freed, uninitialized or non-object `me` passed to a dispatcher is undefined
behavior. `Class_cast` and `Type_of` read the first word of whatever they are given.
*Avoid it:* for production, validate at the boundary, and check every downcast from a foreign host
with `Class_cast`. This is the first thing to fix.

### A C++ exception reaching an `extern "C"` entry point

*What goes wrong:* on the inverted routes, an exception escaping into C or a host language may
terminate the process or unwind through frames that cannot handle it. **Neither `cpp1/` nor `cpp2/`
guards against this**, and the C ABI has no error channel yet.
*Avoid it:* catch everything at each entry point and turn failure into a return value, or make sure
no code behind the gateway can throw.

### Drift between a generator's two sides (`cpp2/`)

*What goes wrong:* the model and the generated C stop meaning the same thing, and the build stays
green.
*Avoid it:* see guideline 12. Run the same checks against both sides, as `cpp2/src/checks.inc` does.
Here that is a requirement of the examples (REQ4/INV-3); for your code it is a guideline.

### Trusting a build's stale object

*What goes wrong:* make's one-second timestamps can keep an object built from a file you have just
broken and restored, so a check seems to pass or fail for the wrong reason.
*Avoid it:* `make clean` after breaking and restoring a file on purpose.

## Guidelines for a portable inverted API

These apply when C++ is the source and the C ABI is the gateway to other languages.

| # | Guideline | Why |
| - | --------- | --- |
| 1 | Only scalars and pointers to oopc objects cross the gateway: no by-value classes, references, templates or `std::` types. | C has no layout or ownership for anything else. |
| 2 | State ownership. A parameter is borrowed for the call; only `Class_new` returns what the caller must `Class_delete`. | Every host needs one rule. |
| 3 | Use types whose width holds on every target: `double`, and `int` where 32 bits is enough. Avoid `long` and `size_t`. A 64-bit integer needs `BigInt` in JavaScript. | `long` and pointers change size across wasm32, 32-bit and 64-bit. |
| 4 | Pass strings and buffers as pointer plus length, with the caller allocating. | Allocators do not cross languages. |
| 5 | Let no exception cross. Failure is a return value. | Unwinding through C or a host is not portable. |
| 6 | Give each class a default constructor only, and configure through methods. | A host allocates from `Type.size` with nothing to pass. |
| 7 | One name per function: no overloads and no default arguments. | C and FFI have one symbol per name. |
| 8 | Use single public inheritance and a virtual destructor, and make every crossing method virtual. | This is what the prefix layout can represent. |
| 9 | Let hosts call functions; never have them read data symbols or struct offsets. | ctypes, raw wasm and Emscripten differ on data symbols. |
| 10 | Change the ABI by appending: new slots and fields go last. Anything else is a new ABI. | Compiled hosts and derived classes depend on the prefix. |
| 11 | Do not pass callbacks across the gateway unless you design them per host, and promise no thread-safety you have not built. | Each host differs. |
| 12 | Run the same checks against both sides of the inversion, and run at least one non-C host through the gateway. | Drift is invisible otherwise. |

### What `cpp2/` holds you to today

The [generator](../cpp2/README.md#the-subset) refuses input outside its subset, which enforces 6 and 8
and refuses overloads (part of 7). Three things differ from the guidelines above:

- **Guideline 1 and 2, for parameters only.** It accepts `C *` and `const C *` for a model class, as a
  borrowed pointer, and refuses an object pointer as a field or a return. Handing an object over has
  no rule yet.
- **Looser than guideline 3.** It accepts `long` and `size_t`, so avoiding them is up to you. Its list
  of accepted types does not include `int32_t`.
- **Silent on 5 and 10.** Nothing checks for a throwing entry point or a reordered ABI.
