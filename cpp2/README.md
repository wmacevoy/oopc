# cpp-2 — C++ is the source, libclang emits the C

Same inversion as `cpp1/`, opposite mechanism. Here the model is **ordinary
C++** — real `virtual`, a real virtual destructor, real overrides, and nothing
that knows this tool exists — and a generator reads it with libclang and writes
the C.

```bash
make cpp2 test-cpp2          # generate, build, verify
make gen                     # just regenerate cpp2/gen/
python3 cpp2/test_oopcc.py   # the generator's own suite
```

| | |
|---|---|
| `cpp2/model/*.hpp` | the input: plain C++, compiles and runs on its own |
| `cpp2/oopcc.py` | the generator |
| `cpp2/gen/*.{h,c}` | the output: the oopc pattern, in the shape of `src/Bottle.c` |
| `cpp2/src/cpp2main.cpp` | the model run as ordinary C++ |
| `cpp2/test_oopcc.py` | 16 assertions: 4 accept, 12 refuse |

`bin/cbottle2` is the **unmodified** `src/cmain.c` compiled against the
generated headers, and its output is byte-identical to `bin/cbottle`'s.
`python/oopc.py` passes unchanged against `bin/liboopc2.<dylib|so>`.

## What the generator writes so you don't

Everything `CLAUDE.md` warns about:

- `SizedBottleVftbl` restating every base slot, in order, with matching types;
- `struct SizedBottleStruct` restating every base field, in order;
- `SizedBottle *me = (SizedBottle *)base;` at the top of every override;
- inherited slots resolved to the base's function — `Bottle_vf_getVolume`
  appears in `SizedBottle_vftbl` because the generator put it there;
- the fini chain, as `Bottle_vf_fini((Bottle *)me)` and never `Bottle_fini` —
  the footgun becomes generator output, so it cannot be typed wrong;
- every slot typed on the **root** base type, so no function pointer is cast;
- the `TRACE` lines.

Read `cpp2/gen/SizedBottle.c` against `src/SizedBottle.c`. They are the same
file, and one of them is maintained by hand.

## Why cpp-1 and cpp-2 differ where they do

`cpp1/` cannot use the `virtual` keyword: it emits the C ABI from a header at
compile time, so it has to own the vtable layout. cpp-2 has no such constraint —
the C++ is never linked into the artifact, it is only *read*. So the model here
can be the idiomatic version, and the compiler's private vtable is simply
irrelevant: the generator reproduces the dispatch explicitly in C.

That is the trade in one sentence. **cpp-1 gives up `virtual` to keep a build
with no extra step; cpp-2 keeps `virtual` and pays a toolchain dependency.**

## The subset

The generator refuses what it cannot flatten rather than emitting C that quietly
means something else. Every rule below is exercised by `cpp2/test_oopcc.py`.

| refused | why |
|---|---|
| more than one base | oopc is single inheritance; a second base has no prefix to live in |
| virtual base | the base subobject would not sit at a fixed prefix offset |
| non-public base | a C upcast is a plain pointer cast and cannot be access-checked |
| a non-`virtual` method | no slot, so a host calling through the C ABI could not reach the right one |
| an overloaded method | C has one name per function |
| a non-`virtual` destructor | it becomes the fini slot; a non-virtual one could not run derived teardown |
| a constructor with arguments | a host allocating from `Type::size` has nothing to pass |
| a copy or move constructor | oopc has no notion of copying; add a `clone` slot instead |
| a class-typed or reference field, parameter or return | the C ABI carries scalars; anything else needs an ownership rule the generator would have to invent |
| a member template | only instantiations could cross, and C cannot name one |

`class` at file scope is a model class and must satisfy all of the above.
`struct` is left alone — an ordinary helper type, the same split C++ convention
already uses.

## Body translation

Method bodies are lifted verbatim from the source and four things are rewritten,
using clang cursors rather than text matching:

| C++ | generated C |
|---|---|
| `volume` / `this->volume` | `me->volume` |
| `this` | `me` |
| `Bottle::setVolume(v)` | `Bottle_vf_setVolume((Bottle *)me, v)` — non-virtual, chains |
| `setVolume(v)` | `SizedBottle_setVolume(me, v)` — virtual, via the dispatcher |

The qualified/unqualified distinction is exactly C++'s, and it lands on exactly
the right side of the C pattern's most dangerous edge: a qualified call becomes
the `_vf_` implementation, never the dispatcher.

Everything not rewritten must already be valid C. The generator does not check
that; the C compiler does, and `cpp2/test_oopcc.py` builds every accepted case
under `-Wall -Wextra -Wpedantic -Werror`.

`#include <...>` lines in the model are carried into the generated `.c`, so a
destructor that calls `printf` still compiles. `#include "..."` lines are not —
those name other model headers, which become their own generated pair.

## Requires

`pip install libclang`, or any system `libclang.so`/`.dylib` — the generator
prefers the toolchain's own copy, because it has to parse the same standard
library headers the compiler will, and a pip-installed libclang older than the
installed SDK cannot always do that. It also asks the real compiler for
`-isysroot` and `-print-resource-dir`, since `libclang.parse()` is not the clang
driver and applies neither.

If libclang is unavailable, `make cpp2` fails and nothing else in the repository
is affected.

## Generated files in version control

`cpp2/gen/` is a build product and `make clean` removes it. For a teaching
repository there is a decent argument for committing it anyway: the whole point
is to put the C++ and the C side by side and show they are the same object, and
that is easier when the C is readable without running anything.

## Known gaps

- One default constructor, one virtual destructor, scalar signatures. Widening
  the subset is mostly mechanical; widening it *safely* is the interesting part,
  and every widening needs a new refusal test alongside it.
- Exceptions are not handled at all. Any real use of this needs `noexcept` or a
  catch-and-translate at every entry point, plus an error channel the C ABI does
  not currently have. This is the largest single tax on the inversion.
- `Type` carries `size` but not `align`, so a host cannot honor an over-aligned
  class. One `size_t` per class would close it — but it changes the shared
  `Type` layout, which `js/oopc.mjs` reads by offset, so it belongs in a change
  to the base model rather than here.
