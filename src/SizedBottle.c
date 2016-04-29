#include <stdlib.h>
#include <stdio.h>

#include "SizedBottle.h"

#ifdef __cplusplus
extern "C" {
#endif

  static void setVolume(SizedBottle *me, double value)
  {
    printf("SizedBottle vf setVolume(%p,%lf)\n",me,value);
    if (value < 0) value=0;
    if (value > me->maxVolume) value=me->maxVolume;
    me->volume=value;
  }

  static double getMaxVolume(const SizedBottle *me)
  {
    printf("SizedBottle vf getMaxVolume(%p)\n",me);
    return me->maxVolume;
  }

  static void setMaxVolume(SizedBottle *me, double value)
  {
    printf("SizedBottle vf setMaxVolume(%p,%lf)\n",me,value);
    if (value < 0) value = 0;
    me->maxVolume = value;
  }

  static void fini(SizedBottle *me)
  {
    printf("SizedBottle vf fini(%p)\n",me);
    Bottle_vftbl()->fini((Bottle*)me);
  }

  static SizedBottleVftbl vftbl = {0};

  SizedBottleVftbl *SizedBottle_vftbl() {
    if (vftbl.fini == 0) {
      BottleVftbl *super=Bottle_vftbl();
      vftbl.fini = fini;
      vftbl.getVolume = (double (*)(const SizedBottle *me)) super->getVolume;
      vftbl.setVolume = setVolume;
      vftbl.getMaxVolume = getMaxVolume;
      vftbl.setMaxVolume = setMaxVolume;
    }
    return &vftbl;
  }

  SizedBottle *SizedBottle_new()
  {
    SizedBottle *me=(SizedBottle*)malloc(sizeof(SizedBottle));
    printf("SizedBottle_new()=%p\n",me);
    if (me != 0) SizedBottle_init(me);
    return me;
  }

  void SizedBottle_delete(SizedBottle *me) {
    printf("SizedBottle_delete(%p)\n",me);
    if (me != 0) {
      SizedBottle_fini(me);
      printf("delete SizedBottle@%p\n",me);
      free(me);
    }
  }

  void SizedBottle_init(SizedBottle *me) {
    printf("SizedBottle_init(%p)\n",me);
    Bottle_init((Bottle*)me);
    me->vftbl=SizedBottle_vftbl();
    me->maxVolume=1;
  }

  void SizedBottle_fini(SizedBottle *me) {
    printf("SizedBottle_fini(%p)\n",me);
    me->vftbl->fini(me);
  }
  
  double SizedBottle_getVolume(const SizedBottle *me)
  {
    printf("SizedBottle_getVolume(%p)\n",me);
    return me->vftbl->getVolume(me);
  }
  
  void SizedBottle_setVolume(SizedBottle *me, double value)
  {
    printf("SizedBottle_setVolume(%p,%lf)\n",me,value);
    me->vftbl->setVolume(me,value);
  }

  double SizedBottle_getMaxVolume(const SizedBottle *me)
  {
    printf("SizedBottle_getMaxVolume(%p)\n",me);
    return me->vftbl->getMaxVolume(me);
  }
  
  void SizedBottle_setMaxVolume(SizedBottle *me, double value)
  {
    printf("SizedBottle_setMaxVolume(%p,%lf)\n",me,value);
    me->vftbl->setMaxVolume(me,value);
  }

#ifdef __cplusplus
} /* extern "C" */
#endif

