/*
  A bounded proof of the two classes, for a model checker (CBMC).

  A test runs the cases someone thought of.  This file is checked for EVERY
  value the nondet_ functions could return: every class, every sequence of
  STEPS calls, every argument.  Each assert below is therefore a claim about
  all of those executions at once, and a failure comes with a counterexample.

  Each side of the C ABI assumes something of the other.  This proof's own
  assumptions are written as __CPROVER_assume and are the whole list:

    K4-1  numbers are finite: no NaN, no infinity          (any_finite, below)
    K4-2  an execution is at most STEPS calls long         (bounded, below)
    K4-3  the checker's model of C is the compiler's

  K4-2 is lifted for the invariant by `inductive`: it starts from ANY state
  that satisfies the invariant, not from a new object, and takes one step.

  Nothing is assumed about malloc: it may return null, and then Class_new must.

  Run: make prove        (labels name the requirement, docs/REQUIREMENTS_v9.md;
                          the proof itself is REQ9/PRF-1 to PRF-3)
*/

#include "Bottle.h"
#include "SizedBottle.h"
#include "Type.h"

#ifndef STEPS
#define STEPS 4
#endif

int nondet_int(void);
double nondet_double(void);

/* K4-1: plain finite numbers only. */
static double any_finite(void)
{
  double x = nondet_double();
  __CPROVER_assume(__CPROVER_isfinited(x));
  return x;
}

/* What REQ2/OM-6 says setVolume and setMaxVolume do, written out. */
static double clamp(double x, double low, double high)
{
  if (x < low) return low;
  if (x > high) return high;
  return x;
}

/* One arbitrary call on a SizedBottle, through the base where the base has the
   method, with its exact result asserted. */
static void step_sized(SizedBottle *sized)
{
  Bottle *b = (Bottle*)sized;
  double volume = Bottle_getVolume(b);
  double max = SizedBottle_getMaxVolume(sized);
  double x = any_finite();

  if (nondet_int()) {
    Bottle_setVolume(b, x);            /* REQ2/OM-1: the override runs */
    __CPROVER_assert(Bottle_getVolume(b) == clamp(x, 0, max),
                     "REQ2/OM-6 setVolume clamps into [0, max]");
    __CPROVER_assert(SizedBottle_getMaxVolume(sized) == max,
                     "REQ2/OM-6 setVolume leaves the max alone");
  } else {
    SizedBottle_setMaxVolume(sized, x);
    __CPROVER_assert(SizedBottle_getMaxVolume(sized) == clamp(x, 0, x),
                     "REQ2/OM-6 setMaxVolume stores a negative as 0");
    __CPROVER_assert(Bottle_getVolume(b) == clamp(volume, volume, SizedBottle_getMaxVolume(sized)),
                     "REQ2/OM-6 setMaxVolume lowers the volume to the new max, and only that");
  }

  __CPROVER_assert(0 <= Bottle_getVolume(b)
                   && Bottle_getVolume(b) <= SizedBottle_getMaxVolume(sized),
                   "REQ2/OM-6 0 <= volume <= maxVolume");
}

/* K4-2: every execution of up to STEPS calls on a new object of either class. */
static void bounded(void)
{
  int want_sized = nondet_int();
  Bottle *b = want_sized ? (Bottle*)SizedBottle_new() : Bottle_new();
  int i;

  if (b == 0) return;                  /* malloc failed, and Class_new said so */

  __CPROVER_assert(Bottle_cast(b) == b, "REQ2/TY-2 every object is-a Bottle");
  __CPROVER_assert((SizedBottle_cast(b) != 0) == (want_sized != 0),
                   "REQ2/TY-2 SizedBottle_cast is non-null exactly for a SizedBottle");
  __CPROVER_assert(Type_of(b) == (want_sized ? SizedBottle_getType() : Bottle_getType()),
                   "REQ2/TY-1 an object reports its own class");

  if (want_sized) {
    __CPROVER_assert(Bottle_getVolume(b) == 0 && SizedBottle_getMaxVolume((SizedBottle*)b) == 1,
                     "REQ2/OM-6 a new SizedBottle has volume 0 and max 1");
  }

  for (i = 0; i < STEPS; ++i) {
    if (want_sized) {
      step_sized((SizedBottle*)b);
    } else {
      double x = any_finite();
      Bottle_setVolume(b, x);
      __CPROVER_assert(Bottle_getVolume(b) == x, "REQ2/OM-1 a plain Bottle keeps what it is given");
    }
  }

  Bottle_delete(b);                    /* REQ1/OM-5: and nothing is leaked */
}

/* The invariant with no bound on history: from ANY SizedBottle state that
   satisfies it, one arbitrary call leaves it satisfied.  With "a new one
   satisfies it" (above), that is induction over every execution.  The fields
   are written directly here, and only here, to name an arbitrary state. */
static void inductive(void)
{
  SizedBottle sized;
  SizedBottle_init(&sized);
  sized.maxVolume = any_finite();
  sized.volume = any_finite();
  __CPROVER_assume(0 <= sized.volume && sized.volume <= sized.maxVolume);

  step_sized(&sized);

  SizedBottle_fini(&sized);
}

int main(void)
{
  if (nondet_int()) bounded(); else inductive();
  return 0;
}
