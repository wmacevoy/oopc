# Adopting the pattern

For someone copying this pattern into their own code. It lists what the pattern lets you get wrong
(REQ7/ADP-1) and how to design an API that stays portable across hosts. What each piece does is in
the [README](../README.md); this page is what to watch for once the code is yours.

This repository offers a pattern, not a production object system: validation beyond `Class_cast` and
thread-safety beyond `const` tables are yours to add.

## Choosing a route

| Route | Your source | The compiler or generator keeps | You keep |
| ----- | ----------- | ------------------------------- | -------- |
| Hand-written C (`src/`) | C | function-pointer types | every footgun below |
| Wrapper over C (`cpp/*.hpp`) | C, then C++ | sizes, by `static_assert` | the C footguns, plus the wrapper's |
| Inverted, header-only (`cpp-inv/`) | C++17, no `virtual` | prefixes, casts, the fini chain | exceptions, trusting callers |
| Inverted, generated (`cpp-inv-gen/`) | ordinary C++ | prefixes, casts, the fini chain | exceptions, trusting callers, **drift** |
| Inverted, handle (not built here) | ordinary C++ | everything behind the gateway | exceptions, trusting callers, one thin function per method |
| Binding over C (`rust/`) | C, then Rust | nothing across the boundary; Rust's rules on its own side | the `extern` declarations, and keeping views zero-sized |
| Inverted, hand-written (`rust-inv/`) | Rust, `#![no_std]` | prefixes, inherited slots, the fini chain | one downcast per class, trusting callers |
| Inverted, attribute macro (not built here) | ordinary Rust | everything `rust-inv/` writes by hand | the macro, and two crates to build it |

`cpp-inv-gen/` is the most legible route, because its source is ordinary C++ and the C reads like the
hand-written version. **Adopt it with caution:** a generator can make its two sides disagree
silently, and nothing about the output will look wrong.

### The handle route

Choose it when your hosts need a gateway and not the inheritance, which is usual for real C++ holding
strings, containers, or other objects. The C type is opaque, and in C++ it is a smart pointer to your
class:

```cpp
#ifdef __cplusplus
using CThing = std::shared_ptr<Thing>;
#else
typedef struct CThing CThing;
#endif
```

Each C function is one line over the C++ method. `CThing_init(me, ...)` constructs in `me`, or
allocates when `me` is null; named variants such as `CThing_init_fromJson` take the place of
overloads and may take arguments. Virtual dispatch stays inside C++, so hosts see one C type for a
whole hierarchy. The cost is a smart pointer and its control block per object, and no inheritance in
the host. It does not show the mechanism, so it is not what this repository teaches.

**Keep a C++ exception out of it with one wrapper**, not a `try` in each function: in the code this
route came from, the per-function version missed some.

### The Rust attribute-macro route

`rust-inv/` is to Rust what `cpp-inv/` is to C++: the ABI is written out, and the compiler keeps the
prefixes. The automatic form, the counterpart of `cpp-inv-gen/`, would be a procedural macro: an
attribute such as `#[class(extends = Bottle)]` on a plain struct and its `impl`, which emits the
`#[repr(C)]` layout, the vtable, the slots and the `#[no_mangle]` dispatchers. It is not built here
because it needs a cargo project and a parser crate, and because its output is Rust a reader never
sees, where `cpp-inv-gen/` emits C you can read beside `src/`. A generator from Rust to C in the
shape of `cpp-inv-gen/` is not practical: Rust has no class hierarchy to read, and its method bodies
are not nearly C the way that C++ subset is.

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

### A Rust type that owns the object's bytes

*What goes wrong:* if the Rust `Bottle` is a struct the size of the C one, safe code holding two
`&mut Bottle` can `std::mem::swap` them. When one is really a `SizedBottle`, that exchanges two
vtable pointers between objects of different sizes: slicing, with no `unsafe` in sight. In a trial
the swapped objects reported each other's class, and the clamp stopped working.
*Avoid it:* make the Rust type a view with no bytes (`rust/oopc.rs`), and hold the object through a
pointer. A swap of two views then moves nothing. Assert `size_of::<View>() == 0` at compile time.

### A Rust `extern` declaration that disagrees with the header

*What goes wrong:* `rust/oopc.rs` restates each C signature by hand, as ctypes code does. Nothing
compares it with `include/*.h`, so a wrong argument type compiles, links, and is undefined behavior.
*Avoid it:* keep the block short and beside the header in review, or generate it with a binding
generator once the API is large enough to pay for the dependency.

### A base with tail padding (`rust-inv/`)

*What goes wrong:* `rust-inv/` gets its prefix by making the base the first field. C's hand-written
derived struct restates the fields flat. The two layouts agree for `Bottle`, but a base that ends in
padding (a pointer and then an `int`, on a 64-bit target) is laid out differently by the two: flat
fields reuse the padding, a nested base does not (16 bytes against 24 for that example, measured in
C; the Rust side of it has not been built here).
*Avoid it:* compare `Type.size` from the Rust build with `sizeof` in the C header, as the C suite's
`Type_size` check does when it runs against `rust-inv/`.

### The C ABI trusts its callers

*What goes wrong:* a null, freed, uninitialized or non-object `me` passed to a dispatcher is undefined
behavior. `Class_cast` and `Type_of` read the first word of whatever they are given.
*Avoid it:* for production, validate at the boundary, and check every downcast from a foreign host
with `Class_cast`. This is the first thing to fix.

### A C++ exception reaching an `extern "C"` entry point

*What goes wrong:* on the inverted routes, an exception escaping into C or a host language may
terminate the process or unwind through frames that cannot handle it. **Neither `cpp-inv/` nor `cpp-inv-gen/`
guards against this** (`rust-inv/` is built with `panic=abort`, so a Rust panic there ends the process
and never unwinds into C), and the examples here have no error channel.
*Avoid it:* catch at every entry point, in one shared wrapper, and report through a convention: an
`int` status with out-parameters, or null for a function returning an object. Or make sure no code
behind the gateway can throw.

### Drift between a generator's two sides (`cpp-inv-gen/`)

*What goes wrong:* the model and the generated C stop meaning the same thing, and the build stays
green.
*Avoid it:* see guideline 12. Run the same checks against both sides, as `cpp-inv-gen/src/checks.inc` does.
Here that is a requirement of the examples (REQ4/INV-3); for your code it is a guideline.

### A copy of this pattern going stale

*What goes wrong:* a copied page or file stays as it was. Early versions of this pattern filled
vtables lazily in mutable storage, which is unsound, and a copy still teaches that with nothing
marking it old.
*Avoid it:* record the oopc commit or release you copied from, next to the copy.

### Trusting a build's stale object

*What goes wrong:* make's one-second timestamps can keep an object built from a file you have just
broken and restored, so a check seems to pass or fail for the wrong reason.
*Avoid it:* `make clean` after breaking and restoring a file on purpose.

## Guidelines for a portable inverted API

These apply when C++ is the source and the C ABI is the gateway to other languages.

| # | Guideline | Why |
| - | --------- | --- |
| 1 | Only scalars, pointers to oopc objects or handles, and plain structs of doubles by pointer cross the gateway: no by-value classes, references, templates or `std::` types. | C has no layout or ownership for anything else. |
| 2 | State ownership, with one of two rules. Borrowed: a parameter lasts for the call, and only `Class_new` returns what the caller must `Class_delete`. Handles: every handle is its own reference, a setter shares, a getter returns a new handle to delete. | Every host needs one rule. |
| 3 | Use types whose width holds on every target: `double`, and `int` where 32 bits is enough. Avoid `long` and `size_t`. A 64-bit integer needs `BigInt` in JavaScript. | `long` and pointers change size across wasm32, 32-bit and 64-bit. |
| 4 | Pass strings and buffers as pointer plus length, with the caller allocating. If the library must allocate, export its own free function. | Allocators do not cross languages, and a host left to find `free` can find the wrong one. |
| 5 | Let no exception cross, using one wrapper. Failure is an `int` status, or null for an object. | Unwinding through C or a host is not portable, and a rule applied per function drifts. |
| 6 | For host-allocated objects, a default constructor only, configured through methods. A handle may use named initializers with arguments. | A host allocating from `Type.size` has nothing to pass. |
| 7 | One name per function: no overloads and no default arguments. | C and FFI have one symbol per name. |
| 8 | Use single public inheritance and a virtual destructor, and make every crossing method virtual. | This is what the prefix layout can represent. |
| 9 | Let hosts call functions; never have them read data symbols or struct offsets. | ctypes, raw wasm and Emscripten differ on data symbols. |
| 10 | Change the ABI by appending: new slots and fields go last. Anything else is a new ABI. | Compiled hosts and derived classes depend on the prefix. |
| 11 | Do not pass callbacks across the gateway unless you design them per host, and promise no thread-safety you have not built. | Each host differs. |
| 12 | Run the same checks against both sides of the inversion, and run at least one non-C host through the gateway. | Drift is invisible otherwise. |

### What `cpp-inv-gen/` holds you to today

The [generator](../cpp-inv-gen/README.md#the-subset) refuses input outside its subset, which enforces 6 and 8
and refuses overloads and default arguments (7). Three things differ from the guidelines above:

- **Guideline 1 and 2, for parameters only.** It accepts `C *` and `const C *` for a model class, as a
  borrowed pointer, and refuses an object pointer as a field or a return. Handing an object over has
  no rule yet.
- **Looser than guideline 3.** It accepts `long` and `size_t`, so avoiding them is up to you. Its list
  of accepted types does not include `int32_t`.
- **Stricter than guidelines 1 and 4.** It refuses plain structs, buffers and strings.
- **Silent on 5 and 10.** Nothing checks for a throwing entry point or a reordered ABI.
