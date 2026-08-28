#include <stdlib.h>
#include <stdio.h>

#include "SizedBottle.h"

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
    printf("SizedBottle vf setVolume(%p,%lf)\n",(void*)me,value);
    if (value < 0) value=0;
    if (value > me->maxVolume) value=me->maxVolume;
    me->volume=value;
  }

  double SizedBottle_vf_getMaxVolume(const Bottle *base)
  {
    const SizedBottle *me = (const SizedBottle*)base;
    printf("SizedBottle vf getMaxVolume(%p)\n",(void*)me);
    return me->maxVolume;
  }

  void SizedBottle_vf_setMaxVolume(Bottle *base, double value)
  {
    SizedBottle *me = (SizedBottle*)base;
    printf("SizedBottle vf setMaxVolume(%p,%lf)\n",(void*)me,value);
    if (value < 0) value = 0;
    me->maxVolume = value;
  }

  void SizedBottle_vf_fini(Bottle *base)
  {
    printf("SizedBottle vf fini(%p)\n",(void*)base);
    /* chain to the base implementation: a direct, NON-virtual call.  Calling
       Bottle_fini here instead would re-dispatch through the vtable, land back
       in this function, and recurse forever. */
    Bottle_vf_fini(base);
  }

  /* Compile-time const table.  getVolume is inherited by naming Bottle's
     implementation directly -- the slot types match, so there is no cast and no
     need to look the base table up at runtime. */
  static const SizedBottleVftbl vftbl = {
    SizedBottle_vf_fini,
    Bottle_vf_getVolume,
    SizedBottle_vf_setVolume,
    SizedBottle_vf_getMaxVolume,
    SizedBottle_vf_setMaxVolume
  };

  const SizedBottleVftbl *SizedBottle_vftbl(void) {
    return &vftbl;
  }

  SizedBottle *SizedBottle_new(void)
  {
    SizedBottle *me=(SizedBottle*)malloc(sizeof(SizedBottle));
    printf("SizedBottle_new()=%p\n",(void*)me);
    if (me != 0) SizedBottle_init(me);
    return me;
  }

  void SizedBottle_delete(SizedBottle *me) {
    printf("SizedBottle_delete(%p)\n",(void*)me);
    if (me != 0) {
      SizedBottle_fini(me);
      printf("delete SizedBottle@%p\n",(void*)me);
      free(me);
    }
  }

  void SizedBottle_init(SizedBottle *me) {
    printf("SizedBottle_init(%p)\n",(void*)me);
    Bottle_init((Bottle*)me);
    me->vftbl=SizedBottle_vftbl();
    me->maxVolume=1;
  }

  void SizedBottle_fini(SizedBottle *me) {
    printf("SizedBottle_fini(%p)\n",(void*)me);
    me->vftbl->fini((Bottle*)me);
  }
  
  double SizedBottle_getVolume(const SizedBottle *me)
  {
    printf("SizedBottle_getVolume(%p)\n",(void*)me);
    return me->vftbl->getVolume((const Bottle*)me);
  }
  
  void SizedBottle_setVolume(SizedBottle *me, double value)
  {
    printf("SizedBottle_setVolume(%p,%lf)\n",(void*)me,value);
    me->vftbl->setVolume((Bottle*)me,value);
  }

  double SizedBottle_getMaxVolume(const SizedBottle *me)
  {
    printf("SizedBottle_getMaxVolume(%p)\n",(void*)me);
    return me->vftbl->getMaxVolume((const Bottle*)me);
  }
  
  void SizedBottle_setMaxVolume(SizedBottle *me, double value)
  {
    printf("SizedBottle_setMaxVolume(%p,%lf)\n",(void*)me,value);
    me->vftbl->setMaxVolume((Bottle*)me,value);
  }

#ifdef __cplusplus
} /* extern "C" */
#endif
