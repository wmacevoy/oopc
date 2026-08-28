#include <stdlib.h>

#include "SizedBottle.h"
#include "Trace.h"

#ifdef __cplusplus
extern "C" {
#endif

  /*
    vtable implementations.  Each takes Bottle * (the slot type) and downcasts
    to SizedBottle * -- an ordinary data pointer cast, valid because a
    SizedBottle starts with a Bottle.  Nothing here casts a function pointer.
  */

  void SizedBottle_vf_setVolume(Bottle *base, double value)
  {
    SizedBottle *me = (SizedBottle*)base;
    TRACE("SizedBottle vf setVolume(%p,%lf)\n",(void*)me,value);
    if (value < 0) value=0;
    if (value > me->maxVolume) value=me->maxVolume;
    me->volume=value;
  }

  double SizedBottle_vf_getMaxVolume(const Bottle *base)
  {
    const SizedBottle *me = (const SizedBottle*)base;
    TRACE("SizedBottle vf getMaxVolume(%p)\n",(const void*)me);
    return me->maxVolume;
  }

  void SizedBottle_vf_setMaxVolume(Bottle *base, double value)
  {
    SizedBottle *me = (SizedBottle*)base;
    TRACE("SizedBottle vf setMaxVolume(%p,%lf)\n",(void*)me,value);
    if (value < 0) value = 0;
    me->maxVolume = value;
  }

  void SizedBottle_vf_fini(Bottle *base)
  {
    TRACE("SizedBottle vf fini(%p)\n",(void*)base);
    /* chain to the base implementation: a direct, NON-virtual call.  Calling
       Bottle_fini here instead would re-dispatch through the vtable, land back
       in this function, and recurse forever. */
    Bottle_vf_fini(base);
  }

  /*
    Class constants.  The parent link is &Bottle_type and the inherited slot is
    Bottle_vf_getVolume -- both compile-time addresses, so the whole table is
    const with no runtime setup and no cast.
  */
  const Type SizedBottle_type = { "SizedBottle", &Bottle_type, sizeof(SizedBottle) };

  const SizedBottleVftbl SizedBottle_vftbl = {
    &SizedBottle_type,
    SizedBottle_vf_fini,
    Bottle_vf_getVolume,
    SizedBottle_vf_setVolume,
    SizedBottle_vf_getMaxVolume,
    SizedBottle_vf_setMaxVolume
  };

  SizedBottle *SizedBottle_new(void)
  {
    SizedBottle *me=(SizedBottle*)malloc(sizeof(SizedBottle));
    TRACE("SizedBottle_new()=%p\n",(void*)me);
    if (me != 0) SizedBottle_init(me);
    return me;
  }

  void SizedBottle_delete(SizedBottle *me) {
    TRACE("SizedBottle_delete(%p)\n",(void*)me);
    if (me != 0) {
      SizedBottle_fini(me);
      free(me);
    }
  }

  void SizedBottle_init(SizedBottle *me) {
    TRACE("SizedBottle_init(%p)\n",(void*)me);
    Bottle_init((Bottle*)me);
    me->vftbl=&SizedBottle_vftbl;
    me->maxVolume=1;
  }

  void SizedBottle_fini(SizedBottle *me) {
    TRACE("SizedBottle_fini(%p)\n",(void*)me);
    me->vftbl->fini((Bottle*)me);
  }
  
  double SizedBottle_getVolume(const SizedBottle *me)
  {
    TRACE("SizedBottle_getVolume(%p)\n",(const void*)me);
    return me->vftbl->getVolume((const Bottle*)me);
  }
  
  void SizedBottle_setVolume(SizedBottle *me, double value)
  {
    TRACE("SizedBottle_setVolume(%p,%lf)\n",(void*)me,value);
    me->vftbl->setVolume((Bottle*)me,value);
  }

  double SizedBottle_getMaxVolume(const SizedBottle *me)
  {
    TRACE("SizedBottle_getMaxVolume(%p)\n",(const void*)me);
    return me->vftbl->getMaxVolume((const Bottle*)me);
  }
  
  void SizedBottle_setMaxVolume(SizedBottle *me, double value)
  {
    TRACE("SizedBottle_setMaxVolume(%p,%lf)\n",(void*)me,value);
    me->vftbl->setMaxVolume((Bottle*)me,value);
  }

  SizedBottle *SizedBottle_cast(void *object)
  {
    return (SizedBottle*)Type_cast(object,&SizedBottle_type);
  }

  const Type *SizedBottle_getType(void)
  {
    return &SizedBottle_type;
  }

#ifdef __cplusplus
} /* extern "C" */
#endif
