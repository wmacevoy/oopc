#pragma once

#include "Type.h"

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

    The leading Type * is how any object reports its class: see Type.h.
  */
  typedef struct {
    const Type *type;
    void (*fini)(Bottle *me);
    double (*getVolume)(const Bottle *me);
    void (*setVolume)(Bottle *me, double value);
  } BottleVftbl;

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

  /* Checked downcast -- 0 if `object` is not a Bottle or anything derived. */
  Bottle *Bottle_cast(void *object);

  /* The class's Type, as a call.  Bottle_type below is the same object; this
     exists because a plain function is the one thing EVERY foreign-function
     interface can reach -- ctypes, wasm exports and Emscripten all differ on
     how (or whether) they expose a data symbol. */
  const Type *Bottle_getType(void);

  /*
    "Protected": the class's own constants and vtable implementations.

    These are exposed, rather than being static, so a derived class in another
    translation unit can name them in a STATIC INITIALIZER -- which is what lets
    every vtable in the hierarchy be const, built at compile time, and placed in
    read-only memory.  A derived class uses them to inherit a slot, to chain to
    the base version from an override, and to set its Type's parent link.

    Do not call the vf_ functions from application code; call the dispatchers
    above, which go through the vtable and so respect overrides.
  */
  extern const Type Bottle_type;
  extern const BottleVftbl Bottle_vftbl;

  void Bottle_vf_fini(Bottle *me);
  double Bottle_vf_getVolume(const Bottle *me);
  void Bottle_vf_setVolume(Bottle *me, double value);

#ifdef __cplusplus
}
#endif
