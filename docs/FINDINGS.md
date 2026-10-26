# Findings

What we learned the expensive way: surprises, dead ends, measurements. The only document reality
writes ([SE1/W10](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/08-work.md#w10)).
Blameless: say what would have found it, not who should have asked
([SE1/H12](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/09-people.md#h12)).

Entries before 2026-10-26 are rebuilt from commit messages; their dates are the commit dates.

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
- **Cost:** none; found while writing REQUIREMENTS_v2 (BND-2, TY-4).
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
  ([SE1/T3](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/06-testing.md#t3)).
- **Changed:** `src/test_oopc.c` and `src/test_oopc.cpp` assert values; each was seen to fail with
  the clamp removed, and again with the `setMaxVolume` re-clamp removed (QUEUE #1).

## 2026-08-28 — the dylib link dropped `CFLAGS`, so UBSan failed on macOS only

- **What happened:** objects compiled with `-fsanitize=undefined` were linked without its runtime.
  Linux `.so` links tolerate undefined symbols; a macOS dylib does not.
- **Kind:** K2.
- **Cost:** a red CI job on `macos-latest`.
- **Findable by:** CI on every platform shipped
  ([SE1/O3](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/07-operating.md#o3)) —
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
