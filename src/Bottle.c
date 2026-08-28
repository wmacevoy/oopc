#include <stdlib.h>
#include <stdio.h>

#include "Bottle.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* vtable implementations -- see the "protected" note in Bottle.h */

  void Bottle_vf_fini(Bottle *me)
  {
    printf("Bottle vf fini(%p)\n",(void*)me);
  }

  double Bottle_vf_getVolume(const Bottle *me)
  {
    printf("Bottle vf getVolume(%p)\n",(void*)me);
    return me->volume;
  }

  void Bottle_vf_setVolume(Bottle *me, double value)
  {
    printf("Bottle vf setVolume(%p,%lf)\n",(void*)me,value);
    me->volume = value;
  }

  /* One const table per class, initialized at compile time so it lands in
     read-only data.  No lazy fill-in, so no data race between threads racing
     into Bottle_new(), and nothing writable for a stray pointer to corrupt. */
  static const BottleVftbl vftbl = {
    Bottle_vf_fini,
    Bottle_vf_getVolume,
    Bottle_vf_setVolume
  };

  const BottleVftbl *Bottle_vftbl(void) {
    return &vftbl;
  }

  Bottle *Bottle_new(void)
  {
    Bottle *me=(Bottle*)malloc(sizeof(Bottle));
    printf("Bottle_new()=%p\n",(void*)me);
    if (me != 0) { Bottle_init(me); }
    return me;
  }
  
  void Bottle_delete(Bottle *me) {
    printf("Bottle_delete(%p)\n",(void*)me);
    if (me != 0) {
      me->vftbl->fini(me);
      free(me);
    }
  }

  void Bottle_init(Bottle *me) {
    printf("Bottle_init(%p)\n",(void*)me);
    me->vftbl=Bottle_vftbl();
    me->volume=0;
  }

  void Bottle_fini(Bottle *me) {
    printf("Bottle_fini(%p)\n",(void*)me);
    me->vftbl->fini(me);
  }

  double Bottle_getVolume(const Bottle *me)
  {
    printf("Bottle_getVolume(%p)\n",(void*)me);
    return me->vftbl->getVolume(me);
  }
  
  void Bottle_setVolume(Bottle *me, double value)
  {
    printf("Bottle_setVolume(%p,%lf)\n",(void*)me,value);
    me->vftbl->setVolume(me,value);
  }

#ifdef __cplusplus
} /* extern "C" */
#endif
