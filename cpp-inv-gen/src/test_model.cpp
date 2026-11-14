/* REQ4/INV-3: the shared checks against the C++ model.  Run: make test-cpp-inv-gen
   These functions give the model the C names checks.inc is written in. */

#include "SizedBottle.hpp"

static Bottle *Bottle_new() { return new Bottle; }
static SizedBottle *SizedBottle_new() { return new SizedBottle; }
static void Bottle_delete(Bottle *me) { delete me; }
static double Bottle_getVolume(Bottle *me) { return me->getVolume(); }
static void Bottle_setVolume(Bottle *me, double value) { me->setVolume(value); }
static double SizedBottle_getMaxVolume(SizedBottle *me) { return me->getMaxVolume(); }
static void SizedBottle_setMaxVolume(SizedBottle *me, double value) { me->setMaxVolume(value); }

#define CHECK_SIDE "C++ model"
#include "checks.inc"

int main()
{
  return run_checks();
}
