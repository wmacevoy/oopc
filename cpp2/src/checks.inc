/*
  REQ4/INV-3: the behavior the C++ model and its generated C must share.

  One file, compiled twice, so the two sides cannot be held to different checks:
  as C against cpp2/gen (test_generated.c), and as C++ against cpp2/model
  (test_model.cpp, which supplies the same C names over the model).

  The includer defines CHECK_SIDE.  Destruction order (REQ1/OM-5) is checked in
  cpp2/test_oopcc.py, because the model's destructors are empty.
*/

#include <stdio.h>

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

static int run_checks(void)
{
  Bottle *plain = Bottle_new();
  SizedBottle *sized = SizedBottle_new();
  SizedBottle *fresh = SizedBottle_new();
  Bottle *base = (Bottle *)sized;

  SizedBottle_setMaxVolume(sized, 10);
  Bottle_setVolume(plain, 100);
  Bottle_setVolume(base, 100);
  check("REQ2/OM-1 Bottle keeps 100", Bottle_getVolume(plain), 100);
  check("REQ2/OM-1 SizedBottle's override clamps to 10", Bottle_getVolume(base), 10);

  Bottle_setVolume(base, -5);
  check("REQ2/OM-6 negative volume -> 0", Bottle_getVolume(base), 0);
  Bottle_setVolume(base, 8);
  SizedBottle_setMaxVolume(sized, 3);
  check("REQ2/OM-6 lowering the max lowers the volume", Bottle_getVolume(base), 3);
  SizedBottle_setMaxVolume(sized, -1);
  check("REQ2/OM-6 negative max -> 0", SizedBottle_getMaxVolume(sized), 0);
  check("REQ2/OM-6 new max is 1", SizedBottle_getMaxVolume(fresh), 1);
  check("REQ2/OM-6 new volume is 0", Bottle_getVolume((Bottle *)fresh), 0);

  Bottle_delete(plain);
  Bottle_delete(base);
  Bottle_delete((Bottle *)fresh);

  if (failures) {
    printf("%s: %d FAILED\n", CHECK_SIDE, failures);
    return 1;
  }
  printf("%s: all checks passed\n", CHECK_SIDE);
  return 0;
}
