/*
  cpp-1 from the C++ side: this is what the inversion buys the C++ programmer.

  Stack objects, RAII, polymorphism through a base reference, checked casts --
  and the objects are still exactly the C structs, with one pointer of overhead
  and no C++ vtable pointer anywhere.  Nothing here uses the `virtual` keyword,
  because the vtable is authored rather than emitted.

  Contrast include/Bottle.hpp: that wrapper has to reserve `extra` bytes by hand
  and forward every method to a C function.  Here there is no wrapper -- the
  C++ class IS the implementation, and the C functions are the wrapper.
*/

#include <cstdio>

#include "oopc/SizedBottle.hpp"

/* Polymorphic through a base reference: the override is selected by the C
   vtable, so a SizedBottle passed here clamps. */
static void fill(oopc1::Bottle &b)
{
  std::printf("  %-12s volume ", b.type()->name);
  b.setVolume(1.0);
  std::printf("%.1lf ", b.getVolume());
  b.setVolume(100.0);
  std::printf("-> %.1lf\n", b.getVolume());
}

int main()
{
  std::printf("layout\n");
  std::printf("  sizeof(oopc1::Bottle)      = %zu  (::Bottle      = %zu)\n",
              sizeof(oopc1::Bottle), sizeof(::Bottle));
  std::printf("  sizeof(oopc1::SizedBottle) = %zu  (::SizedBottle = %zu)\n",
              sizeof(oopc1::SizedBottle), sizeof(::SizedBottle));

  std::printf("\ndispatch through oopc1::Bottle &\n");
  {
    /* RAII: the destructor runs the whole C fini chain, and it runs at the
       closing brace.  No new, no delete, no Bottle_fini call. */
    oopc1::Bottle plain;
    oopc1::SizedBottle sized;
    sized.setMaxVolume(10);

    fill(plain);
    fill(sized);

    /* The object and its C view are the same bytes at the same address --
       hand it straight to any C or host-language API. */
    ::Bottle *asC = reinterpret_cast< ::Bottle *>(&sized);
    std::printf("\ninterop\n");
    std::printf("  &sized=%p  as ::Bottle*=%p  same=%d\n",
                (void *)&sized, (void *)asC, (void *)&sized == (void *)asC);
    std::printf("  ::Bottle_getVolume on it   = %.1lf\n", ::Bottle_getVolume(asC));
    std::printf("  ::SizedBottle_cast(it)     = %s\n",
                ::SizedBottle_cast(asC) ? "ok" : "NULL");
    std::printf("  ::SizedBottle_cast(&plain) = %s\n",
                ::SizedBottle_cast(&plain) ? "ok" : "NULL");
  }

  return 0;
}
