# Queue

What is being worked, in what order, by whom, and what would make it done. The shared cursor
([SE1/W10](https://github.com/wmacevoy/software-engineering/blob/v1.1/guide/08-work.md#w10)).
Nothing enters without a *done* sentence that can be checked from outside. Only a human sets *done*.

| # | Item | Versioned requirement IDs (or *chore*) | Owner | Done when | Estimate | Status | Since |
| - | ---- | -------------------------------------- | ----- | --------- | -------- | ------ | ----- |
| 1 | Make the C and C++ suites assert behaviour, not just exit 0 | REQ2/OM-1, REQ2/OM-6, REQ2/TY-2, REQ2/CXX-3 | agent | with the clamp in `src/SizedBottle.c` removed, `make test-c` and `make test-cpp` each fail ([FINDINGS](FINDINGS.md)) | time box: half a day | review | 2026-10-26 |
| 2 | Name requirement IDs in the tests | chore | agent | every check in the four `test_oopc` suites names the `REQ2/…` it checks, except Python's context-manager check, which no requirement covers | range: 1–2 h | review | 2026-10-26 |
| 3 | Confirm the *(inferred)* requirements with the client | all of REQUIREMENTS_v1 | Warren | REQUIREMENTS_v2 agreed | time box: 30 min | review | 2026-10-26 |
| 4 | Add `cpp1/` and `cpp2/` to CI | REQ2/INV-1, REQ2/INV-2 | — | a PR that breaks `cpp1/` or `cpp2/` turns `.github/workflows/build-test.yml` red | range: 1–3 h | ready | 2026-10-26 |
| 5 | See each `static_assert` fire once | REQ2/CXX-1, REQ2/OM-3 | — | a note in FINDINGS that each size/align assert and the mistyped-slot error was seen red | time box: 1 h | ready | 2026-10-26 |
| 6 | Check the layering rule | chore | — | a check fails when a layer-1 file includes a layer-3 one (DESIGN_v1, Components) | time box: 1 h | ready | 2026-10-26 |
| 7 | Fail the raw wasm build over the size limit | REQ2/BND-4 | — | `make wasm-raw` fails if the module exceeds the limit (CI prints the size today) | range: 30–60 min | blocked: the 4 KB limit is invented; REQUIREMENTS_v3 "Not yet answered" | 2026-10-26 |
| 8 | `WASI_SYSROOT` defaults to a Homebrew path | chore | agent | `make wasm-raw` with a missing `WASI_SYSROOT` says what to set, instead of a compiler error | range: 15–30 min | review | 2026-10-26 |
| 9 | Exceptions across the inverted models' entry points — also needs an error channel the C ABI does not have | REQ2/INV-1 | — | every `extern "C"` entry in `cpp1/` and `cpp2/` is `noexcept` or translates, and a test throws through one | range: 1–2 days | blocked: deferred in REQUIREMENTS_v3 | 2026-10-26 |
| 10 | `align` in `Type` | REQ2/TY-3 | — | a host allocating from `Type` honours an over-aligned class | range: half a day | blocked: deferred in REQUIREMENTS_v3 | 2026-10-26 |
| 11 | Type accessors; JS reads `Type` without offsets | REQ2/TY-4 | agent | `js/oopc.mjs` contains no `Type` offset, and the JS suite checks name, parent and size | range: 1 h | review | 2026-10-26 |
| 12 | `setMaxVolume` lowers the volume, in `src/`, `cpp1/` and `cpp2/` | REQ2/OM-6 | agent | the C, C++, Python and JS suites each check it, and the C suite passes against both inverted models | range: 1 h | review | 2026-10-26 |
| 13 | 32-bit and raw-wasm CI jobs | REQ2/BLD-1, REQ2/BLD-2 | agent | the `i386` and `wasm-raw` jobs pass on GitHub Actions | range: 1 h | review: not yet run on Actions | 2026-10-26 |
