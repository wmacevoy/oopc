/*
  cpp-2 from the C++ side: the model, run as ordinary C++.

  This program includes cpp2/model/SizedBottle.hpp and uses it exactly as
  written -- real `virtual`, real overrides, a real C++ vtable.  Nothing here
  knows the generator exists.

  The output is the same as bin/cbottle2, which runs the *generated C* against
  the unmodified src/cmain.c.  That is the cpp-2 claim in one line: the C++ and
  the C mean the same thing.

  What the C++ object costs here that the C one does not: nothing extra in
  bytes -- both are a vtable pointer plus the fields -- but the C++ vtable is
  the compiler's, so its layout is unspecified, its slots are not addressable
  from C, and the entry points are name-mangled.  The generator exists to turn
  that private arrangement into the public one in cpp2/gen.
*/

#include <cstdio>

#include "SizedBottle.hpp"

static void fill(Bottle &b, const char *name)
{
  std::printf("  %-12s volume ", name);
  b.setVolume(1.0);
  std::printf("%.1lf ", b.getVolume());
  b.setVolume(100.0);
  std::printf("-> %.1lf\n", b.getVolume());
}

int main()
{
  Bottle plain;
  SizedBottle sized;
  sized.setMaxVolume(10);

  std::printf("dispatch through Bottle&  (the compiler's vtable)\n");
  fill(plain, "Bottle");
  fill(sized, "SizedBottle");

  std::printf("\nsizes\n");
  std::printf("  sizeof(Bottle)      = %zu\n", sizeof(Bottle));
  std::printf("  sizeof(SizedBottle) = %zu\n", sizeof(SizedBottle));
  std::printf("\ncompare bin/cbottle2: the same behaviour, through the generated C.\n");
  return 0;
}
