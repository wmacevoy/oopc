CFLAGS=-g -Iinclude -fPIC
CXXFLAGS=$(CFLAGS) -std=c++11

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

.PHONY: all clean test test-c test-cpp test-python test-js wasm wasm-raw wasm-debug emcc-build

all : bin/cbottle bin/cppbottle $(SHLIB)

clean : 
	/bin/rm -rf tmp/* bin/*

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
