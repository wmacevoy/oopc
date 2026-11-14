# Queue

What is being worked, in what order, by whom, and what would make it done. The shared cursor
([SE1/W10](https://github.com/wmacevoy/software-engineering/blob/v1.3/guide/08-work.md#w10)).
Nothing enters without a *done* sentence that can be checked from outside. Only a human sets *done*.

| # | Item | Versioned requirement IDs (or *chore*) | Owner | Done when | Estimate | Status | Since |
| - | ---- | -------------------------------------- | ----- | --------- | -------- | ------ | ----- |
| 4 | Add `cpp1/` and `cpp2/` to CI | REQ1/INV-1, REQ1/INV-2, REQ4/INV-4, REQ4/BLD-3 | agent | a PR that breaks `cpp1/` or `cpp2/` turns `.github/workflows/build-test.yml` red, on GCC and on clang, through pixi, and the Python suites run at Python 3.9 as well as the newest (REQ4/BLD-3) | range: 1–3 h | review | 2026-11-09 |
| 15 | Assert the C++ model directly, with the same checks as its generated C | REQ4/INV-3 | agent | breaking the clamp in `cpp2/model/SizedBottle.hpp` alone fails the C++ model's check, and the demo no longer prints PASS on exit 0 alone | range: 1–3 h | review | 2026-11-09 |
| 16 | Write the adopter's list of footguns | REQ4/ADP-1 | agent | one document, linked from the README, covers each footgun REQ4/ADP-1 names | range: 1–2 h | review | 2026-11-09 |
| 17 | Let `cpp2/` accept borrowed pointers to oopc objects as parameters (ADOPTING guideline 1) | REQ1/INV-2, REQ1/INV-1 | agent | `cpp2/oopcc.py` flattens a method taking a `Bottle *` and an accepted case behaves the same as C and as C++, while by-value and reference parameters are still refused with a test each; the ownership rule is in `cpp2/README.md`. An example on `cpp2/model` must also exist in every other layer (REQ1/INV-1), or stay in the generator's test cases | range: half a day | review | 2026-11-09 |
| 18 | Refuse what the generator would mistranslate: default member initializers, initializer lists, pure virtual methods, default arguments, namespaces, sources with no model class | REQ1/INV-2 | agent | each form exits 2 with a test seen failing, and the generated C for `cpp2/model` is unchanged | range: 1 h | review | 2026-11-09 |
| 1 | Make the C and C++ suites assert behaviour, not just exit 0 | REQ2/OM-1, REQ2/OM-6, REQ2/TY-2, REQ2/CXX-3 | agent | with the clamp in `src/SizedBottle.c` removed, `make test-c` and `make test-cpp` each fail ([FINDINGS](FINDINGS.md)) | time box: half a day | review | 2026-10-26 |
| 2 | Name requirement IDs in the tests | chore | agent | every check in the four `test_oopc` suites names the `REQ2/…` it checks, except Python's context-manager check, which no requirement covers | range: 1–2 h | review | 2026-10-26 |
| 3 | Confirm the *(inferred)* requirements with the client | all of REQUIREMENTS_v1 | Warren | REQUIREMENTS_v2 agreed | time box: 30 min | review | 2026-10-26 |
| 5 | See each `static_assert` fire once | REQ2/CXX-1, REQ2/OM-3 | agent | a note in FINDINGS that each size/align assert and the mistyped-slot error was seen red | time box: 1 h | review | 2026-11-09 |
| 6 | Check the layering rule | chore | — | a check fails when a layer-1 file includes a layer-3 one (DESIGN_v1, Components) | time box: 1 h | ready | 2026-10-26 |
| 7 | Fail the raw wasm build over the size limit | REQ6/BND-4 | agent | nothing to build: REQ6/BND-4 has no size limit | — | review | 2026-11-09 |
| 8 | `WASI_SYSROOT` defaults to a Homebrew path | chore | agent | `make wasm-raw` with a missing `WASI_SYSROOT` says what to set, instead of a compiler error | range: 15–30 min | review | 2026-10-26 |
| 9 | Exceptions across the inverted models' entry points — also needs an error channel the C ABI does not have | REQ2/INV-1 | — | every `extern "C"` entry in `cpp1/` and `cpp2/` is `noexcept` or translates, and a test throws through one | range: 1–2 days | blocked: deferred in REQUIREMENTS_v6 | 2026-10-26 |
| 10 | `align` in `Type` | REQ2/TY-3 | — | a host allocating from `Type` honours an over-aligned class | range: half a day | blocked: deferred in REQUIREMENTS_v6 | 2026-10-26 |
| 11 | Type accessors; JS reads `Type` without offsets | REQ2/TY-4 | agent | `js/oopc.mjs` contains no `Type` offset, and the JS suite checks name, parent and size | range: 1 h | review | 2026-10-26 |
| 12 | `setMaxVolume` lowers the volume, in `src/`, `cpp1/` and `cpp2/` | REQ2/OM-6 | agent | the C, C++, Python and JS suites each check it, and the C suite passes against both inverted models | range: 1 h | review | 2026-10-26 |
| 13 | 32-bit and raw-wasm CI jobs | REQ2/BLD-1, REQ2/BLD-2 | agent | the `i386` and `wasm-raw` jobs pass on GitHub Actions | range: 1 h | review | 2026-10-26 |
| 14 | Generated C keeps one statement per line, so GCC accepts it | REQ1/INV-2, REQ4/INV-4 | agent | `pixi run make test-cpp2` passes under GCC, and the generated C for `cpp2/model` is unchanged | range: 30 min | review | 2026-11-09 |
