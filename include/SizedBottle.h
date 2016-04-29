#pragma once

#include "Bottle.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct SizedBottleStruct;
  typedef struct SizedBottleStruct SizedBottle;

  typedef struct {
    /* exacly match Bottle vftbl first (base class) with SizedBottle */
    void (*fini)(SizedBottle *me);
    double (*getVolume)(const SizedBottle *me);
    void (*setVolume)(SizedBottle *me,double value);

    /* add new methods below */
    double (*getMaxVolume)(const SizedBottle *me);
    void (*setMaxVolume)(SizedBottle *me,double value);
  } SizedBottleVftbl;

  SizedBottleVftbl *SizedBottle_vftbl();

  struct SizedBottleStruct {
    /* exactly match Bottle decl first (base class) with new vftbl */
    const SizedBottleVftbl *vftbl;
    double volume;

    /* add new fields below */
    double maxVolume;
  };

  SizedBottle *SizedBottle_new();
  void SizedBottle_delete(SizedBottle *me);

  void SizedBottle_init(SizedBottle *me);
  void SizedBottle_fini(SizedBottle *me);

  void SizedBottle_setVolume(SizedBottle *me, double value);
  double SizedBottle_getVolume(const SizedBottle *me);
  void SizedBottle_setMaxVolume(SizedBottle *me, double value);
  double SizedBottle_getMaxVolume(const SizedBottle *me);

#ifdef __cplusplus
}
#endif
