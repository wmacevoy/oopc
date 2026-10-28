/* Tests for the C object model.  Run: make test-c
   Labels name the requirement checked, from docs/REQUIREMENTS_v2.md. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "Bottle.h"
#include "SizedBottle.h"
#include "Type.h"

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

/* What OM-2 promises an object costs: its vtable pointer and its fields,
   laid out by the compiler like any other struct on this target. */
struct PlainBottle      { const void *vftbl; double volume; };
struct PlainSizedBottle { const void *vftbl; double volume; double maxVolume; };

int main(void)
{
  Bottle *plain = Bottle_new();
  Bottle *sized = (Bottle*)SizedBottle_new();

  printf("dispatch through Bottle*\n");
  SizedBottle_setMaxVolume((SizedBottle*)sized, 10);
  Bottle_setVolume(plain, 100);
  Bottle_setVolume(sized, 100);
  check("REQ2/OM-1 Bottle keeps 100", Bottle_getVolume(plain), 100);
  check("REQ2/OM-1 SizedBottle's override clamps to 10", Bottle_getVolume(sized), 10);

  printf("\nSizedBottle holds 0 <= volume <= maxVolume\n");
  Bottle_setVolume(sized, -5);
  check("REQ2/OM-6 negative volume -> 0", Bottle_getVolume(sized), 0);
  Bottle_setVolume(sized, 8);
  SizedBottle_setMaxVolume((SizedBottle*)sized, 3);
  check("REQ2/OM-6 lowering the max lowers the volume", Bottle_getVolume(sized), 3);
  SizedBottle_setMaxVolume((SizedBottle*)sized, -1);
  check("REQ2/OM-6 negative max -> 0",
        SizedBottle_getMaxVolume((SizedBottle*)sized), 0);
  {
    SizedBottle *fresh = SizedBottle_new();
    check("REQ2/OM-6 new max is 1", SizedBottle_getMaxVolume(fresh), 1);
    check("REQ2/OM-6 new volume is 0", Bottle_getVolume((Bottle*)fresh), 0);
    Bottle_delete((Bottle*)fresh);
  }

  printf("\nsize: a vtable pointer plus the fields\n");
  check("REQ2/OM-2 sizeof(Bottle)", sizeof(Bottle), sizeof(struct PlainBottle));
  check("REQ2/OM-2 sizeof(SizedBottle)",
        sizeof(SizedBottle), sizeof(struct PlainSizedBottle));

  printf("\nruntime types\n");
  check("REQ2/TY-1 dynamic type of a SizedBottle",
        strcmp(Type_name(Type_of(sized)), "SizedBottle") == 0, 1);
  check("REQ2/TY-1 its parent is Bottle",
        Type_parent(Type_of(sized)) == &Bottle_type, 1);
  check("REQ2/TY-1 Bottle is a root", Type_parent(&Bottle_type) == 0, 1);
  check("REQ2/TY-4 Type_size", Type_size(&SizedBottle_type), sizeof(SizedBottle));

  printf("\nchecked downcasts\n");
  check("REQ2/TY-2 SizedBottle_cast(plain Bottle) is null",
        SizedBottle_cast(plain) == 0, 1);
  check("REQ2/TY-2 SizedBottle_cast(SizedBottle) is itself",
        (void*)SizedBottle_cast(sized) == (void*)sized, 1);
  check("REQ2/TY-2 Bottle_cast(SizedBottle) is itself",
        (void*)Bottle_cast(sized) == (void*)sized, 1);
  check("REQ2/TY-2 Bottle_cast(null) is null", Bottle_cast(0) == 0, 1);

  printf("\ncaller-owned memory\n");
  {
    /* malloc stands in for any host allocator: C sees only the bytes. */
    SizedBottle *mine = malloc(Type_size(&SizedBottle_type));
    SizedBottle_init(mine);
    SizedBottle_setMaxVolume(mine, 3);
    Bottle_setVolume((Bottle*)mine, 99);
    check("REQ2/TY-3 clamped in caller-owned memory", Bottle_getVolume((Bottle*)mine), 3);
    SizedBottle_fini(mine);
    free(mine);
  }

  Bottle_delete(plain);
  Bottle_delete(sized);

  if (failures) {
    printf("\nC: %d FAILED\n", failures);
    return 1;
  }
  printf("\nC: all checks passed\n");
  return 0;
}
