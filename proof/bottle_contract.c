/*
  The extender's contract as code: the least Bottle that meets it.

  A SizedBottle is proved over THIS, not over src/Bottle.c or any other real
  Bottle (make prove).  extender_contract.c is run on this file too, so it is
  shown to meet the contract it stands in for.

  For a base as small as Bottle the contract leaves no freedom, so this is
  close to src/Bottle.c without the tracing.  For a real base it is where the
  saving is: a derived class is proved against a page, not against the base.
*/

#include <stdlib.h>

#include "Bottle.h"

void Bottle_vf_fini(Bottle *me) { (void)me; }
double Bottle_vf_getVolume(const Bottle *me) { return me->volume; }
void Bottle_vf_setVolume(Bottle *me, double value) { me->volume = value; }

const Type Bottle_type = { "Bottle", 0, sizeof(Bottle) };

const BottleVftbl Bottle_vftbl = {
  &Bottle_type, Bottle_vf_fini, Bottle_vf_getVolume, Bottle_vf_setVolume
};

void Bottle_init(Bottle *me) { me->vftbl = &Bottle_vftbl; me->volume = 0; }
void Bottle_fini(Bottle *me) { me->vftbl->fini(me); }

Bottle *Bottle_new(void)
{
  Bottle *me = (Bottle*)malloc(sizeof(Bottle));
  if (me != 0) Bottle_init(me);
  return me;
}

void Bottle_delete(Bottle *me)
{
  if (me != 0) { me->vftbl->fini(me); free(me); }
}

double Bottle_getVolume(const Bottle *me) { return me->vftbl->getVolume(me); }
void Bottle_setVolume(Bottle *me, double value) { me->vftbl->setVolume(me, value); }
Bottle *Bottle_cast(void *object) { return (Bottle*)Type_cast(object, &Bottle_type); }
const Type *Bottle_getType(void) { return &Bottle_type; }
