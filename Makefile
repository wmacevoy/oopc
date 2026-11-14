CFLAGS=-g -Iinclude -fPIC
# REQ2/OM-3: a mistyped vtable slot is only a warning in GCC 12 C; clang already
# errors.  Kept out of CFLAGS so a CFLAGS override keeps it, and off the C++
# lines, where g++ rejects it as C-only.
CCHECKS=-Werror=incompatible-pointer-types
# -Icpp is here, not in CFLAGS, so a CFLAGS override keeps it and the C++17
# builds below never see the cxx:: wrapper headers.
CXXFLAGS=$(CFLAGS) -std=c++11 -Icpp
# cpp-inv/ needs auto NTTPs and inline variables; the C++11 wrapper does not.
CXX17FLAGS=$(CFLAGS) -std=c++17

UNAME_S:=$(shell uname -s)
ifeq ($(UNAME_S),Darwin)
  SHLIB_EXT=dylib
  SHARED_FLAG=-dynamiclib
else
  SHLIB_EXT=so
  SHARED_FLAG=-shared
endif

SHLIB=bin/liboopc.$(SHLIB_EXT)
OBJS=tmp/Type.o tmp/Trace.o tmp/Bottle.o tmp/SizedBottle.o

PYTHON?=python3
NODE?=node

tmp/Type.o : src/Type.c include/Type.h
	$(CC) $(CFLAGS) $(CCHECKS) -c -o $@ $<

tmp/Trace.o : src/Trace.c include/Trace.h
	$(CC) $(CFLAGS) $(CCHECKS) -c -o $@ $<

tmp/Bottle.o : src/Bottle.c include/Bottle.h include/Type.h include/Trace.h
	$(CC) $(CFLAGS) $(CCHECKS) -c -o $@ $<

tmp/SizedBottle.o : src/SizedBottle.c include/SizedBottle.h include/Bottle.h include/Type.h include/Trace.h
	$(CC) $(CFLAGS) $(CCHECKS) -c -o $@ $<

tmp/cmain.o : src/cmain.c include/Bottle.h include/SizedBottle.h include/Type.h
	$(CC) $(CFLAGS) $(CCHECKS) -c -o $@ $<

tmp/test_oopc.o : src/test_oopc.c include/Bottle.h include/SizedBottle.h include/Type.h
	$(CC) $(CFLAGS) $(CCHECKS) -c -o $@ $<

tmp/test_oopc_cpp.o : cpp/test_oopc.cpp cpp/Bottle.hpp cpp/SizedBottle.hpp include/Bottle.h include/SizedBottle.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

tmp/cppmain.o : cpp/cppmain.cpp cpp/Bottle.hpp cpp/SizedBottle.hpp include/Bottle.h include/SizedBottle.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

bin/cbottle : tmp/cmain.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

bin/cppbottle : tmp/cppmain.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

bin/test_c : tmp/test_oopc.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

bin/test_cpp : tmp/test_oopc_cpp.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Shared library -- this is the entire Python binding's dependency.  There is no
# extension module and no C glue: python/oopc.py drives this with ctypes.
# CFLAGS matters on the link line too: a sanitizer build needs its runtime, and
# a macOS dylib (unlike a Linux .so) will not link with undefined symbols.
$(SHLIB) : $(OBJS)
	$(CC) $(CFLAGS) $(SHARED_FLAG) -o $@ $^

.PHONY: all clean test test-c test-cpp test-python test-js wasm wasm-raw wasm-debug emcc-build \
        cpp-inv cpp-inv-gen gen test-cpp-inv test-cpp-inv-gen test-inverted \
        rust test-rust rust-inv test-rust-inv rust-inv-wasm test-rust-inv-wasm \
        prove prove-src prove-cpp-inv-gen test-mix

all : bin/cbottle bin/cppbottle $(SHLIB)

clean : 
	/bin/rm -rf tmp/* bin/*
	/bin/rm -f cpp-inv-gen/gen/*.h cpp-inv-gen/gen/*.c

test : test-c test-cpp test-python
	@echo "All tests: PASS"

# The demos only show the mechanism; the test_* programs check it.
test-c : bin/cbottle bin/test_c
	./bin/cbottle > /dev/null
	./bin/test_c > tmp/test_c.out || { cat tmp/test_c.out; exit 1; }
	@echo "C test: PASS"

test-cpp : bin/cppbottle bin/test_cpp
	./bin/cppbottle > /dev/null
	./bin/test_cpp > tmp/test_cpp.out || { cat tmp/test_cpp.out; exit 1; }
	@echo "C++ test: PASS"

test-python : $(SHLIB)
	$(PYTHON) python/test_oopc.py && echo "Python test: PASS"

WASM_SRCS=src/Type.c src/Trace.c src/Bottle.c src/SizedBottle.c

# Raw wasm: clang only, no libc linked, no emscripten.  malloc/free become
# imports the host supplies -- see js/load.mjs.  Builds anywhere clang can
# target wasm32 and the wasi-libc headers are present.
WASM_CC?=clang
WASI_SYSROOT?=/opt/homebrew/opt/wasi-libc/share/wasi-sysroot

wasm-raw : bin/oopc.raw.wasm

bin/oopc.raw.wasm : $(WASM_SRCS) $(wildcard include/*.h)
	@test -d $(WASI_SYSROOT) || { echo "no wasi-libc sysroot at $(WASI_SYSROOT); set WASI_SYSROOT=<dir>"; exit 1; }
	$(WASM_CC) --target=wasm32-wasip1 --sysroot=$(WASI_SYSROOT) -nostdlib -O2 -Iinclude \
	  -Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined -o $@ $(WASM_SRCS)

# Emscripten build; CI installs emsdk (see .github/workflows/build-test.yml).
# WASM_CFLAGS lets CI build a -DDEBUG variant whose traces land in console.log.
WASM_CFLAGS?=
WASM_OUT?=bin/oopc.js

wasm : bin/oopc.js

wasm-debug :
	$(MAKE) WASM_CFLAGS=-DDEBUG WASM_OUT=bin/oopc-debug.js emcc-build

bin/oopc.js : $(WASM_SRCS) js/exports.json
	$(MAKE) emcc-build

emcc-build :
	emcc -Iinclude -O2 $(WASM_CFLAGS) \
	  -o $(WASM_OUT) $(WASM_SRCS) \
	  -s MODULARIZE=1 -s EXPORT_ES6=1 -s EXPORT_NAME=createOopc \
	  -s ALLOW_MEMORY_GROWTH=1 \
	  -s EXPORTED_FUNCTIONS=@js/exports.json \
	  -s EXPORTED_RUNTIME_METHODS='["getValue","UTF8ToString"]'

test-js :
	$(NODE) js/test_oopc.mjs && echo "JS test: PASS"

# ===========================================================================
# cpp-inv -- C++ is the source, the C ABI is emitted by templates in a header.
#
# Same symbols, same layout, same behaviour: bin/cbottle-cpp-inv is the UNMODIFIED
# src/cmain.c linked against the C++-authored implementation, and its output
# must be byte-identical to bin/cbottle's.  python/oopc.py runs against it
# unchanged too, via OOPC_LIB.
# ===========================================================================

CPPINV_HDRS=cpp-inv/include/oopc/Class.hpp cpp-inv/include/oopc/Bottle.hpp cpp-inv/include/oopc/SizedBottle.hpp
CPPINV_OBJS=tmp/cpp-inv_exports.o tmp/Type.o tmp/Trace.o
SHLIB_CPPINV=bin/liboopc-cpp-inv.$(SHLIB_EXT)

tmp/cpp-inv_exports.o : cpp-inv/src/exports.cpp $(CPPINV_HDRS) include/Bottle.h include/SizedBottle.h
	$(CXX) $(CXX17FLAGS) -Icpp-inv/include -c -o $@ $<

tmp/cpp-inv_main.o : cpp-inv/src/main.cpp $(CPPINV_HDRS)
	$(CXX) $(CXX17FLAGS) -Icpp-inv/include -c -o $@ $<

bin/cpp-inv-bottle : tmp/cpp-inv_main.o $(CPPINV_OBJS)
	$(CXX) $(CXX17FLAGS) -o $@ $^

bin/cbottle-cpp-inv : tmp/cmain.o $(CPPINV_OBJS)
	$(CXX) $(CXX17FLAGS) -o $@ $^

bin/test_c-cpp-inv : tmp/test_oopc.o $(CPPINV_OBJS)
	$(CXX) $(CXX17FLAGS) -o $@ $^

$(SHLIB_CPPINV) : $(CPPINV_OBJS)
	$(CXX) $(CXX17FLAGS) $(SHARED_FLAG) -o $@ $^

cpp-inv : bin/cpp-inv-bottle bin/cbottle-cpp-inv bin/test_c-cpp-inv $(SHLIB_CPPINV)

test-cpp-inv : bin/cbottle bin/cbottle-cpp-inv bin/cpp-inv-bottle bin/test_c-cpp-inv $(SHLIB_CPPINV)
	@./bin/test_c-cpp-inv > tmp/test_c-cpp-inv.out || { cat tmp/test_c-cpp-inv.out; exit 1; }
	@echo "cpp-inv: unmodified C suite: PASS"
	@./bin/cbottle > tmp/c.out && ./bin/cbottle-cpp-inv > tmp/c-cpp-inv.out
	@diff tmp/c.out tmp/c-cpp-inv.out && echo "cpp-inv: the C demo is byte-identical against the C++ build"
	@./bin/cpp-inv-bottle > /dev/null && echo "cpp-inv: native C++ demo: PASS"
	@OOPC_LIB=$(SHLIB_CPPINV) $(PYTHON) python/test_oopc.py > /dev/null \
	  && echo "cpp-inv: unmodified Python binding against liboopc-cpp-inv: PASS"

# ===========================================================================
# cpp-inv-gen -- C++ is the source, libclang emits the C.
#
# cpp-inv-gen/model/*.hpp is ordinary single-inheritance polymorphic C++ that knows
# nothing about oopc.  cpp-inv-gen/oopcc.py flattens it into cpp-inv-gen/gen/*.{h,c} in the
# shape of src/Bottle.c.  Same two proofs as cpp-inv.
# ===========================================================================

CPPINVGEN_MODEL=cpp-inv-gen/model/Bottle.hpp cpp-inv-gen/model/SizedBottle.hpp
CPPINVGEN_GEN=cpp-inv-gen/gen/Bottle.h cpp-inv-gen/gen/Bottle.c cpp-inv-gen/gen/SizedBottle.h cpp-inv-gen/gen/SizedBottle.c
CPPINVGEN_OBJS=tmp/cpp-inv-gen_Bottle.o tmp/cpp-inv-gen_SizedBottle.o tmp/Type.o tmp/Trace.o
SHLIB_CPPINVGEN=bin/liboopc-cpp-inv-gen.$(SHLIB_EXT)

# One run of the generator produces all four files.  Grouped targets (&:) need
# GNU Make 4.3; a stamp says the same thing to every make in use.
tmp/cpp-inv-gen.stamp : cpp-inv-gen/oopcc.py $(CPPINVGEN_MODEL)
	$(PYTHON) cpp-inv-gen/oopcc.py $(CPPINVGEN_MODEL) -o cpp-inv-gen/gen -I cpp-inv-gen/model
	@touch $@

$(CPPINVGEN_GEN) : tmp/cpp-inv-gen.stamp

gen : tmp/cpp-inv-gen.stamp

tmp/cpp-inv-gen_Bottle.o : cpp-inv-gen/gen/Bottle.c cpp-inv-gen/gen/Bottle.h
	$(CC) $(CFLAGS) $(CCHECKS) -Icpp-inv-gen/gen -c -o $@ $<

tmp/cpp-inv-gen_SizedBottle.o : cpp-inv-gen/gen/SizedBottle.c cpp-inv-gen/gen/SizedBottle.h cpp-inv-gen/gen/Bottle.h
	$(CC) $(CFLAGS) $(CCHECKS) -Icpp-inv-gen/gen -c -o $@ $<

# src/cmain.c, unmodified, over the GENERATED headers.
tmp/cpp-inv-gen_cmain.o : src/cmain.c $(CPPINVGEN_GEN)
	$(CC) $(CFLAGS) $(CCHECKS) -Icpp-inv-gen/gen -c -o $@ $<

bin/cbottle-cpp-inv-gen : tmp/cpp-inv-gen_cmain.o $(CPPINVGEN_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

tmp/cpp-inv-gen_test_oopc.o : src/test_oopc.c $(CPPINVGEN_GEN)
	$(CC) -Icpp-inv-gen/gen $(CFLAGS) $(CCHECKS) -c -o $@ $<

bin/test_c-cpp-inv-gen : tmp/cpp-inv-gen_test_oopc.o $(CPPINVGEN_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# The model needs nothing from include/ or cpp/: cpp/SizedBottle.hpp is the
# cxx:: wrapper, a different thing with the same basename, and CXX17FLAGS does
# not reach it.
bin/cpp-inv-gen-bottle : cpp-inv-gen/src/main.cpp $(CPPINVGEN_MODEL)
	$(CXX) $(CXX17FLAGS) -Icpp-inv-gen/model -o $@ $<

# REQ4/INV-3: cpp-inv-gen/src/checks.inc compiled against each side.  -Icpp-inv-gen/gen comes
# first so the generated headers win over include/*.h, which share their names.
bin/test-cpp-inv-gen-generated : cpp-inv-gen/src/test_generated.c cpp-inv-gen/src/checks.inc $(CPPINVGEN_GEN) $(CPPINVGEN_OBJS)
	$(CC) -Icpp-inv-gen/gen $(CFLAGS) $(CCHECKS) -o $@ $< $(CPPINVGEN_OBJS)

bin/test-cpp-inv-gen-model : cpp-inv-gen/src/test_model.cpp cpp-inv-gen/src/checks.inc $(CPPINVGEN_MODEL)
	$(CXX) $(CXX17FLAGS) -Icpp-inv-gen/model -o $@ $<

$(SHLIB_CPPINVGEN) : $(CPPINVGEN_OBJS)
	$(CC) $(CFLAGS) $(SHARED_FLAG) -o $@ $^

cpp-inv-gen : bin/cbottle-cpp-inv-gen bin/cpp-inv-gen-bottle bin/test_c-cpp-inv-gen bin/test-cpp-inv-gen-generated bin/test-cpp-inv-gen-model $(SHLIB_CPPINVGEN)

test-cpp-inv-gen : bin/cbottle bin/cbottle-cpp-inv-gen bin/cpp-inv-gen-bottle bin/test_c-cpp-inv-gen bin/test-cpp-inv-gen-generated bin/test-cpp-inv-gen-model $(SHLIB_CPPINVGEN)
	@./bin/test_c-cpp-inv-gen > tmp/test_c-cpp-inv-gen.out || { cat tmp/test_c-cpp-inv-gen.out; exit 1; }
	@echo "cpp-inv-gen: unmodified C suite: PASS"
	@$(PYTHON) cpp-inv-gen/test_oopcc.py > /dev/null && echo "cpp-inv-gen: generator subset tests: PASS"
	@./bin/cbottle > tmp/c.out && ./bin/cbottle-cpp-inv-gen > tmp/c-cpp-inv-gen.out
	@diff tmp/c.out tmp/c-cpp-inv-gen.out && echo "cpp-inv-gen: the C demo is byte-identical against the generated C"
	@./bin/test-cpp-inv-gen-generated > tmp/test-gen.out || { cat tmp/test-gen.out; exit 1; }
	@./bin/test-cpp-inv-gen-model > tmp/test-model.out || { cat tmp/test-model.out; exit 1; }
	@echo "cpp-inv-gen: same checks against the C++ model and the generated C: PASS"
	@./bin/cpp-inv-gen-bottle > /dev/null && echo "cpp-inv-gen: C++ model demo ran"
	@OOPC_LIB=$(SHLIB_CPPINVGEN) $(PYTHON) python/test_oopc.py > /dev/null \
	  && echo "cpp-inv-gen: unmodified Python binding against liboopc-cpp-inv-gen: PASS"

# ===========================================================================
# rust -- Rust over the C ABI, the way cpp/ is C++ over it.
#
# rust/oopc.rs is the whole binding.  Bare rustc: no cargo project, no crates.
# The suite links the same objects the C and C++ suites do.
# ===========================================================================

RUSTC?=rustc
# rustc's default edition is 2015; c"..." literals need 2021.
RUSTFLAGS=--edition 2021 -g

# The C objects reach rustc as a static library, not as bare link arguments:
# rustc puts a library before libc on the link line and a bare argument after
# it, and on Linux arm64 an object placed after libc cannot find the stack
# protector's guard (FINDINGS 2026-11-14).
tmp/liboopc.a : $(OBJS)
	rm -f $@ && $(AR) rcs $@ $^

bin/test_rust : rust/test_oopc.rs rust/oopc.rs tmp/liboopc.a
	$(RUSTC) $(RUSTFLAGS) -o $@ $< -L native=tmp -l static=oopc

# REQ8/RS-5: the same suite with SizedBottle_new bound to Bottle_new, the fault
# the constructor's self-test exists for.  It must refuse, and say why.
bin/test_rust_wrong_new : rust/test_oopc.rs rust/oopc.rs tmp/liboopc.a
	$(RUSTC) $(RUSTFLAGS) --cfg wrong_new --check-cfg 'cfg(wrong_new)' -o $@ $< -L native=tmp -l static=oopc

rust : bin/test_rust bin/test_rust_wrong_new

test-rust : bin/test_rust bin/test_rust_wrong_new
	@./bin/test_rust > tmp/test_rust.out || { cat tmp/test_rust.out; exit 1; }
	@if ./bin/test_rust_wrong_new > tmp/test_rust_wrong_new.out 2>&1; then \
	  echo "REQ8/RS-5: a constructor bound to the wrong C function was accepted"; exit 1; fi
	@grep -q 'self-test: Class_new built a "Bottle", not a "SizedBottle"' tmp/test_rust_wrong_new.out \
	  || { echo "REQ8/RS-5: the wrong constructor failed, but not at the self-test:"; cat tmp/test_rust_wrong_new.out; exit 1; }
	@echo "Rust test: PASS"

# ===========================================================================
# rust-inv -- Rust is the source, the C ABI is the artifact.
#
# One object file, no std, linked exactly like tmp/Bottle.o + tmp/SizedBottle.o.
# -O so the few core functions used are inlined: the object then needs nothing
# from Rust's libraries, only malloc, free and Type_cast.  panic=abort because
# nothing here can unwind into C.  Same proofs as cpp-inv, plus rust/'s own
# suite: Rust, through the C ABI, over Rust.
# ===========================================================================

RUSTINV_SRCS=rust-inv/oopc.rs rust-inv/bottle.rs rust-inv/sized_bottle.rs
RUSTINV_OBJS=tmp/rust-inv.o tmp/Type.o tmp/Trace.o
SHLIB_RUSTINV=bin/liboopc-rust-inv.$(SHLIB_EXT)

tmp/rust-inv.o : $(RUSTINV_SRCS)
	$(RUSTC) $(RUSTFLAGS) --crate-type=lib --emit=obj -O -C panic=abort -C relocation-model=pic -o $@ $<

bin/cbottle-rust-inv : tmp/cmain.o $(RUSTINV_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

bin/test_c-rust-inv : tmp/test_oopc.o $(RUSTINV_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

tmp/liboopc-rust-inv.a : $(RUSTINV_OBJS)
	rm -f $@ && $(AR) rcs $@ $^

bin/test_rust-rust-inv : rust/test_oopc.rs rust/oopc.rs tmp/liboopc-rust-inv.a
	$(RUSTC) $(RUSTFLAGS) -o $@ $< -L native=tmp -l static=oopc-rust-inv

$(SHLIB_RUSTINV) : $(RUSTINV_OBJS)
	$(CC) $(CFLAGS) $(SHARED_FLAG) -o $@ $^

rust-inv : bin/cbottle-rust-inv bin/test_c-rust-inv bin/test_rust-rust-inv $(SHLIB_RUSTINV)

test-rust-inv : bin/cbottle bin/cbottle-rust-inv bin/test_c-rust-inv bin/test_rust-rust-inv $(SHLIB_RUSTINV)
	@./bin/test_c-rust-inv > tmp/test_c-rust-inv.out || { cat tmp/test_c-rust-inv.out; exit 1; }
	@echo "rust-inv: unmodified C suite: PASS"
	@./bin/cbottle > tmp/c.out && ./bin/cbottle-rust-inv > tmp/c-rust-inv.out
	@diff tmp/c.out tmp/c-rust-inv.out && echo "rust-inv: the C demo is byte-identical against the Rust build"
	@./bin/test_rust-rust-inv > tmp/test_rust-rust-inv.out || { cat tmp/test_rust-rust-inv.out; exit 1; }
	@echo "rust-inv: unmodified Rust binding over the Rust build: PASS"
	@OOPC_LIB=$(SHLIB_RUSTINV) $(PYTHON) python/test_oopc.py > /dev/null \
	  && echo "rust-inv: unmodified Python binding against liboopc-rust-inv: PASS"

# rust-inv for wasm32: the raw wasm build above, with the two classes coming
# from Rust.  Type.c and Trace.c are still C, compiled by the same clang; no
# libc and no Rust library is linked, and malloc/free stay imports.  rustc
# needs core for wasm32-unknown-unknown, which the pixi `wasm` environment has.
# No -g, as in the raw build: debug sections would triple the module.
tmp/rust-inv.wasm.o : $(RUSTINV_SRCS)
	$(RUSTC) --edition 2021 --target wasm32-unknown-unknown --crate-type=lib --emit=obj -O -C panic=abort -o $@ $<

bin/oopc.rust-inv.wasm : tmp/rust-inv.wasm.o src/Type.c src/Trace.c $(wildcard include/*.h)
	@test -d $(WASI_SYSROOT) || { echo "no wasi-libc sysroot at $(WASI_SYSROOT); set WASI_SYSROOT=<dir>"; exit 1; }
	$(WASM_CC) --target=wasm32-wasip1 --sysroot=$(WASI_SYSROOT) -nostdlib -O2 -Iinclude \
	  -Wl,--no-entry -Wl,--export-all -Wl,--allow-undefined -o $@ tmp/rust-inv.wasm.o src/Type.c src/Trace.c

rust-inv-wasm : bin/oopc.rust-inv.wasm

test-rust-inv-wasm : bin/oopc.rust-inv.wasm
	@OOPC_WASM=bin/oopc.rust-inv.wasm $(NODE) js/test_oopc.mjs > tmp/test_js-rust-inv.out \
	  || { cat tmp/test_js-rust-inv.out; exit 1; }
	@echo "rust-inv: unmodified JS binding over the Rust wasm build: PASS"

# ===========================================================================
# prove -- a bounded proof, by a model checker, of what the suites test.
#
# proof/oopc_proof.c states the claims and the assumptions (K4s) they rest on.
# CBMC checks them for every class, every argument and every sequence of
# PROOF_STEPS calls, and checks each pointer use, each array bound and that
# nothing leaks, with malloc free to fail.  The same proof runs on the
# hand-written C and on the C generated by cpp-inv-gen.
# ===========================================================================

CBMC?=cbmc
PROOF_STEPS?=4
# The loop bound is one more than the steps; --unwinding-assertions makes it an
# error, not a silent cut, if any loop or recursion needs more.
CBMCFLAGS=-DSTEPS=$(PROOF_STEPS) --unwind $(shell expr $(PROOF_STEPS) + 1) --unwinding-assertions \
          --pointer-check --bounds-check --memory-leak-check --malloc-may-fail --malloc-fail-null

prove-src : proof/oopc_proof.c $(WASM_SRCS)
	@command -v $(CBMC) > /dev/null || { echo "no model checker '$(CBMC)'; install CBMC or set CBMC=<path>"; exit 1; }
	@$(CBMC) proof/oopc_proof.c $(WASM_SRCS) -Iinclude $(CBMCFLAGS) > tmp/prove-src.out 2>&1 \
	  || { grep -E "FAILURE|ERROR|error" tmp/prove-src.out; echo "see tmp/prove-src.out"; exit 1; }
	@grep -q "^VERIFICATION SUCCESSFUL" tmp/prove-src.out
	@echo "prove: src/: $$(grep -c ': SUCCESS$$' tmp/prove-src.out) properties, 0 failed, any $(PROOF_STEPS) calls: PASS"

prove-cpp-inv-gen : proof/oopc_proof.c $(CPPINVGEN_GEN) src/Type.c src/Trace.c
	@command -v $(CBMC) > /dev/null || { echo "no model checker '$(CBMC)'; install CBMC or set CBMC=<path>"; exit 1; }
	@$(CBMC) proof/oopc_proof.c cpp-inv-gen/gen/Bottle.c cpp-inv-gen/gen/SizedBottle.c src/Type.c src/Trace.c \
	  -Icpp-inv-gen/gen -Iinclude $(CBMCFLAGS) > tmp/prove-cpp-inv-gen.out 2>&1 \
	  || { grep -E "FAILURE|ERROR|error" tmp/prove-cpp-inv-gen.out; echo "see tmp/prove-cpp-inv-gen.out"; exit 1; }
	@grep -q "^VERIFICATION SUCCESSFUL" tmp/prove-cpp-inv-gen.out
	@echo "prove: cpp-inv-gen/gen/: $$(grep -c ': SUCCESS$$' tmp/prove-cpp-inv-gen.out) properties, 0 failed, any $(PROOF_STEPS) calls: PASS"

prove : prove-src prove-cpp-inv-gen
	@echo "Proof: PASS"

# ===========================================================================
# test-mix -- a class in one language, extended in another (REQ10/INV-6).
#
# A Bottle from C, from C++ (the C that cpp-inv-gen generates) or from Rust,
# under a SizedBottle from each of the three: nine programs, each the
# unmodified C suite, which also derives its own classes from the result.
# A test, not an example: the Rust SizedBottle that can sit over a foreign
# Bottle is rust-inv/test_sized_over_abi.rs, and `--cfg bottle_only` builds
# rust-inv with its own SizedBottle left out.
# ===========================================================================

RUSTOBJ=$(RUSTC) $(RUSTFLAGS) --crate-type=lib --emit=obj -O -C panic=abort -C relocation-model=pic

tmp/rust-inv_Bottle.o : $(RUSTINV_SRCS)
	$(RUSTOBJ) --cfg bottle_only --check-cfg 'cfg(bottle_only)' -o $@ $<

tmp/rust-inv_SizedBottle.o : rust-inv/test_sized_over_abi.rs
	$(RUSTOBJ) -o $@ $<

MIX_BOTTLES=C:tmp/Bottle.o C++:tmp/cpp-inv-gen_Bottle.o Rust:tmp/rust-inv_Bottle.o
MIX_SIZED=C:tmp/SizedBottle.o C++:tmp/cpp-inv-gen_SizedBottle.o Rust:tmp/rust-inv_SizedBottle.o

test-mix : tmp/test_oopc.o tmp/Type.o tmp/Trace.o \
           tmp/Bottle.o tmp/cpp-inv-gen_Bottle.o tmp/rust-inv_Bottle.o \
           tmp/SizedBottle.o tmp/cpp-inv-gen_SizedBottle.o tmp/rust-inv_SizedBottle.o
	@for b in $(MIX_BOTTLES); do for s in $(MIX_SIZED); do \
	  what="mix: Bottle in $${b%%:*}, SizedBottle in $${s%%:*}"; \
	  $(CC) $(CFLAGS) -o bin/test_c-mix tmp/test_oopc.o $${b#*:} $${s#*:} tmp/Type.o tmp/Trace.o \
	    || { echo "$$what: does not link"; exit 1; }; \
	  ./bin/test_c-mix > tmp/test_c-mix.out \
	    || { echo "$$what: FAILED"; cat tmp/test_c-mix.out; exit 1; }; \
	  echo "$$what: unmodified C suite: PASS"; \
	done; done

test-inverted : test-cpp-inv test-cpp-inv-gen test-rust-inv test-mix
	@echo "Inverted models: PASS"
