CFLAGS=-g -Iinclude
CXXFLAGS=$(CFLAGS) -std=c++11

tmp/Bottle.o : src/Bottle.c include/Bottle.h
	$(CC) $(CFLAGS) -c -o $@ $<

tmp/SizedBottle.o : src/SizedBottle.c include/SizedBottle.h include/Bottle.h
	$(CC) $(CFLAGS) -c -o $@ $<

tmp/cmain.o : src/cmain.c include/Bottle.h include/SizedBottle.h
	$(CC) $(CFLAGS) -c -o $@ $<

tmp/cppmain.o : src/cppmain.cpp include/Bottle.h include/Bottle.hpp include/SizedBottle.h include/SizedBottle.hpp
	$(CXX) $(CXXFLAGS) -c -o $@ $<

bin/cbottle : tmp/cmain.o tmp/Bottle.o tmp/SizedBottle.o
	$(CC) $(CFLAGS) -o $@ $^

bin/cppbottle : tmp/cppmain.o tmp/Bottle.o tmp/SizedBottle.o
	$(CXX) $(CXXFLAGS) -o $@ $^

clean : 
	/bin/rm -rf tmp/* bin/*

all : bin/cbottle bin/cppbottle

