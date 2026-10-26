CFLAGS=-g -Iinclude -fPIC
CXXFLAGS=$(CFLAGS) -std=c++11
# cpp1/ needs auto NTTPs and inline variables; the C++11 wrapper does not.
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
	$(CC) $(CFLAGS) -c -o $@ $<

tmp/Trace.o : src/Trace.c include/Trace.h
	$(CC) $(CFLAGS) -c -o $@ $<

tmp/Bottle.o : src/Bottle.c include/Bottle.h include/Type.h include/Trace.h
	$(CC) $(CFLAGS) -c -o $@ $<

tmp/SizedBottle.o : src/SizedBottle.c include/SizedBottle.h include/Bottle.h include/Type.h include/Trace.h
	$(CC) $(CFLAGS) -c -o $@ $<

tmp/cmain.o : src/cmain.c include/Bottle.h include/SizedBottle.h include/Type.h
	$(CC) $(CFLAGS) -c -o $@ $<

tmp/cppmain.o : src/cppmain.cpp include/Bottle.hpp include/SizedBottle.hpp include/Bottle.h include/SizedBottle.h
	$(CXX) $(CXXFLAGS) -c -o $@ $<

bin/cbottle : tmp/cmain.o $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

bin/cppbottle : tmp/cppmain.o $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Shared library -- this is the entire Python binding's dependency.  There is no
# extension module and no C glue: python/oopc.py drives this with ctypes.
# CFLAGS matters on the link line too: a sanitizer build needs its runtime, and
# a macOS dylib (unlike a Linux .so) will not link with undefined symbols.
$(SHLIB) : $(OBJS)
	$(CC) $(CFLAGS) $(SHARED_FLAG) -o $@ $^

.PHONY: all clean test test-c test-cpp test-python test-js wasm wasm-raw wasm-debug emcc-build \
        cpp1 cpp2 gen test-cpp1 test-cpp2 test-inverted

all : bin/cbottle bin/cppbottle $(SHLIB)

clean : 
	/bin/rm -rf tmp/* bin/*
	/bin/rm -f cpp2/gen/*.h cpp2/gen/*.c

test : test-c test-cpp test-python
	@echo "All tests: PASS"

test-c : bin/cbottle
	./bin/cbottle > /dev/null && echo "C test: PASS"

test-cpp : bin/cppbottle
	./bin/cppbottle > /dev/null && echo "C++ test: PASS"

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
# cpp-1 -- C++ is the source, the C ABI is emitted by templates in a header.
#
# Same symbols, same layout, same behaviour: bin/cbottle1 is the UNMODIFIED
# src/cmain.c linked against the C++-authored implementation, and its output
# must be byte-identical to bin/cbottle's.  python/oopc.py runs against it
# unchanged too, via OOPC_LIB.
# ===========================================================================

CPP1_HDRS=cpp1/include/oopc/Class.hpp cpp1/include/oopc/Bottle.hpp cpp1/include/oopc/SizedBottle.hpp
CPP1_OBJS=tmp/cpp1_exports.o tmp/Type.o tmp/Trace.o
SHLIB1=bin/liboopc1.$(SHLIB_EXT)

tmp/cpp1_exports.o : cpp1/src/exports.cpp $(CPP1_HDRS) include/Bottle.h include/SizedBottle.h
	$(CXX) $(CXX17FLAGS) -Icpp1/include -c -o $@ $<

tmp/cpp1main.o : cpp1/src/cpp1main.cpp $(CPP1_HDRS)
	$(CXX) $(CXX17FLAGS) -Icpp1/include -c -o $@ $<

bin/cpp1bottle : tmp/cpp1main.o $(CPP1_OBJS)
	$(CXX) $(CXX17FLAGS) -o $@ $^

bin/cbottle1 : tmp/cmain.o $(CPP1_OBJS)
	$(CXX) $(CXX17FLAGS) -o $@ $^

$(SHLIB1) : $(CPP1_OBJS)
	$(CXX) $(CXX17FLAGS) $(SHARED_FLAG) -o $@ $^

cpp1 : bin/cpp1bottle bin/cbottle1 $(SHLIB1)

test-cpp1 : bin/cbottle bin/cbottle1 bin/cpp1bottle $(SHLIB1)
	@./bin/cbottle > tmp/c.out && ./bin/cbottle1 > tmp/c1.out
	@diff tmp/c.out tmp/c1.out && echo "cpp1: the C demo is byte-identical against the C++ build"
	@./bin/cpp1bottle > /dev/null && echo "cpp1: native C++ demo: PASS"
	@OOPC_LIB=$(SHLIB1) $(PYTHON) python/test_oopc.py > /dev/null \
	  && echo "cpp1: unmodified Python binding against liboopc1: PASS"

# ===========================================================================
# cpp-2 -- C++ is the source, libclang emits the C.
#
# cpp2/model/*.hpp is ordinary single-inheritance polymorphic C++ that knows
# nothing about oopc.  cpp2/oopcc.py flattens it into cpp2/gen/*.{h,c} in the
# shape of src/Bottle.c.  Same two proofs as cpp-1.
# ===========================================================================

CPP2_MODEL=cpp2/model/Bottle.hpp cpp2/model/SizedBottle.hpp
CPP2_GEN=cpp2/gen/Bottle.h cpp2/gen/Bottle.c cpp2/gen/SizedBottle.h cpp2/gen/SizedBottle.c
CPP2_OBJS=tmp/cpp2_Bottle.o tmp/cpp2_SizedBottle.o tmp/Type.o tmp/Trace.o
SHLIB2=bin/liboopc2.$(SHLIB_EXT)

# One run of the generator produces all four files.  Grouped targets (&:) need
# GNU Make 4.3; a stamp says the same thing to every make in use.
tmp/cpp2.stamp : cpp2/oopcc.py $(CPP2_MODEL)
	$(PYTHON) cpp2/oopcc.py $(CPP2_MODEL) -o cpp2/gen -I cpp2/model
	@touch $@

$(CPP2_GEN) : tmp/cpp2.stamp

gen : tmp/cpp2.stamp

tmp/cpp2_Bottle.o : cpp2/gen/Bottle.c cpp2/gen/Bottle.h
	$(CC) $(CFLAGS) -Icpp2/gen -c -o $@ $<

tmp/cpp2_SizedBottle.o : cpp2/gen/SizedBottle.c cpp2/gen/SizedBottle.h cpp2/gen/Bottle.h
	$(CC) $(CFLAGS) -Icpp2/gen -c -o $@ $<

# src/cmain.c, unmodified, over the GENERATED headers.
tmp/cpp2_cmain.o : src/cmain.c $(CPP2_GEN)
	$(CC) $(CFLAGS) -Icpp2/gen -c -o $@ $<

bin/cbottle2 : tmp/cpp2_cmain.o $(CPP2_OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# The model is built WITHOUT -Iinclude: cpp2/src/cpp2main.cpp includes
# "SizedBottle.hpp", and include/SizedBottle.hpp is the cxx:: wrapper for the C
# library -- a different thing with the same basename.
bin/cpp2bottle : cpp2/src/cpp2main.cpp $(CPP2_MODEL)
	$(CXX) $(filter-out -Iinclude,$(CXX17FLAGS)) -Icpp2/model -o $@ $<

$(SHLIB2) : $(CPP2_OBJS)
	$(CC) $(CFLAGS) $(SHARED_FLAG) -o $@ $^

cpp2 : bin/cbottle2 bin/cpp2bottle $(SHLIB2)

test-cpp2 : bin/cbottle bin/cbottle2 bin/cpp2bottle $(SHLIB2)
	@$(PYTHON) cpp2/test_oopcc.py > /dev/null && echo "cpp2: generator subset tests: PASS"
	@./bin/cbottle > tmp/c.out && ./bin/cbottle2 > tmp/c2.out
	@diff tmp/c.out tmp/c2.out && echo "cpp2: the C demo is byte-identical against the generated C"
	@./bin/cpp2bottle > /dev/null && echo "cpp2: model runs as ordinary C++: PASS"
	@OOPC_LIB=$(SHLIB2) $(PYTHON) python/test_oopc.py > /dev/null \
	  && echo "cpp2: unmodified Python binding against liboopc2: PASS"

test-inverted : test-cpp1 test-cpp2
	@echo "Inverted models: PASS"
