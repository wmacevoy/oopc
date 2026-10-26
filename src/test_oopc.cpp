/* Tests for the C++ wrapper.  Run: make test-cpp
   CXX-1 and CXX-2 are static_asserts in the headers: if this compiles, they
   hold.  Labels name the requirement checked, from docs/REQUIREMENTS_v2.md. */

#include <stdio.h>

#include "Bottle.hpp"
#include "SizedBottle.hpp"

static int failures = 0;

static void check(const char *label, double got, double want)
{
  if (got == want) {
    printf("  ok  %-52s %g\n", label, got);
  } else {
    printf("  FAIL %s: got %g, want %g\n", label, got, want);
    failures++;
  }
}

/* Takes the base by reference; the C vtable picks the override. */
static double fill(cxx::Bottle &b, double value)
{
  b.setVolume(value);
  return b.getVolume();
}

int main()
{
  cxx::Bottle b;
  cxx::SizedBottle sb;
  sb.setMaxVolume(10);

  printf("\n--- checks ---\n");
  check("REQ2/CXX-3 Bottle& on a Bottle keeps 100", fill(b, 100), 100);
  check("REQ2/CXX-3 Bottle& on a SizedBottle clamps to 10", fill(sb, 100), 10);
  check("REQ2/CXX-1 me() is the object itself", (void*)sb.me() == (void*)&sb, 1);

  sb.setMaxVolume(4);
  check("REQ2/OM-6 lowering the max lowers the volume", sb.getVolume(), 4);

  if (failures) {
    printf("\nC++: %d FAILED\n", failures);
    return 1;
  }
  printf("\nC++: all checks passed\n");
  return 0;
}
