#include <stdlib.h>

#include "Bottle.h"
#include "Trace.h"

#ifdef __cplusplus
extern "C" {
#endif

  /* vtable implementations -- see the "protected" note in Bottle.h */

  void Bottle_vf_fini(Bottle *me)
  {
    TRACE("Bottle vf fini(%p)\n",(void*)me);
  }

  double Bottle_vf_getVolume(const Bottle *me)
  {
    TRACE("Bottle vf getVolume(%p)\n",(void*)me);
    return me->volume;
  }

  void Bottle_vf_setVolume(Bottle *me, double value)
  {
    TRACE("Bottle vf setVolume(%p,%lf)\n",(void*)me,value);
    me->volume = value;
  }

  /*
    Class constants.  Both are const objects with compile-time initializers, so
    they live in read-only memory: no lazy fill-in, no data race between threads
    racing into Bottle_new(), and nothing writable to corrupt.
  */
  const Type Bottle_type = { "Bottle", 0, sizeof(Bottle) };

  const BottleVftbl Bottle_vftbl = {
    &Bottle_type,
    Bottle_vf_fini,
    Bottle_vf_getVolume,
    Bottle_vf_setVolume
  };

  Bottle *Bottle_new(void)
  {
    Bottle *me=(Bottle*)malloc(sizeof(Bottle));
    TRACE("Bottle_new()=%p\n",(void*)me);
    if (me != 0) { Bottle_init(me); }
    return me;
  }
  
  void Bottle_delete(Bottle *me) {
    TRACE("Bottle_delete(%p)\n",(void*)me);
    if (me != 0) {
      me->vftbl->fini(me);
      free(me);
    }
  }

  void Bottle_init(Bottle *me) {
    TRACE("Bottle_init(%p)\n",(void*)me);
    me->vftbl=&Bottle_vftbl;
    me->volume=0;
  }

  void Bottle_fini(Bottle *me) {
    TRACE("Bottle_fini(%p)\n",(void*)me);
    me->vftbl->fini(me);
  }

  double Bottle_getVolume(const Bottle *me)
  {
    TRACE("Bottle_getVolume(%p)\n",(void*)me);
    return me->vftbl->getVolume(me);
  }
  
  void Bottle_setVolume(Bottle *me, double value)
  {
    TRACE("Bottle_setVolume(%p,%lf)\n",(void*)me,value);
    me->vftbl->setVolume(me,value);
  }

  Bottle *Bottle_cast(void *object)
  {
    return (Bottle*)Type_cast(object,&Bottle_type);
  }

  const Type *Bottle_getType(void)
  {
    return &Bottle_type;
  }

#ifdef __cplusplus
} /* extern "C" */
#endif
