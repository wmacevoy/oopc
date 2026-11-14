/*
  The caller's contract (contract.h), proved in two halves that meet.

    each_class_keeps_it    the real Bottle and the real SizedBottle, from any
                           state, keep the contract.
    client_of_any_subclass a client is proved against the contract ALONE: its
                           object's slots are not Bottle's or SizedBottle's
                           but Any's, which do nothing except keep the contract.

  Together: the client's claim holds for Bottle, for SizedBottle, and for every
  class not yet written that passes the first half.  That is polymorphism as
  something proved, not only something dispatched.

  Run: make prove
*/

#include "contract.h"
#include "SizedBottle.h"

static void each_class_keeps_it(void)
{
  double x = any_finite();
  if (nondet_int()) {
    Bottle b;
    Bottle_init(&b);
    b.volume = any_finite();                 /* any state a Bottle allows */
    caller_contract(&b);
    __CPROVER_assert(settle(&b, x) == Bottle_getVolume(&b),
                     "REQ10/PRF-5 the client's claim, on a real Bottle");
    Bottle_fini(&b);
  } else {
    SizedBottle s;
    SizedBottle_init(&s);
    s.maxVolume = any_finite();              /* any state a SizedBottle allows */
    s.volume = any_finite();
    __CPROVER_assume(0 <= s.volume && s.volume <= s.maxVolume);
    caller_contract((Bottle*)&s);
    __CPROVER_assert(settle((Bottle*)&s, x) == Bottle_getVolume((Bottle*)&s),
                     "REQ10/PRF-5 the client's claim, on a real SizedBottle");
    SizedBottle_fini(&s);
  }
}

/* ANY subclass.  All that is known of its slots is the contract: it holds some
   finite volume; setting what it holds changes nothing; setting anything else
   may leave it holding any finite number at all. */
static double any_volume;

static void Any_vf_fini(Bottle *me) { (void)me; }
static double Any_vf_getVolume(const Bottle *me) { (void)me; return any_volume; }
static void Any_vf_setVolume(Bottle *me, double value)
{
  (void)me;
  if (value != any_volume) any_volume = any_finite();
}

static const Type Any_type = { "Any", &Bottle_type, sizeof(Bottle) };
static const BottleVftbl Any_vftbl = {
  &Any_type, Any_vf_fini, Any_vf_getVolume, Any_vf_setVolume
};

static void client_of_any_subclass(void)
{
  Bottle b;
  Bottle_init(&b);
  b.vftbl = &Any_vftbl;
  any_volume = any_finite();

  caller_contract(&b);                       /* Any keeps the contract it stands for */
  __CPROVER_assert(settle(&b, any_finite()) == Bottle_getVolume(&b),
                   "REQ10/PRF-5 the client's claim, from the contract alone");
  Bottle_fini(&b);
}

int main(void)
{
  if (nondet_int()) each_class_keeps_it(); else client_of_any_subclass();
  return 0;
}
