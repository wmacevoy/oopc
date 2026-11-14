/*
  The extender's contract (contract.h): what a derived class may rely on from
  ITS BASE.  Linked with one Bottle and no derived class at all, this shows
  that Bottle meets it.  A SizedBottle proved over bottle_contract.c is then
  correct over this Bottle with no proof of the pair.

  Run: make prove
*/

#include "contract.h"

/* A table that only records which slot ran, to see that the dispatchers go
   through the object's table and do nothing else. */
static int ran;
static void Watch_vf_fini(Bottle *me) { (void)me; ran |= 1; }
static double Watch_vf_getVolume(const Bottle *me) { (void)me; ran |= 2; return 7; }
static void Watch_vf_setVolume(Bottle *me, double value) { (void)me; (void)value; ran |= 4; }
static const Type Watch_type = { "Watch", &Bottle_type, sizeof(Bottle) };
static const BottleVftbl Watch_vftbl = {
  &Watch_type, Watch_vf_fini, Watch_vf_getVolume, Watch_vf_setVolume
};

int main(void)
{
  Bottle b;                                  /* storage the caller owns, not yet an object */
  double x = any_finite();
  const BottleVftbl *table;
  Bottle *heap;

  Bottle_init(&b);
  table = b.vftbl;
  __CPROVER_assert(Type_of(&b) == &Bottle_type && Bottle_getType() == &Bottle_type,
                   "REQ10/PRF-6 E1 Bottle_init makes the storage a Bottle");
  __CPROVER_assert(b.volume == 0, "REQ10/PRF-6 E1 with volume 0");

  Bottle_vf_setVolume(&b, x);
  __CPROVER_assert(b.volume == x, "REQ10/PRF-6 E2 Bottle_vf_setVolume stores exactly x, in the field");
  __CPROVER_assert(Bottle_vf_getVolume(&b) == x, "REQ10/PRF-6 E2 Bottle_vf_getVolume returns the field");
  __CPROVER_assert(b.vftbl == table, "REQ10/PRF-6 E2 the vf_ layer leaves the table alone");

  Bottle_vf_fini(&b);
  __CPROVER_assert(b.vftbl == table && b.volume == x,
                   "REQ10/PRF-6 E3 Bottle_vf_fini touches nothing a derived class owns or reads");

  b.vftbl = &Watch_vftbl;                    /* as a derived init does */
  ran = 0;
  Bottle_setVolume(&b, x);
  __CPROVER_assert(ran == 4, "REQ10/PRF-6 E4 Bottle_setVolume calls the object's own slot, only");
  __CPROVER_assert(Bottle_getVolume(&b) == 7 && ran == 6,
                   "REQ10/PRF-6 E4 Bottle_getVolume calls the object's own slot, only");
  Bottle_fini(&b);
  __CPROVER_assert(ran == 7, "REQ10/PRF-6 E4 Bottle_fini calls the object's own slot, only");
  __CPROVER_assert(Bottle_cast(&b) == &b, "REQ10/PRF-6 E5 Bottle_cast accepts a class derived from Bottle");

  heap = Bottle_new();                       /* may be null: malloc may fail */
  if (heap != 0) {
    __CPROVER_assert(Type_of(heap) == &Bottle_type && heap->volume == 0,
                     "REQ10/PRF-6 E1 Bottle_new is malloc and Bottle_init");
    heap->vftbl = &Watch_vftbl;
    ran = 0;
    Bottle_delete(heap);                     /* and the leak check sees the free */
    __CPROVER_assert(ran == 1, "REQ10/PRF-6 E4 Bottle_delete calls the object's own fini, once");
  }
  return 0;
}
