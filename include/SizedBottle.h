#pragma once

#include "Bottle.h"

#ifdef __cplusplus
extern "C" {
#endif

  struct SizedBottleStruct;
  typedef struct SizedBottleStruct SizedBottle;

  typedef struct {
    /* exactly match Bottle vftbl first (base class), same order, same types.
       Slots take Bottle * -- not SizedBottle * -- so an inherited slot is
       assignable with no cast.  See the note in Bottle.h. */
    const Type *type;
    void (*fini)(Bottle *me);
    double (*getVolume)(const Bottle *me);
    void (*setVolume)(Bottle *me,double value);

    /* add new methods below -- also typed on Bottle *, so that a class derived
       from SizedBottle can inherit them without a cast in turn */
    double (*getMaxVolume)(const Bottle *me);
    void (*setMaxVolume)(Bottle *me,double value);
  } SizedBottleVftbl;

  struct SizedBottleStruct {
    /* exactly match Bottle decl first (base class) with new vftbl */
    const SizedBottleVftbl *vftbl;
    double volume;

    /* add new fields below */
    double maxVolume;
  };

  SizedBottle *SizedBottle_new(void);
  void SizedBottle_delete(SizedBottle *me);

  void SizedBottle_init(SizedBottle *me);
  void SizedBottle_fini(SizedBottle *me);

  void SizedBottle_setVolume(SizedBottle *me, double value);
  double SizedBottle_getVolume(const SizedBottle *me);
  void SizedBottle_setMaxVolume(SizedBottle *me, double value);
  double SizedBottle_getMaxVolume(const SizedBottle *me);

  /* Checked downcast -- 0 if `object` is a plain Bottle rather than a
     SizedBottle.  This is the point of Type: a host language handing us a
     pointer can be told no. */
  SizedBottle *SizedBottle_cast(void *object);

  /* see Bottle_getType */
  const Type *SizedBottle_getType(void);

  /* "protected" -- see the note in Bottle.h */
  extern const Type SizedBottle_type;
  extern const SizedBottleVftbl SizedBottle_vftbl;

  void SizedBottle_vf_fini(Bottle *me);
  void SizedBottle_vf_setVolume(Bottle *me, double value);
  double SizedBottle_vf_getMaxVolume(const Bottle *me);
  void SizedBottle_vf_setMaxVolume(Bottle *me, double value);

#ifdef __cplusplus
}
#endif
