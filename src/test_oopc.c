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

/* What REQ2/OM-2 promises an object costs: its vtable pointer and its fields,
   laid out by the compiler like any other struct on this target. */
struct PlainBottle      { const void *vftbl; double volume; };
struct PlainSizedBottle { const void *vftbl; double volume; double maxVolume; };

/* REQ1/OM-5: the fini slot is the virtual destructor.  To watch it, two classes
   made here the way SizedBottle.c makes one: Inner : Bottle and Outer : Inner.
   They add no fields and no slots, only a fini that writes its letter and
   chains to its base's fini with a direct, non-virtual call.

   Their const tables name Bottle's slots, so this also holds every build of
   the ABI to exporting the "protected" Bottle_vf_ functions (REQ1/INV-1). */
static char finis[8];
static int nfinis = 0;

/* Inner is never instantiated here, so its table is not written out. */
static void Inner_vf_fini(Bottle *me) { finis[nfinis++] = 'I'; Bottle_vf_fini(me); }
static const Type Inner_type = { "Inner", &Bottle_type, sizeof(Bottle) };

static void Outer_vf_fini(Bottle *me) { finis[nfinis++] = 'O'; Inner_vf_fini(me); }
static const Type Outer_type = { "Outer", &Inner_type, sizeof(Bottle) };
static const BottleVftbl Outer_vftbl = {
  &Outer_type, Outer_vf_fini, Bottle_vf_getVolume, Bottle_vf_setVolume
};

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

  printf("\nthe virtual destructor\n");
  {
    Bottle *outer = Bottle_new();
    outer->vftbl = &Outer_vftbl;   /* what Outer_init would do after Bottle_init */
    Bottle_delete(outer);          /* through the base: only a Bottle * is in sight */
    check("REQ1/OM-5 delete through Bottle* ran two finis", nfinis, 2);
    check("REQ1/OM-5 the derived fini first", finis[0] == 'O', 1);
    check("REQ1/OM-5 then its base's, once each", finis[1] == 'I', 1);
    check("REQ2/TY-1 a class made here is-a Bottle", Type_isa(&Outer_type, &Bottle_type), 1);
  }

  printf("\nthe protected layer, by name: what a derived class in C may call\n");
  {
    /* The vf_ functions are the implementations themselves, with no dispatch.
       A derived class calls them to inherit a slot or to chain to its base;
       application code must not, and the second check shows why. */
    SizedBottle *mine = malloc(Type_size(&SizedBottle_type));
    SizedBottle_init(mine);
    SizedBottle_vf_setMaxVolume((Bottle*)mine, 5);
    check("REQ1/INV-1 SizedBottle_vf_getMaxVolume", SizedBottle_vf_getMaxVolume((Bottle*)mine), 5);
    Bottle_vf_setVolume((Bottle*)mine, 100);
    check("REQ1/INV-1 Bottle_vf_setVolume is the base's: no clamp",
          Bottle_vf_getVolume((Bottle*)mine), 100);
    SizedBottle_vf_setVolume((Bottle*)mine, 100);
    check("REQ1/INV-1 SizedBottle_vf_setVolume is the override: clamps",
          Bottle_vf_getVolume((Bottle*)mine), 5);
    SizedBottle_vf_fini((Bottle*)mine);   /* the whole chain, called directly */
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
