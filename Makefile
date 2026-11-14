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
        cpp-inv cpp-inv-gen gen test-cpp-inv test-cpp-inv-gen test-inverted

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

test-inverted : test-cpp-inv test-cpp-inv-gen
	@echo "Inverted models: PASS"
