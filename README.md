# OOPC — Object Oriented Programming in C

A single-inheritance object system in plain C, small enough to read in one
sitting, wrapped so the *same* hierarchy appears natively in C++, Python and
JavaScript — with no per-class glue in any of them.

The C ABI is the interface. Every other language binds to it with code in that
language only.

```
   C          the object model: prefix layout, const vtables, runtime types
   C++        include/*.hpp    zero bytes and zero vtable pointers of overhead
   Python     python/oopc.py   ctypes; no extension module
   JavaScript js/oopc.mjs      wasm exports; no glue module
```

The demo hierarchy is deliberately trivial — a `Bottle` with a volume, and a
`SizedBottle` that clamps it — so that nothing distracts from the mechanism.

## The pattern in one screen

Inheritance is **prefix layout**: a derived struct repeats its base's fields
first, so upcasting is a plain pointer cast and costs nothing.

```c
struct BottleStruct      { const BottleVftbl      *vftbl; double volume; };
struct SizedBottleStruct { const SizedBottleVftbl *vftbl; double volume;
                           double maxVolume; };
```

Dispatch is **one vtable pointer per object, one vtable per class**:

```c
double Bottle_getVolume(const Bottle *me) { return me->vftbl->getVolume(me); }
```

`sizeof(Bottle)` is 16 and `sizeof(SizedBottle)` is 24 — byte-identical to what
C++ emits for the same hierarchy. You cannot do better and still have virtual
dispatch.

Three rules keep it sound, and the compiler enforces all three:

1. **Every vtable slot takes `Bottle *`** — the root base type — at every level.
   A derived class inherits a slot by naming the same function pointer, so no
   function-pointer cast ever arises. (Casting one and calling through it is
   undefined behavior in C11 6.5.2.2p9 and traps under CFI.) A mistyped slot is
   a compile error.
2. **Tables are `static const`** with compile-time initializers, so they live in
   read-only memory. Nothing is built lazily, so nothing races.
3. **`static_assert` pins the C++ wrapper's layout** to the C struct's, at every
   level.

## Runtime types

Three words per class, zero per object. Objects carry no type tag — they reach
their `Type` through the vtable they already point at.

```c
struct TypeStruct { const char *name; const Type *parent; size_t size; };
```

That buys a checked downcast, which is what makes the foreign bindings safe:

```c
SizedBottle *sb = SizedBottle_cast(something);   /* 0 if it isn't one */
```

`size` is there so a **host** can allocate an object without knowing C's
`sizeof`. Combined with the `Class_init`/`Class_fini` split — which operates on
storage the caller supplies — Python and JavaScript can own the memory outright
and the C layer never needs an allocator. Both test suites demonstrate it.

## Bindings

Each host language's own single inheritance lines up 1:1 with the C hierarchy.
The derived wrapper declares only the methods it *adds*; inherited methods call
the *base* C functions, which dispatch through the C vtable into the derived
override. The host supplies the class hierarchy, C supplies the virtual
dispatch, and neither needs to know how the other does it.

```python
sb = oopc.SizedBottle()
sb.set_max_volume(10)
sb.set_volume(100)      # inherited from Bottle; C clamps it
sb.get_volume()         # 10.0
oopc.SizedBottle.cast(oopc.Bottle())   # None
```

```js
const sb = new SizedBottle();   // SizedBottle declares no constructor
sb.setMaxVolume(10);
sb.setVolume(100);              // inherited from Bottle; C clamps it
sb.getVolume();                 // 10
SizedBottle.cast(new Bottle()); // null
```

The C++ wrapper is the strict case: it adds **no** memory and **no** second
vtable pointer, so `sizeof(cxx::SizedBottle) == sizeof(::SizedBottle)`.
Polymorphism goes through `cxx::Bottle &` and is resolved by the C vtable. See
the header comments in `include/SizedBottle.hpp` for the five rules a new
derived wrapper must follow.

## Tracing

`TRACE(...)` is off unless you build with `-DDEBUG`, and even when off it still
type-checks its arguments, so a stale format string is a compile error rather
than a surprise later.

Where traces *go* is a runtime choice — `Trace_to()` in `include/Trace.h`. The
default is stdout, and under Emscripten that is already `console.log`, so a
wasm `-DDEBUG` build traces into the JS console with no work. Note that these
writes go to file descriptor 1 and bypass `sys.stdout`, so under CPython they
interleave out of order with Python's own output, and under Jupyter they land in
the kernel's terminal rather than the cell.

## Build and test

```bash
make all            # bin/cbottle, bin/cppbottle, bin/liboopc.<dylib|so>
make test           # C, C++ and Python
make CFLAGS="-g -Iinclude -fPIC -DDEBUG" all    # with tracing
```

WebAssembly needs Emscripten, which CI installs — see
`.github/workflows/build-test.yml`:

```bash
make wasm && make test-js
```

There is also a toolchain-free wasm build: clang targeting `wasm32` with the
wasi-libc *headers* but no libc linked, which leaves `malloc`/`free` as imports
the host supplies. It produces a ~2.9 KB module that runs in node and the
browser with no shim.

```bash
make wasm-raw WASM_CC=/path/to/clang WASI_SYSROOT=/path/to/wasi-sysroot
node js/test_oopc.mjs
```
