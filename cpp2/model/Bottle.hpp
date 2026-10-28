/*
  cpp-2 model: ORDINARY C++.

  No macros, no CRTP, no authored vtable, nothing that knows oopc exists.  This
  is what you would write if you had never heard of the C pattern -- single
  inheritance, `virtual`, a virtual destructor, protected data.  It compiles and
  runs as plain C++ (see cpp2/src/cpp2main.cpp).

  cpp2/oopcc.py reads this file with libclang and emits cpp2/gen/Bottle.h and
  cpp2/gen/Bottle.c: the same three layers src/Bottle.c writes by hand, the same
  prefix-compatible struct, the same const vtable, the same Type.

  The subset the generator accepts is documented in cpp2/README.md and enforced
  by the generator, which refuses anything it cannot flatten rather than
  emitting C that quietly means something else.
*/

#pragma once

class Bottle {

public:
  Bottle()
  {
    volume = 0;
  }

  virtual ~Bottle()
  {
  }

  virtual double getVolume() const
  {
    return volume;
  }

  virtual void setVolume(double value)
  {
    volume = value;
  }

protected:
  double volume;
};
