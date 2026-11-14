# Findings

What we learned the expensive way: surprises, dead ends, measurements. The only document reality
writes ([SE1/W10](https://github.com/wmacevoy/software-engineering/blob/v1.3/guide/08-work.md#w10)).
Blameless: say what would have found it, not who should have asked
([SE1/H12](https://github.com/wmacevoy/software-engineering/blob/v1.3/guide/09-people.md#h12)).

Entries before 2026-10-26 are rebuilt from commit messages; their dates are the commit dates.

## Verdicts

One line each, by the named judge of a *(judged)* requirement (SE1/W11).

- REQ4/MISSION-1 verdict: pass by Warren MacEvoy, 2026-11-09
- REQ4/SHAPE-1 verdict: pass by Warren MacEvoy, 2026-11-09
- REQ5/SHAPE-2 verdict: pass by Warren MacEvoy, 2026-11-09
- REQ7/SHAPE-2 verdict: pass by Warren MacEvoy, 2026-11-14
- REQ4/SHAPE-3 verdict: pass by Warren MacEvoy, 2026-11-09
- REQ4/SHAPE-4 verdict: pass by Warren MacEvoy, 2026-11-09
- REQ4/SHAPE-5 verdict: pass by Warren MacEvoy, 2026-11-09

## 2026-11-14 — a Rust struct the size of the C object is sliced by safe code

- **What happened:** before the Rust binding was written, the natural design was a `#[repr(C)]`
  `Bottle` holding the C struct's bytes, as `cxx::Bottle` does. In a trial with the view given 16
  bytes, `std::mem::swap` on two `&mut Bottle`, one of them a `SizedBottle`, exchanged their vtable
  pointers and volumes with no `unsafe` in the calling code: each object then reported the other's
  class, and the clamp no longer ran (5 checks failed). C++ has the same hazard as assignment
  slicing; in Rust it is reachable from safe code.
- **Kind:** K1 — predicted when the design was proposed, then shown.
- **Cost:** none; tried before the requirement was written.
- **Findable by:** a swap through base references in the suite.
- **Changed:** a Rust class is a view with no bytes and `Owned<T>` holds the pointer (REQ7/RS-1,
  REQ7/RS-3); the swap is checked in `rust/test_oopc.rs` and the size is a compile-time assert, each
  seen failing; ADOPTING lists the footgun.

## 2026-11-14 — the proof, cut by two contracts; SizedBottle is not a Bottle under the strong reading

- **What happened:** the client asked whether the proof demonstrates polymorphism. It demonstrated
  dispatch for two classes, read together. Stating a contract for `Bottle` showed why it had not
  been stated: the whole-program proof claims "a plain Bottle keeps what it is given" only when the
  object is not a `SizedBottle`, which clamps. The contract every class keeps is weaker. With it,
  seven proofs pass: both classes keep the caller's contract and a client holds from the contract
  alone (`src/`, and `cpp-inv-gen/`'s output); each `Bottle` meets the extender's contract with no
  derived class linked (`src/`, generated, and the contract's own stub); each `SizedBottle` is correct
  over the stub with no `Bottle` linked (`src/`, generated). As a cross-check, the whole-program proof
  also passes on the two mixed pairs the pieces imply. Seen refusing: the stand-in subclass without
  the "setting what it holds" clause (the client's claim fails); a `setVolume` that halves; a
  `Bottle_vf_setVolume` that stores `x + 1`; a `Bottle_setVolume` that does not dispatch; a
  `Bottle_init` with volume 1; and a `SizedBottle` without its clamp, over the stub.
- **Kind:** K3 for the unstated contract: two classes that pass every test, and no sentence anywhere
  saying what a caller may rely on.
- **Cost:** two hours.
- **Findable by:** trying to write the contract down.
- **Changed:** REQ10/PRF-5 and PRF-6; `proof/contract.h` and three more proof files; `make prove` runs
  them. The Rust rows of the mix are tested and not proved (deferred in REQUIREMENTS_v10).

## 2026-11-14 — nine pairings of Bottle and SizedBottle across C, C++ and Rust all pass

- **What happened:** the client asked that a class in one language be extendable in another, as a
  test. Six pairings needed no new source: a `Bottle` from `src/`, from `cpp-inv-gen/`'s output or
  from `rust-inv/` (built without its `SizedBottle`), under the `SizedBottle` of `src/` or of
  `cpp-inv-gen/`. The Rust `SizedBottle` row needed a test-only file, because `rust-inv/`'s own is
  compiled against its Rust base; `cpp-inv/` is welded the same way and takes no part. All nine pass
  the unmodified C suite. Seen failing: with the clamp removed from the test-only Rust `SizedBottle`,
  its row failed; with the Rust `Bottle` storing nothing, the Rust-Bottle rows failed.
- **Kind:** K1.
- **Cost:** about an hour. It was that cheap only because the suite had begun naming the protected
  layer the same day.
- **Findable by:** linking it.
- **Changed:** REQ10/INV-6; `make test-mix`, run by `test-inverted` and so by CI.

## 2026-11-14 — `cpp-inv/` now exports the `Class_vf_*` functions

- **What happened:** the client decided the open question below: `cpp-inv/` should export them. The
  C suite was changed first, to make its two test classes with const tables that name `Bottle`'s
  slots and to call each of the seven `Class_vf_*` functions; against `cpp-inv/` as it was, it failed
  to link with all seven undefined. Seven one-line exports in `cpp-inv/src/exports.cpp`, each the
  slot in the class's own table called directly, and it passes there and on every other route.
- **Kind:** K3, closed — see the entry below for how it was found.
- **Cost:** an hour.
- **Findable by:** a suite that names every symbol the headers declare.
- **Changed:** the C suite names the whole protected layer, so a route that omits one fails to link
  (REQ1/INV-1); `cpp-inv/README.md` and CLAUDE.md say so.

## 2026-11-14 — "no virtual destructor" read as if the pattern had none

- **What happened:** reading the requirements, the client found "a virtual destructor in the
  wrapper" under *Says no to* and thought his original had one. It did, and still does: `fini` is the
  first vtable slot in the 2016 commit and today (REQ1/OM-5). The C++ wrapper never had the keyword
  `virtual`. The wording, rebuilt from the code on 2026-10-26, named the C++ keyword and not the
  pattern. Checking it turned up three more things. REQ1/OM-5 had no check in any of the four main
  suites. Trials: `delete` through a `cxx::Bottle*` finalizes correctly and runs clean at C++11;
  at C++14 ASan reports 24 bytes allocated and 16 freed; `virtual ~Bottle()` fails both layout
  asserts; a C++20 destroying `operator delete` makes the delete clean with no bytes added. And
  `cpp-inv/` does not define the `Class_vf_*` functions, though its README said every symbol but the
  two tables was defined and REQ1/INV-1 says the same; no suite had named one.
- **Kind:** K3 for the wording and for `cpp-inv/`'s symbols; K2 for the missing check.
- **Cost:** the client's attention, once.
- **Findable by:** reading the requirements back to the client aloud
  ([SE1/H6](https://github.com/wmacevoy/software-engineering/blob/v1.7/guide/09-people.md#h6)); a
  C class in the suite that derives from `Bottle`.
- **Changed:** REQUIREMENTS_v9 defines "virtual destructor" and rewords REQ1/OM-5, *Says no to* and the
  scope decision; `src/test_oopc.c` watches the fini chain through a base pointer, seen failing with
  `Bottle_delete` made non-virtual, and passes on every route; ADOPTING carries the trial results and
  the C++20 fix; `cpp-inv/README.md` says what is not defined. Open: `cpp-inv/` against REQ1/INV-1
  ([QUEUE](QUEUE.md) #29).

## 2026-11-14 — a model checker proves the two classes, and finds NaN by itself

- **What happened:** the client asked whether the pattern could carry a proof layer, with plain
  finite numbers as its assumption and bounded reasoning. `proof/oopc_proof.c` with CBMC 6.11.0
  verified 308 properties on `src/` and 283 on the C generated by `cpp-inv-gen/`, in about four
  seconds each: every class, every finite argument, every sequence of four calls (eight also passes,
  in 18 seconds), with `malloc` free to fail. It then refused each of six changes made on purpose:
  the clamp removed; `setMaxVolume` not lowering the volume; `SizedBottle_cast` accepting anything;
  `Bottle_delete` not freeing (the leak check); a `fini` slot calling `Bottle_fini` (the unwinding
  assertion: the recursion footgun, caught by a tool for the first time here); and the finite
  assumption removed, where its counterexample was `x = NaN`. With infinities allowed and only NaN
  excluded it still passed, so NaN is the only number outside the contract.
- **Kind:** K1 for the question; K2 for one trap: written with `isfinite`, the assumption assumed
  nothing, because the checker has no body for `__builtin_isfinite`. It showed as seven failures, not
  as a false pass.
- **Cost:** half a day, as boxed.
- **Findable by:** running it; and for the trap, the checker's own "no body for callee" line.
- **Changed:** REQ9/PRF-1 to PRF-4; `make prove` and a `prove` CI job; the trap is in CLAUDE.md under
  *The proof*. Not proved: `cpp-inv/` and `rust-inv/`, deferred in REQUIREMENTS_v9.

## 2026-11-14 — a constructor bound to the wrong C function segfaulted, until it tested itself

- **What happened:** the client asked whether a canary could verify an object's shape in the Rust
  constructor. With `SizedBottle_new` deliberately bound to `Bottle_new` and no check, the suite
  died with a segmentation fault at the first call through a slot a `Bottle`'s vtable does not have.
  With the constructor comparing `Type_of` the new object against `SizedBottle_getType()`, the same
  build stops with `self-test: Class_new built a "Bottle", not a "SizedBottle"`. With the comparison
  disabled, `make test-rust` failed, saying the wrong constructor had not stopped at the self-test.
- **Kind:** K1.
- **Cost:** none.
- **Findable by:** a fault on request: the suite built with `--cfg wrong_new`.
- **Changed:** REQ8/RS-5; `make test-rust` builds and runs the faulty variant every time. A slot-count
  canary, which would also catch a reordered slot, is deferred in REQUIREMENTS_v8 because it changes
  `Type`'s layout.

## 2026-11-14 — `rust-inv/` built for wasm32 unchanged

- **What happened:** the client asked for a wasm build of the Rust. The `rust-inv/` sources compiled
  for `wasm32-unknown-unknown` with no change, and the module has the same three imports (`malloc`,
  `free`, `vprintf`) and the same 36 class and type exports as the C raw build: 2,947 bytes against
  3,066. The JS suite passed on it, and failed (REQ2/BND-1) with the clamp removed from
  `rust-inv/sized_bottle.rs`. One surprise: `js/load.mjs` fetches a relative path and reads an
  absolute one, so the suite resolves `OOPC_WASM` first.
- **Kind:** K1 — a question with a cheap answer.
- **Cost:** none.
- **Findable by:** building it.
- **Changed:** REQ8/BLD-5; a `rust-inv-wasm` CI job; `OOPC_WASM` in `js/test_oopc.mjs`.

## 2026-11-14 — the Rust suite did not link on Linux arm64

- **What happened:** the first CI run of `make test-rust` failed on `ubuntu-24.04-arm` only:
  `tmp/Trace.o: undefined reference to symbol '__stack_chk_guard'`. The C objects were passed to
  rustc as bare link arguments, which it places after libc on the link line. On that platform the
  stack protector's guard lives in the dynamic loader, which the linker takes from libc's script
  only if something before it needs it. Linux x64 and macOS linked.
- **Kind:** K2.
- **Cost:** one red CI run on PR #7.
- **Findable by:** CI on every platform shipped
  ([SE1/O3](https://github.com/wmacevoy/software-engineering/blob/v1.3/guide/07-operating.md#o3)) —
  which is how it was found. Not reproducible on the author's x64 machine.
- **Changed:** the C objects go to rustc as a static library (`tmp/liboopc.a`, `-l static=oopc`),
  which rustc places before libc; the Makefile says why.

## 2026-11-14 — each new Rust check was seen failing; three small surprises

- **What happened:** REQ7/RS-1's assert stopped the build when a view had a size. REQ7/RS-2's checks
  failed with the clamp removed from `src/SizedBottle.c`. With the clamp removed from
  `rust-inv/sized_bottle.rs`, the unmodified C suite failed against it (REQ7/INV-5). Three things
  surprised the author: rustc's `improper_ctypes` lint refuses a view made of `PhantomData` alone in
  an `extern` signature; Rust block comments nest, so `include/*.h` inside one never closes; and a
  `cdylib` would not export the C objects linked into it, which is why `rust-inv/` builds an object
  file.
- **Kind:** K2 — none was looked for.
- **Cost:** minutes each.
- **Findable by:** building it.
- **Changed:** the first two are in CLAUDE.md under *The Rust binding*; the third is a decision in
  DESIGN_v1.

## 2026-11-09 — every layout assert fires; a mistyped slot is an error only under some compilers

- **What happened:** with one change each in a scratch copy, all 13 asserts failed with their own
  message: the size, alignment and standard-layout asserts in `include/*.hpp`, and the
  byte-identical, alignment, standard-layout and vtable asserts in `cpp1/`. A mistyped vtable slot
  is an error under clang 23 and under CI's `-Werror`, but under plain GCC 12 it is a warning and the
  object builds, so REQ2/OM-3's "a mistyped slot is a compile error" does not hold for a default
  `make` on that compiler.
- **Kind:** K3 — the claim was checked on the compiler the author used.
- **Cost:** none; found by QUEUE #5.
- **Findable by:** seeing a guard refuse on each compiler that is supposed to enforce it (SE1/T3).
- **Changed:** the Makefile passes `-Werror=incompatible-pointer-types` (`CCHECKS`) on every C
  compile, outside `CFLAGS` so an override keeps it. Seen: the mistyped slot fails a plain `make`
  on GCC 12, and builds with one warning under `make CCHECKS=`.

## 2026-11-09 — the generator accepted C++ it then mistranslated

- **What happened:** run against a real adopter's headers and against that code's style, `cpp2/oopcc.py`
  dropped default member initializers and constructor initializer lists, leaving fields
  uninitialized with no warning; lost default arguments; emitted a body with no return for a pure
  virtual method; and, for `struct`s or classes in a namespace, wrote nothing and exited 0.
  REQ1/INV-2 and the README both said it refuses what it cannot flatten.
- **Kind:** K3 — the refusal tests covered the cases someone had thought of. Bottle's model never
  used these forms.
- **Cost:** none shipped; found by attempting a rewrite outside the repository.
- **Findable by:** running the generator on code not written for it.
- **Changed:** each is now refused, with a test seen failing (`cpp2/test_oopcc.py`). A call to a
  static member still generates a bad call that only the C compiler catches; not fixed.

## 2026-11-09 — the generator's body translation assumed every receiver was `this`

- **What happened:** adding pointer parameters (QUEUE #17), `other->x` would have become `me->x`, valid
  C using the wrong object. The check that refused pointers was also what kept this unreachable.
- **Kind:** K2 — a guard hid an assumption nobody had to state.
- **Cost:** found before shipping, by reading the translator before relaxing the check.
- **Findable by:** asking what a refusal protects before removing it.
- **Changed:** the receiver is translated, other forms are refused, and turning the translation off
  fails a test.

## 2026-11-09 — the inverted models had never run under GCC or in CI, and one check could not fail

- **What happened:** the first run on Linux failed in `cpp2/test_oopcc.py` under
  `-Wmisleading-indentation`, and libclang could not find GCC's builtin headers. Separately, `make`
  printed `model runs as ordinary C++: PASS` for a program with no assertions.
- **Kind:** K3 — a `PASS` with no way to say wrong (SE1/U7).
- **Cost:** two generator fixes, found on the first real run.
- **Findable by:** CI on every platform (SE1/O3), and asking what would make each `PASS` fail.
- **Changed:** the `inverted` CI job (REQ4/INV-4), and `cpp2/src/checks.inc` compiled against both
  sides (REQ4/INV-3).

## 2026-11-09 — a real adopter kept the naming and lifecycle and dropped the vtable

- **What happened:** a private C++ project adopted this pattern for its C API. It kept
  `Class_method(me, ...)` and `init`/`fini`, and replaced the vtable, prefix layout and `Type` with an
  opaque smart-pointer handle. Inheritance stays inside C++. Rewriting its classes in the `cpp2/`
  shape worked for numeric classes (4,398 comparisons with the original, 0 differences at equal
  optimization) and could not express the rest: held objects, strings, buffers, constructors with
  arguments. Its exception handling was applied per function and missed some; its copy of an early
  version of this pattern still taught mutable, lazily filled tables.
- **Kind:** K2 — what real use needs from the gateway was not visible from two classes.
- **Cost:** none; found by reading it.
- **Findable by:** asking an adopter, which is why the requirements now name them.
- **Changed:** REQ5/SHAPE-2 names the handle route; `docs/ADOPTING.md` carries its guidelines.

## 2026-10-26 — the C++ demo printed `sizeof` as `unsigned long`

- **What happened:** the first run of the new `i386` CI job failed to compile `src/cppmain.cpp`:
  `printf("%lu", sizeof(...))` assumes `size_t` is `unsigned long`, which is false on 32-bit x86.
  macOS and 64-bit Linux could never show it.
- **Kind:** K3 — "builds warning-free" was true only on 64-bit targets.
- **Cost:** one CI run.
- **Findable by:** building on a target where the assumption is false (REQ2/BLD-1) — which is how
  it was found.
- **Changed:** `%zu`, as `src/cmain.c` already used.

## 2026-10-26 — the JS suite never tested host-owned memory, though the docs said it did

- **What happened:** CLAUDE.md said "both test suites" exercise a host allocating `Type.size` bytes
  and calling `init`/`fini`. Only the Python suite did. The JS binding read `Type` at offsets that
  hold only on wasm32, which no test would have caught on another target either.
- **Kind:** K3 — a claim nobody had checked against the file.
- **Cost:** none; found while writing REQUIREMENTS_v2 (REQ2/BND-2, REQ2/TY-4).
- **Findable by:** naming the requirement in every check — a requirement no label names is untested.
- **Changed:** `js/test_oopc.mjs` tests it; `js/oopc.mjs` reads `Type` through accessors.

## 2026-10-26 — breaking a file and restoring it can leave make holding the break

- **What happened:** to see a test go red, a line was broken, the suites run, and the file restored —
  all within one second. make's timestamps did not see the restore as newer, so a stale object kept
  the break and `make test` failed on correct source.
- **Kind:** K2.
- **Cost:** a few minutes, and nearly a false finding.
- **Findable by:** `git diff` on the restored file next to the failing output.
- **Changed:** CLAUDE.md says to `make clean` after a break-and-restore.

## 2026-10-26 — `make test-c` and `make test-cpp` cannot fail on a wrong answer

- **What happened:** with `SizedBottle`'s upper clamp commented out, `make test-c` and
  `make test-cpp` both printed PASS. Only the Python suite failed
  (`SizedBottle.set_volume(100) -> volume: got 100.0, want 10.0`). The C and C++ demos always
  return 0; "checked by exit status" checked only that they did not crash.
- **Kind:** K3 — "All tests: PASS" read as covering C and C++ behaviour, and did not.
- **Cost:** none yet; found while adopting the guide.
- **Findable by:** breaking the code on purpose and watching each suite
  ([SE1/T3](https://github.com/wmacevoy/software-engineering/blob/v1.3/guide/06-testing.md#t3)).
- **Changed:** `src/test_oopc.c` and `src/test_oopc.cpp` assert values; each was seen to fail with
  the clamp removed, and again with the `setMaxVolume` re-clamp removed (QUEUE #1).

## 2026-08-28 — the dylib link dropped `CFLAGS`, so UBSan failed on macOS only

- **What happened:** objects compiled with `-fsanitize=undefined` were linked without its runtime.
  Linux `.so` links tolerate undefined symbols; a macOS dylib does not.
- **Kind:** K2.
- **Cost:** a red CI job on `macos-latest`.
- **Findable by:** CI on every platform shipped
  ([SE1/O3](https://github.com/wmacevoy/software-engineering/blob/v1.3/guide/07-operating.md#o3)) —
  which is how it was found.
- **Changed:** `68bdfcb`; the Python suite now also runs against the instrumented library.

## 2026-08-28 — the C++ wrapper held two C objects, and segfaulted

- **What happened:** `cxx::SizedBottle` embedded a second `::SizedBottle` beside its base's
  `::Bottle`. Base methods used the uninitialized one; the first virtual call dereferenced a garbage
  vtable pointer. Size was 40, not 24.
- **Kind:** K3 — the design read as correct.
- **Cost:** a rewrite of both wrapper headers.
- **Findable by:** `static_assert(sizeof(cxx::X) == sizeof(::X))`, now present at every level.
- **Changed:** `61862af`; the rule is in CLAUDE.md as "only `cxx::Bottle` embeds a C struct."

## 2026-08-28 — inheriting a slot cast a function pointer, and the tables were racy

- **What happened:** `SizedBottle` inherited `getVolume` by casting a base function pointer to a
  derived signature — undefined behaviour, traps under CFI. The cast also forced lazy runtime table
  construction, leaving writable tables and a data race on the "already built?" guard.
- **Kind:** K3.
- **Cost:** retyping every slot and implementation.
- **Findable by:** `-fsanitize=cfi-icall`; `-Wall -Wextra -Wpedantic`.
- **Changed:** `fb6fba8`; a mistyped slot is now a compile error.
