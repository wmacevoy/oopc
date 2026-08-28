#pragma once

#ifdef __cplusplus
extern "C" {
#endif
  struct BottleStruct;
  typedef struct BottleStruct Bottle;

  /*
    Every slot takes Bottle * -- the ROOT base type -- at every level of the
    hierarchy, however deep.  That is what lets a derived class inherit a slot
    by naming the same function pointer, with no cast.

    Do not retype a slot to the derived class.  Casting a function pointer to a
    different signature and calling through it is undefined behavior (C11
    6.5.2.2p9) and traps under CFI (-fsanitize=cfi-icall).  A derived
    implementation downcasts its Bottle * argument internally instead, which is
    an ordinary data pointer cast and exactly the prefix layout this pattern is
    built on.
  */
  typedef struct {
    void (*fini)(Bottle *me);
    double (*getVolume)(const Bottle *me);
    void (*setVolume)(Bottle *me, double value);
  } BottleVftbl;

  const BottleVftbl *Bottle_vftbl(void);

  struct BottleStruct {
    const BottleVftbl *vftbl;
    double volume;
  };

  Bottle *Bottle_new(void);
  void Bottle_delete(Bottle *me);
  void Bottle_init(Bottle *me);
  void Bottle_fini(Bottle *me);  

  double Bottle_getVolume(const Bottle *me);
  void Bottle_setVolume(Bottle *me,double value);

  /*
    "Protected": the vtable implementations themselves.  A derived class names
    these to inherit a slot, or to chain to the base version from its own
    override.  They are declared here (rather than being static) so a derived
    class in another translation unit can build its table with a static
    initializer -- which is what keeps the tables const.

    Do not call these directly from application code; call the dispatchers
    above, which go through the vtable and so respect overrides.
  */
  void Bottle_vf_fini(Bottle *me);
  double Bottle_vf_getVolume(const Bottle *me);
  void Bottle_vf_setVolume(Bottle *me, double value);

#ifdef __cplusplus
}
#endif
