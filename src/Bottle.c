#include <stdlib.h>
#include <stdio.h>

#include "Bottle.h"

#ifdef __cplusplus
extern "C" {
#endif

  static void fini(Bottle *me)
  {
    printf("Bottle vf fini(%p)\n",me);
  }

  static double getVolume(const Bottle *me)
  {
    printf("Bottle vf getVolume(%p)\n",me);
    return me->volume;
  }

  static void setVolume(Bottle *me, double value)
  {
    printf("Bottle vf setVolume(%p,%lf)\n",me,value);
    me->volume = value;
  }

  static BottleVftbl vftbl = {0};

  BottleVftbl *Bottle_vftbl() {
    if (vftbl.fini == 0) {
      vftbl.fini = fini;
      vftbl.getVolume = getVolume;
      vftbl.setVolume = setVolume;
    }
    return &vftbl;
  }

  Bottle *Bottle_new()
  {
    Bottle *me=(Bottle*)malloc(sizeof(Bottle));
    printf("Bottle_new()=%p\n",me);
    if (me != 0) { Bottle_init(me); }
    return me;
  }
  
  void Bottle_delete(Bottle *me) {
    printf("Bottle_delete(%p)\n",me);
    if (me != 0) {
      me->vftbl->fini(me);
      free(me);
    }
  }

  void Bottle_init(Bottle *me) {
    printf("Bottle_init(%p)\n",me);
    me->vftbl=Bottle_vftbl();
    me->volume=0;
  }

  void Bottle_fini(Bottle *me) {
    printf("Bottle_fini(%p)\n",me);
    me->vftbl->fini(me);
  }

  double Bottle_getVolume(const Bottle *me)
  {
    printf("Bottle_getVolume(%p)\n",me);
    return me->vftbl->getVolume(me);
  }
  
  void Bottle_setVolume(Bottle *me, double value)
  {
    printf("Bottle_setVolume(%p,%lf)\n",me,value);
    me->vftbl->setVolume(me,value);
  }

#ifdef __cplusplus
} /* extern "C" */
#endif

